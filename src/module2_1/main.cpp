#include <Arduino.h>

constexpr uint8_t BTN_PIN = 4;
constexpr uint8_t RED_LED_PIN = 6;
constexpr uint16_t BLINK_INTERVAL_MS = 500;
constexpr uint16_t BTN_THROTTLE_MS = 250;
constexpr uint16_t LOG_COUNT = 1000;
constexpr uint32_t SERIAL_BAUD_RATE = 115200;

enum LED_STATE {
    OFF,
    ON
};

enum BLINK_MODE {
    BLINK,
    ALWAYS_ON,
    ALWAYS_OFF,
};

volatile bool isButtonPressed = false;
volatile uint32_t lastButtonPressedTimestamp = 0;

struct {
    uint32_t loopTimes[LOG_COUNT];
    uint16_t serialLogCount = 0;
} logData;

void IRAM_ATTR handleButtonInterrupt() {
    uint32_t time = millis();
    if (time - lastButtonPressedTimestamp >= BTN_THROTTLE_MS) {
        isButtonPressed = true;
        lastButtonPressedTimestamp = time;
    }
}

class Blink {
    LED_STATE state = LED_STATE::OFF;
    BLINK_MODE mode = BLINK_MODE::BLINK;
    const uint8_t ledPin;
    const uint16_t blinkIntervalMs;
    uint32_t lastTimeChangeTimestamp = 0;

public:
    Blink(uint8_t pin, uint16_t interval) : ledPin(pin), blinkIntervalMs(interval) {}

    void init() {
        Serial.begin(SERIAL_BAUD_RATE);
        pinMode(ledPin, OUTPUT);
        applyLedState();
    }

    void applyLedState() {
        digitalWrite(ledPin, state); // OFF is 0 so LOW, ON is 1 so HIGH
    }

    void setState(LED_STATE state) {
        this->state = state;
    }

    void toggleState() {
        // stupid requirement to use enum for led state since it has only 2 possible state
        // more usefull could be simple bool then I can just use state = !state;
        setState(state == LED_STATE::ON ? LED_STATE::OFF : LED_STATE::ON);
    }

    void nextMode() {
        switch(mode) {
            case BLINK_MODE::BLINK:
                mode = BLINK_MODE::ALWAYS_ON;
                break;
            case BLINK_MODE::ALWAYS_ON:
                mode = BLINK_MODE::ALWAYS_OFF;
                break;
            case BLINK_MODE::ALWAYS_OFF:
                mode = BLINK_MODE::BLINK;
                break;
        }
    }

    void handleLoop(const uint32_t time) {
        bool hasStateChanged = false;

        if (isButtonPressed) {
            hasStateChanged = true;
            nextMode();
            // isButtonPressed = false;
        }

        if (mode == BLINK_MODE::BLINK && time - lastTimeChangeTimestamp >= blinkIntervalMs) {
            toggleState();
            lastTimeChangeTimestamp = time;
            hasStateChanged = true;
        }

        if (mode == BLINK_MODE::ALWAYS_ON) {
            state = LED_STATE::ON;
        }

        if (mode == BLINK_MODE::ALWAYS_OFF) {
            state = LED_STATE::OFF;
        }

        if (hasStateChanged) {
            applyLedState();
        }
    }
};

Blink redLed(RED_LED_PIN, BLINK_INTERVAL_MS);
Blink internalLed(BUILTIN_LED, BLINK_INTERVAL_MS);

void setup() {
    pinMode(BTN_PIN, INPUT_PULLUP);
    redLed.init();
    internalLed.init();

    attachInterrupt(digitalPinToInterrupt(BTN_PIN), handleButtonInterrupt, FALLING);
}

void loop() {
    const uint32_t startTime = micros();
    const uint32_t now = millis();
    redLed.handleLoop(now);
    internalLed.handleLoop(now);
    if (isButtonPressed) {
        isButtonPressed = false; // this will allow both led react on button
    }
    const uint32_t endTime = micros();

    logData.loopTimes[logData.serialLogCount] = endTime - startTime;
    if (logData.serialLogCount == (LOG_COUNT - 1)) {
        uint32_t sum = 0;
        for (size_t i = 0; i < LOG_COUNT; i++) {
            sum += logData.loopTimes[i];
        }
        Serial.printf("Average time: %d microsec\n", sum / LOG_COUNT); // average time usual is 2 microsec
    }
    logData.serialLogCount = (logData.serialLogCount + 1) % LOG_COUNT;
}