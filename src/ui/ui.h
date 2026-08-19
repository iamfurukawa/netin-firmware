#pragma once

#include "app/app_types.h"
#include "device/device_identity.h"
#include "display/netin_display.h"
#include "input/touch_input.h"
#include "network/network_manager.h"
#include "pairing/pairing_manager.h"
#include "storage/sd_card.h"

class SyncManager;

class Ui {
  public:
    Ui(NetinDisplay &display, UserSettings &settings, NetworkManager &network, PairingManager &pairing, SyncManager &sync, const DeviceIdentity &identity, SdCardManager &sdCard)
        : display_(display), settings_(settings), network_(network), pairing_(pairing), sync_(sync), identity_(identity), sdCard_(sdCard) {}
    void draw();
    void tick();
    void handle(const TouchEvent &event);

  private:
    enum class Screen : uint8_t { Home, Settings, Network, Device, Interaction, MediaReceived };
    bool hit(uint16_t x, uint16_t y, int16_t rx, int16_t ry, int16_t rw, int16_t rh) const;
    void drawHome();
    void drawSettings();
    void drawNetwork();
    void drawDevice();
    void drawInteraction();

    NetinDisplay &display_;
    UserSettings &settings_;
    NetworkManager &network_;
    PairingManager &pairing_;
    SyncManager &sync_;
    const DeviceIdentity &identity_;
    SdCardManager &sdCard_;
    Screen screen_ = Screen::Home;
    NetworkState lastNetworkState_ = NetworkState::Unconfigured;
    PairingState lastPairingState_ = PairingState::Unpaired;
    PresenceStatus lastStatus_ = PresenceStatus::Available;
    uint32_t lastInteractionRevision_ = 0;
    uint32_t lastMediaRevision_ = 0;
    uint32_t interactionOpenedAt_ = 0;
    uint32_t mediaOpenedAt_ = 0;
    bool confirmForgetNetworks_ = false;
};
