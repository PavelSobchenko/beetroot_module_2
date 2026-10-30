#include <Arduino.h>

constexpr uint8_t RELAY_PIN = 4;
constexpr uint8_t CONTACT_PIN = 7;
constexpr uint8_t RELAY_ON = HIGH;
constexpr uint8_t RELAY_OFF = LOW;
constexpr uint8_t MAX_RESULTS = 10;
constexpr uint8_t TIMEOUT_MS = 100;
constexpr uint32_t SERIAL_BAUD_RATE = 115200;

volatile bool isRequestSent = false;
volatile bool isRelayOn = false;
volatile uint32_t startTimeMc = 0;
volatile uint32_t endTimeMc = 0;
volatile uint32_t results[MAX_RESULTS];
volatile size_t resultIndex = 0;


void IRAM_ATTR handleInterrupt() {
    if (isRequestSent && !isRelayOn) {
        isRelayOn = true;
        endTimeMc = micros();
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(CONTACT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CONTACT_PIN), handleInterrupt, FALLING);
    digitalWrite(RELAY_PIN, RELAY_OFF);
}

void loop() {
    isRelayOn = false;
    startTimeMc = micros();
    isRequestSent = true;
    digitalWrite(RELAY_PIN, RELAY_ON);

    uint32_t t = millis();
    while(!isRelayOn && millis() - t < TIMEOUT_MS) {}
    isRequestSent = false;

    digitalWrite(RELAY_PIN, RELAY_OFF);
    delay(1500);

    if (!isRelayOn) {
        Serial.println("Timeout: relay didnt close");
        return;
    }

    results[resultIndex] = endTimeMc - startTimeMc;
    if (resultIndex == MAX_RESULTS - 1) {
        uint32_t sum = 0;
        for (size_t i = 0; i < MAX_RESULTS; i++) {
            Serial.printf("Try (%d): %lu ", i + 1, results[i]);
            sum += results[i];
        }
        Serial.printf("\nAverage time: %lu microsec\n", sum / MAX_RESULTS);
    }
    resultIndex = (resultIndex + 1) % MAX_RESULTS;
}