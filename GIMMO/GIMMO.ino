// ============================================
// GimmoBot - Complete Smart Emotional Weather Bot
// ESP32 + OLED + RoboEyes + LittleFS + Web Config
// ============================================
// Board: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

#include <WiFi.h> 
#include <WebServer.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include "time.h"

// ================== PINS ==================
#define MODE_SWITCH_PIN 4 
#define BUZZER_PIN 27
#define GAME_BUTTON_PIN 32  //FOR THE FLAPPY BIRD GAME

// ================== DISPLAY ==================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

// ================== WEB SERVER ==================
WebServer server(80);

// ================== CONFIG ==================
const char* configFile = "/config.json";
String ssid = "";
String password = "";
String City = "";
String APIKEY = "";
bool configMode = false;

// ================== WEATHER ==================
String weatherCondition = "Loading...";
int humidity = 0;
float temperature = 0.0;
String location = "Loading";

// ================== STATE ==================
int currentMode = 0; // 0 = Emotion, 1 = Weather , 2 = Game
unsigned long lastButtonPress = 0;
const unsigned long buttonDelay = 300;
unsigned long buttonHoldStart = 0;
bool isHoldingButton = false;

// ================== TIME ==================
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

// ================== UI STATE (Weather) ==================
int infoState = 0;
unsigned long lastInfoChange = 0;
const int infoInterval = 4000;
unsigned long lastWeatherUpdate = 0;
const int weatherUpdateInterval = 15 * 60 * 1000;

// ================== EYES ANIMATION STATE ==================
unsigned long moodTimer = 0;
bool loveMode = false;
unsigned long loveStartTime = 0;
bool happyMode = false;
unsigned long happyStartTime = 0;

// ================== CONNECTION STATE ==================
bool wasConnected = false;

// ================== FLAPPY BIRD GAME STATE ==================
struct Bird {
  float y;
  float velocity;
  const float gravity = 1.0;
  const float jump = -7.0;
  const int x = 25;
  const int size = 6;
};

struct Pipe {
  int x;
  int gapY;
  const int width = 12;
  const int gapHeight = 35;
  bool passed = false;
};

Bird bird;
Pipe pipes[3];
int gameScore = 0;
bool gameOver = false;
bool gameStarted = false;
unsigned long lastGameUpdate = 0;
int gameUpdateInterval = 30;

// ================== 8x8 WEATHER ICONS ==================
const unsigned char locationIcon[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x18
};

const unsigned char cloudIcon[] PROGMEM = {
  0x00, 0x3C, 0x7E, 0xFF, 0xFE, 0x7C, 0x00, 0x00
};

const unsigned char dropletIcon[] PROGMEM = {
  0x08, 0x1C, 0x3E, 0x7E, 0x7E, 0x3C, 0x18, 0x00
};

const unsigned char thermometerIcon[] PROGMEM = {
  0x3C, 0x42, 0x42, 0x42, 0x5A, 0x5A, 0x3C, 0x7E
};

const unsigned char sunIcon[] PROGMEM = {
  0x81, 0x42, 0x3C, 0x7E, 0x7E, 0x3C, 0x42, 0x81
};

// ================== SOUND EFFECTS ==================

// Super Mario Startup Sound (13 notes)
void playMarioStartup() {
  int melody[] = {659, 659, 0, 659, 0, 523, 659, 0, 784, 0, 0, 0, 392};
  int durations[] = {150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150};
  
  for(int i = 0; i < 13; i++) {
    if(melody[i] == 0) {
      noTone(BUZZER_PIN);
    } else {
      tone(BUZZER_PIN, melody[i]);
    }
    delay(durations[i]);
  }
  noTone(BUZZER_PIN);
}

// Mushroom Power-Up Sound (4 notes)
void playMushroomPowerUp() {
  int melody[] = {523, 659, 784, 1047};
  int durations[] = {120, 120, 120, 120};
  
  for(int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i]);
    delay(durations[i]);
  }
  noTone(BUZZER_PIN);
}

// Mode Switch Sound (Coin sound)
void playModeSwitch() {
  tone(BUZZER_PIN, 988, 100);
  delay(100);
  tone(BUZZER_PIN, 1319, 400);
  delay(400);
  noTone(BUZZER_PIN);
}
// Flappy Bird Flap Sound
void playFlapSound() {
  tone(BUZZER_PIN, 1000, 80);
  delay(80);
  noTone(BUZZER_PIN);
}

// Flappy Bird Game Over Sound
void playGameOverSound() {
  tone(BUZZER_PIN, 494, 150);  // B
  delay(150);
  tone(BUZZER_PIN, 440, 150);  // A
  delay(150);
  tone(BUZZER_PIN, 392, 150);  // G
  delay(150);
  tone(BUZZER_PIN, 349, 400);  // F
  delay(400);
  noTone(BUZZER_PIN);
}

// ================== LITTLEFS ==================
bool loadConfig() {
  if(!LittleFS.exists(configFile)) return false;
  File file = LittleFS.open(configFile, "r");
  if(!file) return false;
  
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, file);
  file.close();
  
  ssid = doc["ssid"].as<String>();
  password = doc["password"].as<String>();
  City = doc["city"].as<String>();
  APIKEY = doc["apikey"].as<String>();
  
  return ssid.length() > 0;
}

void saveConfig(String s, String p, String key, String city) {
  DynamicJsonDocument doc(1024);
  doc["ssid"] = s;
  doc["password"] = p;
  doc["apikey"] = key;
  doc["city"] = city;
  
  File file = LittleFS.open(configFile, "w");
  serializeJson(doc, file);
  file.close();
}

void deleteConfig() {
  LittleFS.remove(configFile);
}

// ================== EMBEDDED HTML ==================
const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>GimmoBot Setup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    * { box-sizing: border-box; font-family: 'Segoe UI', Tahoma, sans-serif; }
    body { margin: 0; min-height: 100vh; background: radial-gradient(circle at top, #1b1f3b, #0b0f1a); display: flex; justify-content: center; align-items: center; color: #eaeaf0; }
    .card { max-width: 420px; width: 100%; background: rgba(20, 24, 48, 0.95); padding: 28px 24px 30px; border-radius: 20px; box-shadow: 0 20px 40px rgba(0,0,0,0.6); }
    h1 { text-align: center; margin: 0 0 10px; font-size: 22px; }
    .subtitle { text-align: center; font-size: 13px; color: #9aa3ff; margin-bottom: 22px; }
    label { font-size: 13px; color: #cfd3ff; display: block; margin-bottom: 6px; }
    input { width: 100%; padding: 12px 14px; margin-bottom: 16px; border-radius: 12px; border: 1px solid rgba(255,255,255,0.1); background: rgba(10,12,28,0.9); color: #fff; }
    button { width: 100%; padding: 13px; border: none; border-radius: 14px; background: linear-gradient(135deg, #6a7cff, #38e8ff); font-weight: 700; cursor: pointer; color: #fff; }
    .links { margin-top: 18px; text-align: center; }
    .links a { color: #38e8ff; text-decoration: none; font-size: 13px; }
    footer { margin-top: 16px; text-align: center; font-size: 11px; color: #6f74b8; }
  </style>
</head>
<body>
  <div class="card">
    <h1>GimmoBot Setup</h1>
    <div class="subtitle">Smart Desktop Robot Configuration</div>
    <form method="POST" action="/save">
      <label>Wi-Fi SSID</label>
      <input type="text" name="s" required>
      <label>Wi-Fi Password</label>
      <input type="password" name="p" required>
      <label>Weather API Key</label>
      <input type="text" name="a" required>
      <label>City</label>
      <input type="text" name="c" required>
      <button type="submit">Save Configuration</button>
    </form>
    <div class="links">
      <a href="/about">About Us →</a>
    </div>
    <footer>© GimmoBot • IoT Configuration Portal</footer>
  </div>
</body>
</html>
)=====";

const char about_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>About GimmoBot</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    * { box-sizing: border-box; font-family: 'Segoe UI', Tahoma, sans-serif; }
    body { margin: 0; min-height: 100vh; background: radial-gradient(circle at top, #0f1330, #05070f); display: flex; justify-content: center; align-items: center; color: #eaeaf0; }
    .card { max-width: 420px; width: 100%; background: rgba(20, 24, 48, 0.95); padding: 28px 24px; border-radius: 20px; box-shadow: 0 20px 40px rgba(0,0,0,0.6); text-align: center; }
    h1 { margin-bottom: 10px; }
    p { font-size: 13px; color: #cfd3ff; line-height: 1.6; margin-bottom: 20px; }
    .socials { display: flex; justify-content: center; gap: 20px; margin-bottom: 20px; }
    .socials a { text-decoration: none; color: #38e8ff; font-size: 13px; }
    .back { display: inline-block; font-size: 13px; color: #38e8ff; text-decoration: none; }
  </style>
</head>
<body>
  <div class="card">
    <h1>About GimmoBot 🤖</h1>
    <p>GimmoBot is a smart desktop companion robot that combines emotions, real-time weather updates, and IoT technology. Built using ESP32, it brings personality and intelligence to your workspace.</p>
    <div class="socials">
      <a href="https://github.com/yashveermaloniya" target="_blank">GitHub</a>
      <a href="https://www.linkedin.com/in/yashveer-maloniya-1b5b08328/" target="_blank">LinkedIn</a>
    </div>
    <a href="/" class="back">← Back to Setup</a>
  </div>
</body>
</html>
)=====";

// ================== WEB HANDLERS ==================
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleAbout() {
  server.send(200, "text/html", about_html);
}

void handleSave() {
  saveConfig(
    server.arg("s"),
    server.arg("p"),
    server.arg("a"),
    server.arg("c")
  );
  server.send(200, "text/plain", "Saved! Rebooting...");
  delay(1000);
  ESP.restart();
}

// ================== CONFIG MODE ==================
void startConfigMode() {
  configMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("GimmoBot-Setup", "12345678");
  
  server.on("/", handleRoot);
  server.on("/about", handleAbout);
  server.on("/save", handleSave);
  server.begin();
  
  Serial.println("Config mode started");
  Serial.println("SSID: GimmoBot-Setup");
  Serial.println("Password: 12345678");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
}

//this is my credit section
void showCreatorCredit() {
  display.clearDisplay();
  
  // Draw larger rounded rectangle card (centered)
  display.fillRoundRect(10, 15, 108, 34, 8, SSD1306_WHITE);
  display.fillRoundRect(12, 17, 104, 30, 6, SSD1306_BLACK);
  
  // Text - Properly spaced
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // "MADE BY" centered
  display.setCursor(42, 22);
  display.println("MADE BY");
  
  // "YASH VEER" centered (more space)
  display.setCursor(28, 35);
  display.println("    YASH");
  
  display.display();
  delay(2500);
}

// ================== WEATHER ==================
void getWeatherData() {
  if(WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + City + "&units=metric&appid=" + APIKEY;
  http.begin(url);
  
  if(http.GET() > 0) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getString());
    
    weatherCondition = doc["weather"][0]["description"].as<String>();
    humidity = doc["main"]["humidity"].as<int>();
    temperature = doc["main"]["temp"].as<float>();
    location = doc["name"].as<String>();
    
    Serial.println("Weather updated: " + weatherCondition);
  }
  http.end();
}

void initTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(2000);
}

// ================== DISPLAY FUNCTIONS ==================
void drawCenteredString(const String &text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.println(text);
}

void displayWeatherInfo() {
  display.clearDisplay();
  
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    display.setTextSize(1);
    drawCenteredString("Time Error", SCREEN_HEIGHT / 2);
    display.display();
    return;
  }
  
  char timeHour[3], timeMinute[3], timeAmPm[3], dateString[20];
  strftime(timeHour, sizeof(timeHour), "%I", &timeinfo);
  strftime(timeMinute, sizeof(timeMinute), "%M", &timeinfo);
  strftime(timeAmPm, sizeof(timeAmPm), "%p", &timeinfo);
  strftime(dateString, sizeof(dateString), "%a, %b %d", &timeinfo);
  
  String timeStr = String(timeHour) + ":" + String(timeMinute) + " " + String(timeAmPm);
  
  display.setTextSize(2);
  drawCenteredString(timeStr, 5);
  
  display.setTextSize(1);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(dateString, 0, 0, &x1, &y1, &w, &h);
  
  int x = (SCREEN_WIDTH - w) / 2;
  display.drawRect(x - 2, 25 - 2, w + 4, h + 4, SSD1306_WHITE);
  display.setCursor(x, 25);
  display.println(dateString);
  
  String infoText;
  const unsigned char* icon = nullptr;
  
  switch (infoState) {
    case 0: infoText = location; icon = locationIcon; break;
    case 1: infoText = weatherCondition; icon = cloudIcon; break;
    case 2: infoText = "Humidity: " + String(humidity) + "%"; icon = dropletIcon; break;
    case 3: infoText = "Temp: " + String(temperature, 1) + " C"; icon = thermometerIcon; break;
    case 4: infoText = "Have a bright day!"; icon = sunIcon; break;
    case 5: infoText = "Stay hydrated!"; icon = dropletIcon; break;
  }
  
  display.getTextBounds(infoText, 0, 0, &x1, &y1, &w, &h);
  int startX = (SCREEN_WIDTH - (8 + 4 + w)) / 2;
  
  if (icon != nullptr) {
    display.drawBitmap(startX, 47, icon, 8, 8, SSD1306_WHITE);
  }
  
  display.setCursor(startX + 12, 47);
  display.println(infoText);
  
  display.display();
}

void drawHeart(int x, int y, int size, bool smile = false) {
  display.fillCircle(x - size/2, y - size/3, size/2, WHITE);
  display.fillCircle(x + size/2, y - size/3, size/2, WHITE);
  display.fillTriangle(x - size, y - size/4,
                       x + size, y - size/4,
                       x, y + size, WHITE);
  if (smile) {
    int sx = x;
    int sy = y + size/3;
    display.fillTriangle(sx-4, sy,
                         sx+4, sy,
                         sx, sy+4, BLACK);
  }
}

void displayEyes() {
  if (loveMode) {
    display.clearDisplay();
    int pulse = 8 + (sin(millis() * 0.01) * 3);
    drawHeart(40, 30, pulse);
    drawHeart(88, 30, pulse);
    display.display();
    if (millis() - loveStartTime > 4000) {
      loveMode = false;
      roboEyes.open();
    }
    return;
  }
  
  if (happyMode) {
    display.clearDisplay();
    int pulse = 6 + (sin(millis() * 0.02) * 2);
    drawHeart(40, 30, pulse, true);
    drawHeart(88, 30, pulse, true);
    display.display();
    if (millis() - happyStartTime > 3000) {
      happyMode = false;
      roboEyes.open();
    }
    return;
  }
  
  roboEyes.update();
  
  if (millis() - moodTimer > random(6000, 10000)) {
    moodTimer = millis();
    int mood = random(0, 6);
    switch (mood) {
      case 0:
        roboEyes.setMood(DEFAULT);
        break;
      case 1:
        happyMode = true;
        happyStartTime = millis();
        roboEyes.close();
        break;
      case 2:
        roboEyes.setMood(TIRED);
        break;
      case 3:
        roboEyes.setMood(ANGRY);
        break;
      case 4:
        roboEyes.setMood(DEFAULT);
        roboEyes.anim_confused();
        break;
      case 5:
        loveMode = true;
        loveStartTime = millis();
        roboEyes.close();
        break;
    }
  }
}
// ================== FLAPPY BIRD GAME ==================
void initGame() {
  bird.y = 32;
  bird.velocity = 0;
  gameScore = 0;
  gameOver = false;
  gameStarted = false;
  
  for(int i = 0; i < 3; i++) {
    pipes[i].x = 128 + (i * 60);
    pipes[i].gapY = random(15, 40);
    pipes[i].passed = false;
  }
  lastGameUpdate = millis();
}

void updateGame() {
  if(gameOver || !gameStarted) return;
  
  unsigned long currentTime = millis();
  if(currentTime - lastGameUpdate < gameUpdateInterval) return;
  lastGameUpdate = currentTime;
  
  bird.velocity += bird.gravity;
  bird.y += bird.velocity;
  
  if(bird.y < 0 || bird.y > SCREEN_HEIGHT - bird.size) {
    gameOver = true;
    playGameOverSound();
    return;
  }
  
  for(int i = 0; i < 3; i++) {
    pipes[i].x -= 2;
    
    if(pipes[i].x < bird.x + bird.size && 
       pipes[i].x + pipes[i].width > bird.x) {
      if(bird.y < pipes[i].gapY || 
         bird.y + bird.size > pipes[i].gapY + pipes[i].gapHeight) {
        gameOver = true;
        playGameOverSound();
        return;
      }
    }
    
    if(!pipes[i].passed && pipes[i].x + pipes[i].width < bird.x) {
      pipes[i].passed = true;
      gameScore++;
    }
    
    if(pipes[i].x < -pipes[i].width) {
      pipes[i].x = 128;
      pipes[i].gapY = random(15, 40);
      pipes[i].passed = false;
    }
  }
}

void drawGame() {
  display.clearDisplay();
  
  if(!gameStarted) {
    display.setTextSize(2);
    display.setCursor(10, 15);
    display.println("  FLAPPY");
    display.setCursor(20, 35);
    display.println("  BIRD");
    display.setTextSize(1);
    display.setCursor(8, 55);
    display.println("  Press to Start");
    display.display();
    return;
  }
  
  if(gameOver) {
    display.setTextSize(2);
    display.setCursor(5, 10);
    display.println("GAME");
    display.setCursor(10, 28);
    display.println("OVER");
    display.setTextSize(1);
    display.setCursor(30, 48);
    display.print("Score: ");
    display.println(gameScore);
    display.display();
    return;
  }
  
  display.fillCircle(bird.x + bird.size/2, bird.y + bird.size/2, bird.size/2, SSD1306_WHITE);
  
  for(int i = 0; i < 3; i++) {
    display.fillRect(pipes[i].x, 0, pipes[i].width, pipes[i].gapY, SSD1306_WHITE);
    display.fillRect(pipes[i].x, pipes[i].gapY + pipes[i].gapHeight, 
                     pipes[i].width, SCREEN_HEIGHT - pipes[i].gapY - pipes[i].gapHeight, 
                     SSD1306_WHITE);
  }
  
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print("Score:");
  display.println(gameScore);
  display.display();
}

void checkGameButton() {
  static unsigned long lastGameButtonPress = 0;
  bool buttonPressed = digitalRead(GAME_BUTTON_PIN) == LOW;
  
  if(buttonPressed && millis() - lastGameButtonPress > 200) {
    lastGameButtonPress = millis();
    
    if(!gameStarted) {
      gameStarted = true;
      bird.velocity = bird.jump;
      playFlapSound();
    } else if(gameOver) {
      initGame();
    } else {
      bird.velocity = bird.jump;
      playFlapSound();
    }
  }
}

// ================== BUTTON HANDLER ==================
void checkButton() {
  bool currentButtonState = digitalRead(MODE_SWITCH_PIN);
  
  // Button pressed (LOW with INPUT_PULLUP)
  if (currentButtonState == LOW) {
    if (!isHoldingButton) {
      buttonHoldStart = millis();
      isHoldingButton = true;
    }
    
    // Factory reset: Hold for 5 seconds
    unsigned long holdDuration = millis() - buttonHoldStart;
    if (holdDuration > 5000) {
      // Show progress bar
      display.clearDisplay();
      display.setTextSize(1);
      drawCenteredString("FACTORY RESET", 20);
      
      int barWidth = 100;
      int barX = (SCREEN_WIDTH - barWidth) / 2;
      display.drawRect(barX, 35, barWidth, 8, SSD1306_WHITE);
      display.fillRect(barX + 2, 37, barWidth - 4, 4, SSD1306_WHITE);
      display.display();
      delay(500);
      
      deleteConfig();
      ESP.restart();
    } else if (holdDuration > 1000) {
      // Show hold progress
      display.clearDisplay();
      display.setTextSize(1);
      drawCenteredString("Hold to reset...", 20);
      
      int progress = (holdDuration - 1000) * 96 / 4000;
      int barWidth = 96;
      int barX = (SCREEN_WIDTH - barWidth) / 2;
      display.drawRect(barX, 35, barWidth, 8, SSD1306_WHITE);
      display.fillRect(barX + 2, 37, progress, 4, SSD1306_WHITE);
      display.display();
    }
  } else {
    // Button released
    if (isHoldingButton) {
      unsigned long holdDuration = millis() - buttonHoldStart;
      
      // If held less than 5 seconds, it's a normal press
      if (holdDuration < 5000 && holdDuration > 50) {
        unsigned long currentTime = millis();
        if (currentTime - lastButtonPress > buttonDelay) {
          lastButtonPress = currentTime;
          
playModeSwitch();
currentMode = (currentMode + 1) % 3;  // Cycle 0, 1, 2

String modeName = "";
if(currentMode == 0) modeName = "EMOTION";
else if(currentMode == 1) modeName = "WEATHER";
else modeName = "GAME";

Serial.print("Mode: ");
Serial.println(modeName);

// Visual feedback
display.clearDisplay();
display.setTextSize(2);
drawCenteredString(modeName, SCREEN_HEIGHT / 2 - 8);
display.setTextSize(1);
drawCenteredString("MODE", SCREEN_HEIGHT / 2 + 10);
          display.display();
          delay(500);
          
          // Reset eyes animation when entering emotion mode
          if (currentMode == 0) {
            roboEyes.setPosition(DEFAULT);
            roboEyes.open();
            loveMode = false;
            happyMode = false;
            moodTimer = millis();
          }
        }
      }
    }
    isHoldingButton = false;
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== GimmoBot Starting ===");
  
 pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
 pinMode(GAME_BUTTON_PIN, INPUT_PULLUP);  // ADD THIS
 pinMode(BUZZER_PIN, OUTPUT);
  Wire.begin(21, 22);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed!");
    while (true);
  }
  
  display.clearDisplay();
  display.display();
  display.setTextColor(SSD1306_WHITE);
  
  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
  }
  

  
  // 2. Play Mario startup sound
  playMarioStartup();
  
  // 3. Show creator credit
  showCreatorCredit();
  
  // 4. Check for config
  if (!loadConfig()) {
    Serial.println("No config found - starting AP mode");
    startConfigMode();
    return;
  }
  
  // 5. Connect to WiFi
  display.clearDisplay();
  display.setTextSize(1);
  drawCenteredString("Connecting WiFi", SCREEN_HEIGHT / 2 - 10);
  display.display();
  
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi Failed - starting AP mode");
    startConfigMode();
    return;
  }
  
  Serial.println("\nWiFi Connected!");
  wasConnected = true;
  
  // 6. Play mushroom power-up sound
  playMushroomPowerUp();
  
  // 7. Initialize time and weather
  initTime();
  getWeatherData();
  
  // 8. Initialize RoboEyes
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setPosition(DEFAULT);
  roboEyes.open();
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
  roboEyes.setCuriosity(ON);
  // Initialize game
  initGame();
  lastInfoChange = millis();
  moodTimer = millis();
  lastWeatherUpdate = millis();
  
  Serial.println("=== Setup Complete ===");
}

// ================== LOOP ==================
void loop() {
  if (configMode) {
    server.handleClient();
    checkButton();
    return;
  }
  
  checkButton();
  
  if (currentMode == 0) {
    // ========== EMOTION MODE ==========
    displayEyes();
  } else if ( currentMode == 1){
    // ========== WEATHER MODE ==========
    if (WiFi.status() == WL_CONNECTED) {
      if (!wasConnected) {
        wasConnected = true;
        initTime();
        getWeatherData();
        lastWeatherUpdate = millis();
      }
      
      unsigned long currentTime = millis();
      
      if (currentTime - lastWeatherUpdate > weatherUpdateInterval) {
        getWeatherData();
        lastWeatherUpdate = currentTime;
      }
      
      if (currentTime - lastInfoChange > infoInterval) {
        infoState = (infoState + 1) % 6;
        lastInfoChange = currentTime;
      }
      
      displayWeatherInfo();
    } else {
      if (wasConnected) {
        wasConnected = false;
      }
      display.clearDisplay();
      display.setTextSize(1);
      drawCenteredString("WiFi Lost!", SCREEN_HEIGHT / 2 - 10);
      drawCenteredString("Reconnecting...", SCREEN_HEIGHT / 2 + 5);
      display.display();
    }
  } else if (currentMode == 2) {  // ← ADD THIS LINE HERE
    // ========== GAME MODE ==========
    checkGameButton();
    updateGame();
    drawGame();
  }  // ← End of game mode

  delay(10);
}  // ← This closes loop()
