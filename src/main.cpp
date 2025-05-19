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
String ssid;
String password;
String ap_ssid;  // Dynamisch generierte SSID für den Access Point

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
    
    server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
            String newSsid = request->getParam("ssid", true)->value();
            String newPass = request->getParam("pass", true)->value();
    
            prefs.putString("ssid", newSsid);
            prefs.putString("password", newPass);
            prefs.putString("netmode", "client");  // beim Speichern direkt auf Client-Modus schalten
    
            request->send(200, "text/plain", "WLAN gespeichert. Neustart...");
            delay(1000);
            ESP.restart();  // automatisch neu starten
        } else {
            request->send(400, "text/plain", "Fehlende Parameter");
        }
    });    

    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        int n = WiFi.scanNetworks();
        String result = "{\"networks\":[";
        for (int i = 0; i < n; i++) {
            result += "\"" + WiFi.SSID(i) + "\"";
            if (i < n - 1) result += ",";
        }
        result += "]}";
        request->send(200, "application/json", result);
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

    if (networkMode == "client") {
        display.println("Verbinde mit WLAN...");
        display.display();

        WiFi.begin(ssid.c_str(), password.c_str());
        unsigned long startAttemptTime = millis();
        const unsigned long timeout = 10000; // 10 Sekunden

        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Verbinde mit WLAN...");
            display.setCursor(0, 20);
            display.print("Signal: ");
            display.print(WiFi.RSSI());
            display.println(" dBm");
            display.display();

            delay(500);
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WLAN verbunden");
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("WLAN verbunden!");
            display.setCursor(0, 10);
            display.print("IP: ");
            display.println(WiFi.localIP());
            display.display();
            wifiSignalStrength = WiFi.RSSI();
            return;
        }

        // Fallback bei Fehler
        Serial.println("WLAN fehlgeschlagen, starte AP...");
        networkMode = "ap";
    }

    // Access Point starten
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid.c_str(), "esp32pass");
    delay(100);  // kurze Pause zur Initialisierung
    IPAddress myIP = WiFi.softAPIP();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Access Point gestartet!");
    display.setCursor(0, 10);
    display.print("IP: ");
    display.println(myIP);
    display.display();

    Serial.print("AP IP: ");
    Serial.println(myIP);
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
          if (WiFi.getMode() == WIFI_AP) {
              display.println(WiFi.softAPIP());
          } else {
              display.println(WiFi.localIP());
          }          
          display.println("Menu: Home");
          break;

      case MENU_WIFI:
          display.println("WLAN-Info & IP");
          display.println(" ");
      
          display.print("Modus: ");
          if (WiFi.getMode() == WIFI_AP) {
            display.println("AP");
            display.print("SSID: ");
            display.println(ap_ssid);        // <-- dynamische SSID
            display.print("PW: ");
            display.println("esp32pass");    // <-- konstant anzeigen
        }        
            else {
              display.println("Client");
              display.print("SSID: ");
              display.println(WiFi.SSID());
          }
      
          display.print("IP: ");
          if (WiFi.getMode() == WIFI_AP) {
              display.println(WiFi.softAPIP());
          } else {
              display.println(WiFi.localIP());
          }          
      
          if (WiFi.getMode() == WIFI_STA) {
              display.print("Signal: ");
              display.print(WiFi.RSSI());
              display.println(" dBm");
          } else {
              display.println("Signal: n/a");
          }
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
    // Nur prüfen, wenn wir im WLAN-Client-Modus sind
    if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED) {
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
    ssid = prefs.getString("ssid", "ESP32");
    password = prefs.getString("password", "");


    Serial.println("Geladene WLAN-Daten:");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("PASS: ");
    Serial.println(password);

    // SSID prüfen – wenn ungültig, direkt in AP-Modus
    if (ssid.length() == 0 || ssid.length() > 31) {
        Serial.println("Ungültige SSID erkannt. Schalte in AP-Modus.");
        networkMode = "ap";
    }


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

    if (!prefs.isKey("ap_ssid")) {
        String randomSuffix = String((uint32_t)esp_random(), HEX).substring(0, 6);
        String newSSID = "Stripe-" + randomSuffix;
        prefs.putString("ap_ssid", newSSID);
    }
    ap_ssid = prefs.getString("ap_ssid", "Stripe-Setup");

    setupWiFi();

    // Art-Net Setup
    artnet.begin();
    artnet.setArtDmxCallback(onArtnetPacket);

    startWebServer();

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

