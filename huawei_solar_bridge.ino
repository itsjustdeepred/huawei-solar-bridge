#include <SPI.h>
#include <Ethernet.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>

/* --- WAVESHARE ESP32-S3-ETH HARDWARE CONFIGURATION --- */
#define ETH_CS   14
#define ETH_MOSI 11
#define ETH_MISO 12
#define ETH_SCK  13
#define ETH_RST  9

/* --- LOCAL NETWORK (ETH) --- */
IPAddress ip(192, 168, 1, 150);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);
byte mac[] = { 0x42, 0x61, 0xAD, 0x33, 0x09, 0x1B };

/* --- INVERTER NETWORK (WIFI) --- */
const char* inv_ssid = "SUN2000-TAXXXXXXXXXX";
const char* inv_pass = "Changeme";
const char* inv_ip   = "192.168.200.1";
const uint16_t inv_port = 6607;
const uint16_t local_port = 502;

/* --- OTA (change ota_password before deploying) --- */
const char* ota_hostname = "huawei-bridge";
const char* ota_password = "changeme-ota";

/* --- WATCHDOG --- */
#define WDT_TIMEOUT_S 30

EthernetServer ethServer(local_port);
WiFiClient inverterClient;

unsigned long lastWifiAttempt = 0;
const unsigned long WIFI_RETRY_INTERVAL = 10000;

void connectWiFi() {
    Serial.print("[WiFi] Connecting to Inverter...");
    WiFi.begin(inv_ssid, inv_pass);
    unsigned long startWifi = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startWifi < 15000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(WiFi.status() == WL_CONNECTED ? "\n[OK] WiFi Connected!" : "\n[!] WiFi Failed.");
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    // WATCHDOG INIT (auto-reboot if loop() ever stops feeding it)
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WDT_TIMEOUT_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdt_config);
    esp_task_wdt_add(NULL);

    // W5500 PHYSICAL RESET
    pinMode(ETH_RST, OUTPUT);
    digitalWrite(ETH_RST, LOW);
    delay(200);
    digitalWrite(ETH_RST, HIGH);
    delay(500);

    // SPI AND ETHERNET INITIALIZATION
    SPI.begin(ETH_SCK, ETH_MISO, ETH_MOSI, ETH_CS);
    Ethernet.init(ETH_CS);

    Serial.printf("[ETH] Configuring IP %s...\n", ip.toString().c_str());
    Ethernet.begin(mac, ip, dns, gateway, subnet);

    // Hardware Diagnosis
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("!!! ERROR: W5500 not found. Check pins CS (14) and RST (9) !!!");
    } else if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("[!] LAN cable disconnected.");
    } else {
        Serial.print("[OK] Ethernet ready! IP: ");
        Serial.println(Ethernet.localIP());
    }

    // WIFI CONNECTION TO INVERTER
    connectWiFi();

    // OTA SETUP (updates over the wired LAN, port 3232)
    ArduinoOTA.setHostname(ota_hostname);
    ArduinoOTA.setPassword(ota_password);
    ArduinoOTA.onStart([]() { Serial.println("[OTA] Update starting..."); });
    ArduinoOTA.onEnd([]() { Serial.println("\n[OTA] Update complete."); });
    ArduinoOTA.onError([](ota_error_t error) { Serial.printf("[OTA] Error [%u]\n", error); });
    ArduinoOTA.begin();
    Serial.println("[OTA] Ready.");

    ethServer.begin();
    Serial.println("[READY] Bridge active on port 502.");
}

void loop() {
    esp_task_wdt_reset();
    ArduinoOTA.handle();

    // WIFI AUTO-RECONNECT
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastWifiAttempt > WIFI_RETRY_INTERVAL) {
            Serial.println("[WiFi] Disconnected, retrying...");
            WiFi.disconnect();
            WiFi.begin(inv_ssid, inv_pass);
            lastWifiAttempt = millis();
        }
    }

    // Handle Home Assistant traffic (LAN)
    EthernetClient lanClient = ethServer.available();

    if (lanClient) {
        Serial.println("\n[BRIDGE] Request from Home Assistant...");

        // Connect to inverter
        if (inverterClient.connect(inv_ip, inv_port)) {
            inverterClient.setNoDelay(true);
            inverterClient.setTimeout(100);

            while (lanClient.connected() && inverterClient.connected()) {
                // LAN -> Inverter
                while (lanClient.available() > 0) {
                    inverterClient.write(lanClient.read());
                }
                // Inverter -> LAN
                while (inverterClient.available() > 0) {
                    lanClient.write(inverterClient.read());
                }
                esp_task_wdt_reset();
                yield();
            }
            inverterClient.stop();
            Serial.println("[BRIDGE] Session closed.");
        } else {
            Serial.println("[!] Error: Inverter does not accept connection on port 6607.");
        }
        lanClient.stop();
    }

    // Dynamic Ethernet link check
    static bool lastLink = true;
    bool currentLink = (Ethernet.linkStatus() == LinkON);
    if (currentLink != lastLink) {
        Serial.printf("\n[ETH] Link: %s\n", currentLink ? "ACTIVE" : "DISCONNECTED");
        lastLink = currentLink;
    }
}
