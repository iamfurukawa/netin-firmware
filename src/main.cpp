#include <Arduino.h>
#include <TFT_eSPI.h>

#include "display/netin_display.h"
#include "device/device_identity.h"
#include "input/touch_input.h"
#include "network/network_manager.h"
#include "pairing/pairing_manager.h"
#include "storage/settings_store.h"
#include "ui/ui.h"

namespace {
TFT_eSPI tft;
NetinDisplay display(tft);
SettingsStore settingsStore;
UserSettings settings;
DeviceIdentityStore identityStore;
DeviceIdentity identity;
NetworkStore networkStore;
NetworkManager network(networkStore, identity);
PairingStore pairingStore;
PairingManager pairing(identity, network, pairingStore);
TouchInput touch(tft);
Ui ui(display, settingsStore, settings, network, pairing, identity);

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
    identity = identityStore.loadOrCreate();
    network.begin();
    pairing.begin();
    ui.draw();
}

void loop() {
    network.tick();
    pairing.tick();
    ui.tick();
    const TouchEvent event = touch.poll();
    ui.handle(event);
}
