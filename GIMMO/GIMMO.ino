// ESP32 Board link "https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"


#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include "time.h"

// ================== PINS ==================
#define MODE_SWITCH_PIN 4
#define BUZZER_PIN 27  // Connect piezo buzzer here

// ================== DISPLAY ==================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================== ROBOEYES ==================
RoboEyes<Adafruit_SSD1306> roboEyes(display);

// --- Wi-Fi & API ---
const char* ssid = "yifi";
const char* password = "12345678";

String City = "GHAZIABAD";
String Authentication_Key = "83a40a7db82e6d2aa3ae339dc321b594";

String currentWeatherURL =
"https://api.openweathermap.org/data/2.5/weather?q=" +
City + ",IN&units=metric&appid=" + Authentication_Key;

// --- Time ---
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

// --- Weather Data ---
String weatherCondition = "Loading...";
int humidity = 0;
float temperature = 0.0;
String location = "";

// --- Mode State ---
int currentMode = 0; // 0 = Weather, 1 = Eyes
unsigned long lastButtonPress = 0;
const unsigned long buttonDelay = 300;

// --- UI State (Weather) ---
int infoState = 0;
unsigned long lastInfoChange = 0;
const int infoInterval = 4000;
unsigned long lastWeatherUpdate = 0;
const int weatherUpdateInterval = 15 * 60 * 1000;

// --- Eyes Animation State ---
unsigned long moodTimer = 0;
bool loveMode = false;
unsigned long loveStartTime = 0;
bool happyMode = false;
unsigned long happyStartTime = 0;

// --- Connection State ---
bool wasConnected = false;

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

// EMU-like startup sound (Beep Bop)
void playStartupSound() {
  tone(BUZZER_PIN, 1047, 120); // C6 - Beep
  delay(140);
  tone(BUZZER_PIN, 784, 120);  // G5 - Bop
  delay(140);
  noTone(BUZZER_PIN);
}

// Mario-like mode switch sound (Coin sound)
void playModeSwitch() {
  tone(BUZZER_PIN, 988, 100);  // B5
  delay(100);
  tone(BUZZER_PIN, 1319, 400); // E6
  delay(400);
  noTone(BUZZER_PIN);
}

// ================== FUNCTION PROTOTYPES ==================
void drawCenteredString(const String &text, int y);
void connectWiFi();
void handleWiFiDisconnection();
void displayWeatherInfo();
void initTime();
void getWeatherData();
void checkButton();
void displayEyes();
void drawHeart(int x, int y, int size, bool smile = false);

// ================== SETUP ==================

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== ESP32 Weather Station Starting ===");
  
  Wire.begin(21, 22);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed!");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Play startup sound
  playStartupSound();
  
  display.clearDisplay();
  display.setTextSize(2);
  drawCenteredString("BOOTING", SCREEN_HEIGHT / 2 - 8);
  display.setTextSize(1);
  drawCenteredString("UP...", SCREEN_HEIGHT / 2 + 10);
  display.display();
  delay(800);

  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    wasConnected = true;
  }

  initTime();
  getWeatherData();

  // Initialize RoboEyes
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setPosition(DEFAULT);
  roboEyes.open();
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
  roboEyes.setCuriosity(ON);

  lastInfoChange = millis();
  moodTimer = millis();
  
  display.clearDisplay();
  display.setTextSize(2);
  drawCenteredString("READY!", SCREEN_HEIGHT / 2 - 5);
  display.display();
  delay(1000);
  
  Serial.println("=== Setup Complete ===");
  Serial.println("Press button on D4 to switch modes");
}

// ================== LOOP ==================

void loop() {
  checkButton();

  if (currentMode == 0) {
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
      handleWiFiDisconnection();
    }
  } else {
    // ========== EYES MODE (YOUR ORIGINAL CODE) ==========
    
    // ❤️ LOVE MODE
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
    
    // 😊 HAPPY MODE
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
    
    // Normal RoboEyes update
    roboEyes.update();
    
    // Random mood change
    if (millis() - moodTimer > random(6000, 10000)) {
      moodTimer = millis();
      int mood = random(0, 6);
      switch (mood) {
        case 0:
          roboEyes.setMood(DEFAULT);
          break;
        case 1:   // 😊 Happy (heart eyes + smile)
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
        case 5:   // ❤️ LOVE MODE
          loveMode = true;
          loveStartTime = millis();
          roboEyes.close();
          break;
      }
    }
  }

  delay(10);
}

// ================== BUTTON HANDLER ==================

void checkButton() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(MODE_SWITCH_PIN);
  
  // Button pressed (goes LOW with INPUT_PULLUP)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastButtonPress > buttonDelay) {
      lastButtonPress = currentTime;
      
      // Play mode switch sound
      playModeSwitch();
      
      // Toggle mode
      currentMode = !currentMode;
      
      Serial.println("\n======================");
      Serial.print("BUTTON PRESSED! Mode: ");
      Serial.println(currentMode == 0 ? "WEATHER" : "EYES");
      Serial.println("======================\n");
      
      // Visual feedback
      display.clearDisplay();
      display.setTextSize(2);
      drawCenteredString(currentMode == 0 ? "WEATHER" : "EYES", SCREEN_HEIGHT / 2 - 8);
      display.setTextSize(1);
      drawCenteredString("MODE", SCREEN_HEIGHT / 2 + 10);
      display.display();
      delay(500);
      
      // Reset eyes animation when entering eyes mode
      if (currentMode == 1) {
        roboEyes.setPosition(DEFAULT);
        roboEyes.open();
        loveMode = false;
        happyMode = false;
        moodTimer = millis();
        Serial.println("Emotion mode initialized");
      }
    }
  }
  
  lastButtonState = currentButtonState;
}

// ================== WIFI FUNCTIONS ==================

void connectWiFi() {
  display.clearDisplay();
  display.setTextSize(1);
  drawCenteredString("Connecting WiFi", SCREEN_HEIGHT / 2 - 10);
  display.display();

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(300);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    display.clearDisplay();
    drawCenteredString("WiFi Connected!", SCREEN_HEIGHT / 2 - 10);
    display.display();
    delay(1000);

    display.clearDisplay();
    display.setTextSize(2);
    drawCenteredString("MADE BY", SCREEN_HEIGHT / 2 - 15);
    drawCenteredString("YASHVEER", SCREEN_HEIGHT / 2 + 5);
    display.display();
    delay(2000);
  } else {
    Serial.println("\nWiFi Failed!");
    display.clearDisplay();
    drawCenteredString("WiFi Failed!", SCREEN_HEIGHT / 2 - 10);
    display.display();
    delay(2000);
  }
}

void handleWiFiDisconnection() {
  display.clearDisplay();
  drawCenteredString("WiFi Lost!", SCREEN_HEIGHT / 2 - 10);
  drawCenteredString("Reconnecting...", SCREEN_HEIGHT / 2 + 5);
  display.display();
  delay(2000);
  connectWiFi();
}

// ================== TIME ==================

void initTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(2000);
}

// ================== WEATHER ==================

void getWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(currentWeatherURL.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        weatherCondition = doc["weather"][0]["description"].as<String>();
        humidity = doc["main"]["humidity"].as<int>();
        temperature = doc["main"]["temp"].as<float>();
        location = doc["name"].as<String>();
        
        Serial.println("Weather updated:");
        Serial.println(weatherCondition);
      } else {
        Serial.println("JSON parse error!");
      }
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

// ================== WEATHER DISPLAY ==================

void displayWeatherInfo() {
  display.clearDisplay();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
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
    case 5: infoText = "Stay hydrated! "; icon = dropletIcon; break;
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

// ================== DRAWING HELPERS ==================

// ❤️ Heart Drawing (YOUR ORIGINAL CODE)
void drawHeart(int x, int y, int size, bool smile) {
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

void drawCenteredString(const String &text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.println(text);
}