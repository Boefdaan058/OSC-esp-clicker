#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <WiFiManager.h>

const IPAddress milluminIP(192, 168, 1, 100);  // Show-PC IP
const int oscPort = 5000;
WiFiUDP Udp;

// Knop-pin mapping
const int btnNext = 13;
const int btnPrev = 12;
const int btnBlack = 14;

unsigned long lastDebounceNext = 0;
unsigned long lastDebouncePrev = 0;
unsigned long lastDebounceBlack = 0;
const unsigned long debounceDelay = 50;

bool prevNext = LOW, prevPrev = LOW, prevBlack = LOW;

void setup() {
  Serial.begin(115200);

  pinMode(btnNext, INPUT_PULLDOWN);
  pinMode(btnPrev, INPUT_PULLDOWN);
  pinMode(btnBlack, INPUT_PULLDOWN);

  // Start Captive Portal
  WiFiManager wm;
  wm.autoConnect("Klikker-Setup", "wificonnect");

  Serial.println("Verbonden met WiFi!");
  Udp.begin(12345);
}

void loop() {
  handleButton(btnNext, prevNext, lastDebounceNext, "/millumin/next");
  handleButton(btnPrev, prevPrev, lastDebouncePrev, "/millumin/previous");
  handleButton(btnBlack, prevBlack, lastDebounceBlack, "/millumin/blackout");
}

void handleButton(int pin, bool& prevState, unsigned long& lastDebounce, const char* address) {
  bool reading = digitalRead(pin);
  if (reading != prevState) {
    lastDebounce = millis();
  }

  if ((millis() - lastDebounce) > debounceDelay) {
    if (reading == HIGH && prevState == LOW) {
      sendOSC(address);
    }
  }
  prevState = reading;
}

void sendOSC(const char* path) {
  OSCMessage msg(path);
  Udp.beginPacket(milluminIP, oscPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
  Serial.printf("Verstuurde OSC: %s\n", path);
}