#pragma once

#include "app/app_types.h"
#include "device/device_identity.h"
#include "display/netin_display.h"
#include "input/touch_input.h"
#include "network/network_manager.h"
#include "pairing/pairing_manager.h"
#include "storage/settings_store.h"

class Ui {
  public:
    Ui(NetinDisplay &display, SettingsStore &store, UserSettings &settings, NetworkManager &network, PairingManager &pairing, const DeviceIdentity &identity)
        : display_(display), store_(store), settings_(settings), network_(network), pairing_(pairing), identity_(identity) {}
    void draw();
    void tick();
    void handle(const TouchEvent &event);

  private:
    enum class Screen : uint8_t { Home, StatusPicker, Settings, Network, Device };
    bool hit(uint16_t x, uint16_t y, int16_t rx, int16_t ry, int16_t rw, int16_t rh) const;
    void drawHome();
    void drawStatusPicker();
    void drawSettings();
    void drawNetwork();
    void drawDevice();
    void applyStatus(PresenceStatus status);

    NetinDisplay &display_;
    SettingsStore &store_;
    UserSettings &settings_;
    NetworkManager &network_;
    PairingManager &pairing_;
    const DeviceIdentity &identity_;
    Screen screen_ = Screen::Home;
    int16_t pickerScroll_ = 0;
    NetworkState lastNetworkState_ = NetworkState::Unconfigured;
    PairingState lastPairingState_ = PairingState::Unpaired;
    bool confirmForgetNetworks_ = false;
};
