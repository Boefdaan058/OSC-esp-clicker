#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include "esp_sleep.h"

// WiFi settings
const char* ssid = "Stadskerk - Eredienst";
const char* password = "St@dsK3rk!";

// OSC target
const char* destIp = "192.168.178.42";
const int destPort = 8000;

// Pin definitions
#define GREEN_BTN   0
#define RED_BTN     1
#define BLACK_BTN   2
#define LED_PIN     3
#define SWITCH_PIN  4  // SPDT switch for sleep control

// Button indices
enum ButtonIndex { GREEN = 0, RED = 1, BLACK = 2 };
bool lastState[3] = {false, false, false};

WiFiUDP Udp;

void setup() {
  Serial.begin(115200);

  // Configure pins
  pinMode(GREEN_BTN, INPUT_PULLUP);
  pinMode(RED_BTN, INPUT_PULLUP);
  pinMode(BLACK_BTN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // Sleep check
  if (digitalRead(SWITCH_PIN) == LOW) {
    Serial.println("Switch is OFF → entering deep sleep...");
    digitalWrite(LED_PIN, LOW);
    delay(100);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SWITCH_PIN, 1);
    esp_deep_sleep_start();
  }

  // LED ON while awake
  digitalWrite(LED_PIN, HIGH);

  // WiFi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start UDP
  Udp.begin(8888);
}

void loop() {
  checkButton(GREEN_BTN, GREEN, "/action/launchNextColumn");
  checkButton(RED_BTN, RED, "/action/launchPreviousColumn");
  checkButton(BLACK_BTN, BLACK, "/action/displayTestCard"); // Or change if needed

  delay(10);
}

void checkButton(int pin, ButtonIndex index, const char* path) {
  bool currentState = !digitalRead(pin); // LOW when pressed
  if (currentState && !lastState[index]) {
    Serial.print("Button pressed → ");
    Serial.println(path);
    sendOscMessage(path);
    flashLED();
  }
  lastState[index] = currentState;
}

void sendOscMessage(const char* address) {
  OSCMessage msg(address);
  msg.add(1); // You can leave this or remove if Millumin doesn't require an arg

  Udp.beginPacket(destIp, destPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
}

void flashLED() {
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
}
