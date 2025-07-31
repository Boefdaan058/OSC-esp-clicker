#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

// WiFi settings
const char* ssid = "Stadskerk - Eredienst";
const char* password = "St@dsK3rk!";

// OSC settings (Millumin IP and port)
const char* destIp = "192.168.178.42";  // Change to your Millumin/receiver IP
const int destPort = 8000;

// Pins
#define GREEN_BTN 0
#define RED_BTN   1
#define BLACK_BTN 2
#define LED_PIN   3
#define SWITCH_PIN 4  // SPDT switch for deep sleep control

WiFiUDP Udp;

// Track last button states to detect rising edge presses
bool lastState[3] = {false, false, false};

void setup() {
  // Initialize pins
  pinMode(GREEN_BTN, INPUT_PULLDOWN);
  pinMode(RED_BTN, INPUT_PULLDOWN);
  pinMode(BLACK_BTN, INPUT_PULLDOWN);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);

  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);

  // If switch is off (LOW), enter deep sleep immediately
  if (digitalRead(SWITCH_PIN) == LOW) {
    Serial.println("Switch is off - entering deep sleep...");
    esp_deep_sleep_start();
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start UDP on arbitrary local port
  Udp.begin(8888);
}

void loop() {
  checkButton(GREEN_BTN, "/button/green");
  checkButton(RED_BTN, "/button/red");
  checkButton(BLACK_BTN, "/button/black");

  delay(10); // small debounce delay
}

void checkButton(int pin, const char* path) {
  bool currentState = digitalRead(pin);
  int index = pin;  // Use pin number as index

  // Detect rising edge (button pressed)
  if (currentState && !lastState[index]) {
    sendOscMessage(path);
    flashLED();
  }

  lastState[index] = currentState;
}

void sendOscMessage(const char* address) {
  OSCMessage msg(address);
  msg.add(1);  // Add an integer argument to the OSC message

  Udp.beginPacket(destIp, destPort);
  msg.send(Udp);
  Udp.endPacket();

  msg.empty();
}

void flashLED() {
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}
