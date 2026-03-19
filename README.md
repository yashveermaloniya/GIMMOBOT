# 🤖 GimmoBot - Smart Desktop Companion Robot

<p align="center">
  <img src="https://img.shields.io/badge/ESP32-IoT-blue?style=for-the-badge"/>
  <img src="https://img.shields.io/badge/Arduino-Embedded-00979D?style=for-the-badge"/>
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge"/>
</p>

<p align="center">
  <b>An ESP32-powered interactive robot with emotions, real-time weather, and gaming — all on an OLED display.</b>
</p>

---

## 📸 Demo

▶️ Full Video: [https://youtube.com/yashveermaloniya](https://www.youtube.com/@YashveerMaloniya)


---

## 🧠 Highlights

- Built using **ESP32 + Embedded Systems**
- Real-time **Weather API integration**
- Interactive UI on **OLED display**
- Multi-mode system (Emotion + Weather + Game)
- Efficient hardware + software design

---

## ✨ Features

## 🎭 Modes

### Emotion Mode
- Animated robotic eyes with realistic movements  
- Multiple moods: Happy, Love, Angry, Tired, Confused  
- Auto-blinking and idle animations  

### Weather Mode
- Live time and date  
- Temperature and humidity  
- Real-time weather conditions  
- Auto-refresh system  

### Game Mode
- Flappy Bird-style gameplay  
- Single-button control  
- Score tracking  
- Instant restart  

---

## 🔊 Sound System

- Startup melody  
- Mode switching sound  
- Game sound effects  

---

## ⚙️ System Features

- WiFi configuration via web portal  
- Persistent storage using LittleFS  
- Auto WiFi reconnect  
- Factory reset support  

---

## 🛠️ Hardware Requirements

| Component | Specification |
|----------|-------------|
| ESP32 | Dev Module |
| OLED | SSD1306 (128x64, I2C) |
| Buzzer | Active/Passive |
| Buttons | 2 Push Buttons |

---

## 🔌 Wiring


OLED → SDA (21), SCL (22)
Buzzer → GPIO 27
Button 1 → GPIO 4
Button 2 → GPIO 32


---

## 💻 Software Requirements

- Arduino IDE  
- ESP32 Board Package  

### Libraries
- Adafruit GFX  
- Adafruit SSD1306  
- ArduinoJson  
- RoboEyes  

---

## 🚀 Installation

```bash
git clone https://github.com/yashveermaloniya/gimmobot.git
cd gimmobot

Open in Arduino IDE

Select ESP32 Dev Module

Upload code

⚙️ Setup

Power on device

Connect to WiFi:

SSID: GimmoBot-Setup

Password: 12345678

Open:

http://192.168.4.1

Enter:

WiFi credentials

API key

City

🎮 Controls
Action	Button
Change Mode	GPIO 4
Game Control	GPIO 32
🌐 API

Uses OpenWeatherMap API:

http://api.openweathermap.org/data/2.5/weather?q=City&units=metric&appid=API_KEY
(here you have to generate your own api key from the website.)

🐛 Troubleshooting

Issue	Solution

OLED not working	 :- Check I2C wiring
WiFi not connecting :-	Reset and retry
No sound	:- Check buzzer
Upload failed	:- Hold BOOT button

👨‍💻 Author

Yash Veer Maloniya

GitHub: https://github.com/yashveermaloniya

LinkedIn: https://www.linkedin.com/in/yashveer-maloniya-1b5b08328/

📄 License

MIT License

