// ESP32 Board link:
// https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

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
String weatherCondition = "";
int humidity = 0;
float temperature = 0;

// ================== STATE ==================
int currentMode = 0; // 0 Emotion, 1 Weather
unsigned long buttonPressStart = 0;
bool buttonHeld = false;

// ================== TIME ==================
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;

// ================== WALKING ANIMATION ==================
#define FRAME_WIDTH 64
#define FRAME_HEIGHT 64
#define FRAME_COUNT 1   // CHANGE when you add frames
#define FRAME_DELAY 80

const uint8_t frames[FRAME_COUNT][512] PROGMEM = {
  { 0 } // <-- PASTE YOUR FRAME DATA HERE
};

// ================== SOUNDS ==================
void playMarioStartup() {
  int melody[] = {659,659,0,659,0,523,659,0,784};
  for(int i=0;i<9;i++){
    if(melody[i]==0) noTone(BUZZER_PIN);
    else tone(BUZZER_PIN, melody[i]);
    delay(120);
  }
  noTone(BUZZER_PIN);
}

void playMushroom() {
  int melody[] = {523,659,784,1047};
  for(int i=0;i<4;i++){
    tone(BUZZER_PIN, melody[i]);
    delay(120);
  }
  noTone(BUZZER_PIN);
}

void playModeSwitch() {
  tone(BUZZER_PIN, 988);
  delay(120);
  tone(BUZZER_PIN, 1319);
  delay(200);
  noTone(BUZZER_PIN);
}

// ================== LITTLEFS ==================
bool loadConfig() {
  if(!LittleFS.exists(configFile)) return false;
  File file = LittleFS.open(configFile, "r");
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
  doc["ssid"]=s;
  doc["password"]=p;
  doc["apikey"]=key;
  doc["city"]=city;
  File file = LittleFS.open(configFile,"w");
  serializeJson(doc,file);
  file.close();
}

void deleteConfig(){
  LittleFS.remove(configFile);
}

// ================== WEB HANDLERS ==================
void handleRoot(){
  server.send(200,"text/html",
  "<form method='POST' action='/save'>"
  "SSID:<input name='s'><br>"
  "PASS:<input name='p'><br>"
  "API:<input name='a'><br>"
  "CITY:<input name='c'><br>"
  "<button>Save</button></form>");
}

void handleSave(){
  saveConfig(
    server.arg("s"),
    server.arg("p"),
    server.arg("a"),
    server.arg("c")
  );
  server.send(200,"text/plain","Saved. Rebooting...");
  delay(1000);
  ESP.restart();
}

// ================== CONFIG MODE ==================
void startConfigMode(){
  configMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("GimmoBot-Setup","12345678");
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}

// ================== WEATHER ==================
void getWeather(){
  if(WiFi.status()!=WL_CONNECTED) return;
  HTTPClient http;
  String url="http://api.openweathermap.org/data/2.5/weather?q="+City+"&units=metric&appid="+APIKEY;
  http.begin(url);
  if(http.GET()>0){
    DynamicJsonDocument doc(2048);
    deserializeJson(doc,http.getString());
    temperature = doc["main"]["temp"];
    humidity = doc["main"]["humidity"];
    weatherCondition = doc["weather"][0]["main"].as<String>();
  }
  http.end();
}

// ================== DISPLAY ==================
void drawCentered(String text,int y){
  int16_t x1,y1; uint16_t w,h;
  display.getTextBounds(text,0,0,&x1,&y1,&w,&h);
  display.setCursor((128-w)/2,y);
  display.print(text);
}

void showWeather(){
  display.clearDisplay();
  display.setTextSize(2);
  drawCentered(String(temperature,0)+"C",10);
  display.setTextSize(1);
  drawCentered(weatherCondition,40);
  display.display();
}

void showEmotion(){
  roboEyes.update();
}

// ================== BUTTON ==================
void checkButton(){
  if(digitalRead(MODE_SWITCH_PIN)==LOW){
    if(!buttonHeld){
      buttonPressStart = millis();
      buttonHeld=true;
    }
    if(millis()-buttonPressStart>5000){
      deleteConfig();
      ESP.restart();
    }
  }else{
    if(buttonHeld){
      if(millis()-buttonPressStart<5000){
        currentMode = !currentMode;
        playModeSwitch();
      }
    }
    buttonHeld=false;
  }
}

// ================== ANIMATION ==================
void playWalkingAnimation(){
  for(int i=0;i<FRAME_COUNT;i++){
    display.clearDisplay();
    display.drawBitmap(32,0,frames[i],64,64,SSD1306_WHITE);
    display.display();
    delay(FRAME_DELAY);
  }
}

// ================== SETUP ==================
void setup(){
  Serial.begin(115200);
  pinMode(MODE_SWITCH_PIN,INPUT_PULLUP);
  pinMode(BUZZER_PIN,OUTPUT);
  Wire.begin(21,22);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);

  LittleFS.begin(true);

  playWalkingAnimation();
  playMarioStartup();

  if(!loadConfig()){
    startConfigMode();
    return;
  }

  WiFi.begin(ssid.c_str(),password.c_str());
  int attempts=0;
  while(WiFi.status()!=WL_CONNECTED && attempts<20){
    delay(500);
    attempts++;
  }

  if(WiFi.status()!=WL_CONNECTED){
    startConfigMode();
    return;
  }

  playMushroom();

  configTime(gmtOffset_sec,0,ntpServer);
  getWeather();

  roboEyes.begin(SCREEN_WIDTH,SCREEN_HEIGHT,100);
  roboEyes.open();

  // Start web server
  server.on("/", []() {
    if (!LittleFS.exists("/index.html")) {
      server.send(404, "text/plain", "index.html not found");
      return;
    }
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

  server.on("/about", []() {
    if (!LittleFS.exists("/about.html")) {
      server.send(404, "text/plain", "about.html not found");
      return;
    }
    File file = LittleFS.open("/about.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

  server.on("/save", handleSave);

  server.begin();
}

// ================== LOOP ==================
void loop(){
  if(configMode){
    server.handleClient();
    return;
  }

  checkButton();

  server.handleClient();

  if(currentMode==0) showEmotion();
  else showWeather();

  delay(10);
}