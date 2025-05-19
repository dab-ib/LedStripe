
# 💡 LED-Stripes – ESP32 DMX Controller mit Webinterface

Dieses Projekt ist ein vielseitiger LED-Controller für DMX/Art-Net über ESP32. Es kombiniert ein OLED-Menü, Encodersteuerung, SPIFFS-basiertes Webinterface und speichert Einstellungen dauerhaft über `Preferences`.

---

## 🚀 Funktionen

### 🎛️ Hardware-Funktionen
- Steuerung über **Rotary Encoder** mit Button
- OLED-Menü auf 128x64 I²C Display (SSD1306)
- Echtzeit-Anzeige von Status, IP, Signalstärke
- Konfigurierbare DMX-**Universumsnummer**

### 💡 LED-Steuerung
- Unterstützung für **WS2812B-LEDs**
- **Art-Net (DMX over IP)** kompatibel
- Live-Ansteuerung der LEDs über Art-Net-Controller
- Integrierter **LED-Testmodus** (rot, grün, blau, aus)

### 🌐 Webinterface (SPIFFS-basiert)
- Moderne Oberfläche (HTML/CSS/JS)
- AJAX-basierte Konfiguration
- Einstellungen:
  - Universum
  - Netzwerkmodus: **Client** oder **Access Point**
- Änderungen werden **dauerhaft in Preferences gespeichert**

---

## ⚙️ Anforderungen

### 📦 Bibliotheken (`platformio.ini`)
```ini
lib_deps =
  rstephan/ArtnetWifi@^1.6.1
  fastled/FastLED@^3.9.13
  adafruit/Adafruit GFX Library@^1.11.11
  adafruit/Adafruit SSD1306@^2.5.13
  madhephaestus/ESP32Encoder@^0.11.7
  bblanchon/ArduinoJson@^6.21.2
  ESP Async WebServer
  AsyncTCP
```

### 📁 Projektstruktur
```
LED-Stripes/
├── src/
│   └── main.cpp
├── data/
│   ├── index.html
│   ├── style.css
│   └── script.js
├── platformio.ini
```

---

## 🔌 Upload & Flash-Anleitung (PlatformIO)

### 1️⃣ Firmware flashen
Lädt deinen Sketch auf das ESP32-Board:
```bash
pio run --target upload
```

### 2️⃣ Webinterface auf SPIFFS hochladen
Lädt `data/`-Inhalte (HTML/CSS/JS) in den SPIFFS-Dateibereich:
```bash
pio run --target uploadfs
```

> 📌 **Beides muss gemacht werden**, um ein vollständiges, funktionierendes System zu erhalten!

---

## 📲 Zugriff auf das Webinterface

1. Starte den ESP32
2. Öffne die IP-Adresse, die im OLED-Display angezeigt wird
3. Interface z. B.: `http://192.168.x.x`

---

## 🔐 Einstellungen werden gespeichert

- Netzwerkmodus und Universum werden in **Preferences** gespeichert
- Auch nach Neustart des ESP32 bleiben die Werte erhalten

---

## 🧪 Weitere Ideen (Optional)

- OTA-Update via Webinterface
- MQTT-Anbindung für Home Assistant
- Preset-Farben oder Animationen
- Passwortschutz für Webinterface

---

## 🛠️ Entwickler-Info

### Plattform
- ESP32 DevKit
- PlatformIO mit VS Code empfohlen

### Getestet mit:
- `esp32dev` Board
- PlatformIO Core 6.x
- ESP32 Arduino Core >= 2.0.x

---

## 📷 Vorschau

*(Hier kannst du z. B. ein GIF/Screenshot deines Webinterfaces oder OLED-Menüs einfügen)*

---

## 📄 Lizenz

MIT License – feel free to modify & share.
