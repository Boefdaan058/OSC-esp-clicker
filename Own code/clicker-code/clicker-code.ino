#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include "esp_sleep.h"

// === Pin definitions ===
#define GREEN_BTN_PIN  0
#define RED_BTN_PIN    1
#define BLACK_BTN_PIN  2
#define LED_PIN        3
#define SWITCH_PIN     4

// === Button index mapping ===
enum ButtonIndex { GREEN = 0, RED = 1, BLACK = 2 };
bool lastState[3] = {false, false, false};

// === Default config values ===
char osc_ip[16] = "192.168.1.100";
char osc_port_str[6] = "5000";
char cmd_green[32] = "/action/launchNextColumn";
char cmd_red[32]   = "/action/launchPreviousColumn";
char cmd_black[32] = "/blackout.png/";

// === WiFiManager parameters ===
WiFiManagerParameter param_osc_ip("oscip", "OSC Server IP", osc_ip, 16);
WiFiManagerParameter param_osc_port("oscport", "OSC Server Port", osc_port_str, 6);
WiFiManagerParameter param_cmd_green("cmdgreen", "Green Button Command", cmd_green, 32);
WiFiManagerParameter param_cmd_red("cmdred", "Red Button Command", cmd_red, 32);
WiFiManagerParameter param_cmd_black("cmdblack", "Black Button Command", cmd_black, 32);

// === WiFi and WebServer ===
WiFiManager wifiManager;
WebServer server(80);

// === OSC networking ===
WiFiUDP Udp;
IPAddress destIp;
int destPort = 5000;

void setup() {
  Serial.begin(115200);

  pinMode(GREEN_BTN_PIN, INPUT_PULLUP);
  pinMode(RED_BTN_PIN, INPUT_PULLUP);
  pinMode(BLACK_BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // === Sleep check before anything else ===
  if (digitalRead(SWITCH_PIN) == LOW) {
    Serial.println("Switch is ON → Going to deep sleep. Will wake up when switch is turned OFF.");
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 1); // Wake when switch goes HIGH
    delay(500);
    esp_deep_sleep_start();
  } else {
    digitalWrite(LED_PIN, HIGH);  // Turn on LED to show device is active
    Serial.println("Switch is OFF → Staying awake. LED ON.");
  }

  // Add custom parameters to config portal
  wifiManager.addParameter(&param_osc_ip);
  wifiManager.addParameter(&param_osc_port);
  wifiManager.addParameter(&param_cmd_green);
  wifiManager.addParameter(&param_cmd_red);
  wifiManager.addParameter(&param_cmd_black);

  // Attempt to connect to saved WiFi or launch AP
  if (!wifiManager.autoConnect("OSC-CLICKER", "WDYMPASSWORD?")) {
    Serial.println("Failed to connect. Rebooting...");
    delay(3000);
    ESP.restart();
  }

  // Load values from WiFiManager
  strcpy(osc_ip, param_osc_ip.getValue());
  strcpy(osc_port_str, param_osc_port.getValue());
  strcpy(cmd_green, param_cmd_green.getValue());
  strcpy(cmd_red, param_cmd_red.getValue());
  strcpy(cmd_black, param_cmd_black.getValue());

  destIp.fromString(osc_ip);
  destPort = atoi(osc_port_str);
  Udp.begin(12345);  // Local UDP port

  // Web access to config page
  server.on("/", []() {
    server.send(200, "text/html", "<h1>OSC Clicker</h1><a href=\"/config\">Configure</a>");
  });

  server.on("/config", []() {
    server.send(200, "text/plain", "Opening config portal...");
    delay(500);
    wifiManager.startConfigPortal("OSC-CLICKER", "WDYMPASSWORD?");
  });

  server.begin();
  Serial.print("Web server started. Open http://");
  Serial.print(WiFi.localIP());
  Serial.println("/config");
}

void sendOSC(const char* path) {
  OSCMessage msg(path);
  Udp.beginPacket(destIp, destPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
  Serial.print("Sent OSC: ");
  Serial.println(path);
}

void loop() {
  server.handleClient();

  bool stateGreen = digitalRead(GREEN_BTN_PIN) == LOW;
  bool stateRed   = digitalRead(RED_BTN_PIN) == LOW;
  bool stateBlack = digitalRead(BLACK_BTN_PIN) == LOW;

  if (stateGreen && !lastState[GREEN]) {
    sendOSC(cmd_green);
  }
  if (stateRed && !lastState[RED]) {
    sendOSC(cmd_red);
  }
  if (stateBlack && !lastState[BLACK]) {
    sendOSC(cmd_black);
  }

  lastState[GREEN] = stateGreen;
  lastState[RED]   = stateRed;
  lastState[BLACK] = stateBlack;

  delay(10);
}
