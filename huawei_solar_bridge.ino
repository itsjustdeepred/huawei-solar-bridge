#include <SPI.h>
#include <Ethernet.h>
#include <WiFi.h>

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

EthernetServer ethServer(local_port);
WiFiClient inverterClient;

void setup() {
    Serial.begin(115200);
    delay(2000);

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
    Serial.print("[WiFi] Connecting to Inverter...");
    WiFi.begin(inv_ssid, inv_pass);
    unsigned long startWifi = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startWifi < 15000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[OK] WiFi Connected!");
    } else {
        Serial.println("\n[!] WiFi Failed.");
    }

    ethServer.begin();
    Serial.println("[READY] Bridge active on port 502.");
}

void loop() {
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
