#include <Arduino.h>
#include <TFT_eSPI.h>

#include "netin_display.h"
#include "settings_store.h"
#include "touch_input.h"
#include "ui.h"

namespace {
TFT_eSPI tft;
NetinDisplay display(tft);
SettingsStore settingsStore;
UserSettings settings;
TouchInput touch(tft);
Ui ui(display, settingsStore, settings);

uint16_t kTouchCalibration[] = {652, 2994, 423, 3361, 3};
constexpr uint8_t kRgbLedPins[] = {4, 16, 17};
}

void setup() {
    Serial.begin(115200);
    for (const uint8_t pin : kRgbLedPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);  // RGB LED is active-low.
    }

    display.begin();
    tft.setTouch(kTouchCalibration);
    settings = settingsStore.load();
    ui.draw();
}

void loop() {
    const TouchEvent event = touch.poll();
    ui.handle(event);
}
