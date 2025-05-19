#include <Arduino.h>
#include <WiFi.h>
#include <ArtnetWifi.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Encoder.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// OLED Display Konfiguration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define I2C_SDA 21
#define I2C_SCL 22
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Encoder Konfiguration
#define ENCODER_PIN_A 16  
#define ENCODER_PIN_B 17  
#define ENCODER_BUTTON 32 

ESP32Encoder encoder;
volatile bool buttonPressed = false;
int wifiSignalStrength = 0;  

// Website
AsyncWebServer server(80);
bool webServerRunning = false;

Preferences prefs;
String networkMode = "client";  // "client" oder "ap"

// WLAN Zugangsdaten
const char* ssid = "iPhone 16 von Dabib";
const char* password = "Dabib2002";

// Art-Net Konfiguration
ArtnetWifi artnet;
int universe = 0;  

// LED Strip Konfiguration
#define LED_PIN 4      
#define NUM_LEDS 120   
CRGB leds[NUM_LEDS];

// Menü-Definitionen
bool editingUniverse = false;

enum MenuState {
    MENU_HOME,
    MENU_WIFI,
    MENU_ARTNET,
    MENU_DIAGNOSE,
    MENU_WEBSERVER,
    MENU_MAX
};

MenuState menuIndex = MENU_HOME;

// Button Interrupt-Funktion
void IRAM_ATTR onButtonPress() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  
  if (interruptTime - lastInterruptTime > 200) {  // 200ms Entprellzeit
      buttonPressed = true;
  }
  lastInterruptTime = interruptTime;
}

void startWebServer() {
    if (webServerRunning) return;

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"universe\":" + String(universe) + ",";
        json += "\"netmode\":\"" + networkMode + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("universe", true)) {
            universe = request->getParam("universe", true)->value().toInt();
            prefs.putUInt("universe", universe);
        }
        if (request->hasParam("netmode", true)) {
            networkMode = request->getParam("netmode", true)->value();
            prefs.putString("netmode", networkMode);
        }
        request->send(200, "text/plain", "OK");
    });
    

    server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("r")) fill_solid(leds, NUM_LEDS, CRGB::Red);
        else if (request->hasParam("g")) fill_solid(leds, NUM_LEDS, CRGB::Green);
        else if (request->hasParam("b")) fill_solid(leds, NUM_LEDS, CRGB::Blue);
        else fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        request->send(200, "text/plain", "OK");
    });

    server.begin();
    webServerRunning = true;
    Serial.println("Webserver gestartet");
}

void stopWebServer() {
    if (!webServerRunning) return;

    server.end();
    webServerRunning = false;
    Serial.println("Webserver gestoppt");
}

// WLAN-Verbindung aufbauen
void setupWiFi() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (networkMode == "ap") {
        display.println("Starte Access Point...");
        display.display();

        WiFi.softAP("ESP32-Setup", "esp32pass");  // SSID und Passwort für AP-Modus
        IPAddress myIP = WiFi.softAPIP();

        Serial.println("Access Point gestartet!");
        Serial.print("AP IP Adresse: ");
        Serial.println(myIP);

        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("AP gestartet!");
        display.setCursor(0, 10);
        display.print("IP: ");
        display.println(myIP);
        display.display();

    } else {
        display.println("Verbinde mit WLAN...");
        display.display();

        WiFi.begin(ssid, password);
        Serial.print("Verbinde mit WLAN");

        int dotCount = 0;

        while (WiFi.status() != WL_CONNECTED) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Verbinde mit WLAN");

            for (int i = 0; i < dotCount; i++) {
                display.print(".");
            }

            display.setCursor(0, 20);
            if (WiFi.getMode() == WIFI_AP) {
                display.println("Signal: n/a");
            } else {
                display.print("Signal: ");
                display.print(WiFi.RSSI());
                display.println(" dBm");
            }
            
            display.display();

            Serial.print(".");
            dotCount = (dotCount + 1) % 4;
            delay(500);
        }

        Serial.println(" Verbunden!");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("WLAN verbunden!");
        display.setCursor(0, 10);
        display.print("IP: ");
        display.println(WiFi.localIP());
        display.display();
        Serial.print("IP Adresse: ");
        Serial.println(WiFi.localIP());
    }

    wifiSignalStrength = WiFi.RSSI();
}


// OLED Menü aktualisieren
void updateMenuDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  switch (menuIndex) {
      case MENU_HOME:
          display.println("ESP32 DMX Controller");
          display.println(" ");
          display.print("IP: ");
          display.println(WiFi.localIP());
          display.println("Menu: Home");
          break;

      case MENU_WIFI:
          display.println("WLAN-Info & IP");
          display.println(" ");
          display.print("SSID: ");
          display.println(WiFi.SSID());
          display.print("IP: ");
          display.println(WiFi.localIP());
          display.print("Signal: ");
          display.print(WiFi.RSSI());
          display.println(" dBm");
          break;

      case MENU_ARTNET:
          display.println("ArtNet Einstellungen");
          display.println(" ");
          display.print("Universum: ");
          display.print(universe);
          if (editingUniverse) {
              display.print(" <-"); // Cursor anzeigen
          }
          display.println();
          break;

      case MENU_WEBSERVER:
          display.println("Web Interface");
          display.println(" ");
          display.print("Status: ");
          display.println(webServerRunning ? "An" : "Aus");
          break;
      
      case MENU_DIAGNOSE:
          display.println("Diagnose");
          display.println(" ");
          display.print("Heap: ");
          display.println(ESP.getFreeHeap());
          display.print("CPU Takt: ");
          display.println(ESP.getCpuFreqMHz());
          break;
  }

  // Blinken des unteren Cursors, wenn keine Aktion aktiv ist
  if (!editingUniverse) {
      static bool blink = false;
      if (millis() % 1000 < 500) {
          blink = !blink;
      }
      if (blink) {
          display.setCursor(SCREEN_WIDTH / 2 - 3, SCREEN_HEIGHT - 10);
          display.print("_");
      }
  }

  display.display();
}


// Menü-Steuerung mit Encoder
void handleMenuInput(int delta) {
    menuIndex = static_cast<MenuState>((menuIndex + delta + MENU_MAX) % MENU_MAX);
    updateMenuDisplay();
}

// Button-Logik zur Auswahl eines Menüpunkts
void handleButtonPress() {
    Serial.print("Menüpunkt ausgewählt: ");
    Serial.println(menuIndex);

    switch (menuIndex) {
        case MENU_WIFI:
            Serial.println("WLAN wird neu verbunden...");
            WiFi.disconnect();
            delay(1000);
            setupWiFi();
            break;

        case MENU_ARTNET:
            if (editingUniverse) {
                Serial.println("Universum gespeichert.");
                editingUniverse = false;
            } else {
                Serial.println("Bearbeitungsmodus für Universum aktiv.");
                editingUniverse = true;
            }
            break;

        case MENU_WEBSERVER:
            if (webServerRunning) {
                stopWebServer();
            } else {
                startWebServer();
            }
            break;

        case MENU_DIAGNOSE:
            Serial.println("Diagnose gestartet...");
            break;

        default:
            Serial.println("Keine Aktion definiert.");
            break;
    }

    updateMenuDisplay();
}

void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Verbindung verloren. Versuche erneut...");
        setupWiFi();
    }
}

// Art-Net Daten empfangen und LEDs steuern
void onArtnetPacket(uint16_t universeReceived, uint16_t length, uint8_t sequence, uint8_t* data) {
    if (universeReceived == universe) {
        for (int i = 0; i < NUM_LEDS; i++) {
            int dmxIndex = i * 3;
            if (dmxIndex + 2 < length) {
                leds[i].r = data[dmxIndex];
                leds[i].g = data[dmxIndex + 1];
                leds[i].b = data[dmxIndex + 2];
            }
        }
        FastLED.show();
    }
}

void setup() {
    Serial.begin(115200);

    // FastLED initialisieren
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.clear();
    FastLED.show();

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS konnte nicht gestartet werden");
        return;
    }    

    // Preferences für Website
    prefs.begin("config", false);  // Namensraum "config"
    universe = prefs.getUInt("universe", 0);
    networkMode = prefs.getString("netmode", "client");

    // I2C für OLED initialisieren
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 nicht gefunden. Bitte Verkabelung prüfen!");
        while (1);
    }
    display.display();

    // Encoder initialisieren
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachHalfQuad(ENCODER_PIN_A, ENCODER_PIN_B);
    encoder.clearCount();

    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);
    pinMode(ENCODER_BUTTON, INPUT_PULLUP);
    
    attachInterrupt(ENCODER_BUTTON, onButtonPress, FALLING);

    setupWiFi();

    // Art-Net Setup
    artnet.begin();
    artnet.setArtDmxCallback(onArtnetPacket);

    updateMenuDisplay();  // Startanzeige
}

void loop() {
  static unsigned long lastUpdate = 0;
  static unsigned long lastWiFiUpdate = 0;

  artnet.read();

  static long lastPosition = 0;
  long newPosition = encoder.getCount();
  int delta = newPosition - lastPosition;

  if (delta != 0) {
      if (editingUniverse) {
          universe = (universe + delta) % 512; // Universum ändern
          if (universe < 0) universe += 512;
      } else {
          handleMenuInput(delta > 0 ? 1 : -1);
      }
      lastPosition = newPosition;
      updateMenuDisplay();
  }

  if (buttonPressed) {
      buttonPressed = false;
      handleButtonPress();
  }

  if (millis() - lastWiFiUpdate > 30000) {
      lastWiFiUpdate = millis();
      wifiSignalStrength = WiFi.RSSI();
  }

  if (millis() - lastUpdate > 500) {
      lastUpdate = millis();
      checkWiFiConnection();
      updateMenuDisplay();
  }
}

