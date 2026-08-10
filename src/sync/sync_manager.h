#pragma once

#include <Arduino.h>
#include <mqtt_client.h>

#include "app/app_types.h"
#include "device/device_identity.h"
#include "network/network_manager.h"
#include "pairing/pairing_manager.h"
#include "storage/settings_store.h"
#include "storage/sync_store.h"

enum class SyncState : uint8_t { Offline, Connecting, Connected, Revoked };

class SyncManager {
  public:
    SyncManager(const DeviceIdentity &identity, NetworkManager &network, PairingManager &pairing, SettingsStore &settingsStore, UserSettings &settings)
        : identity_(identity), network_(network), pairing_(pairing), settingsStore_(settingsStore), settings_(settings) {}

    void begin();
    void tick();
    void stop();
    bool enqueueStatus(PresenceStatus status);
    SyncState state() const { return state_; }
    uint8_t pendingCount() const { return store_.count(); }

  private:
    static esp_err_t mqttEvent(esp_mqtt_event_handle_t event);
    void handleEvent(esp_mqtt_event_handle_t event);
    void handleCommand(const char *payload, size_t length);
    void handleAcknowledgement(const char *payload, size_t length);
    void publishNextPending();
    void publishHeartbeat();
    void connect();

    const DeviceIdentity &identity_;
    NetworkManager &network_;
    PairingManager &pairing_;
    SettingsStore &settingsStore_;
    UserSettings &settings_;
    esp_mqtt_client_handle_t client_ = nullptr;
    String mqttUsername_;
    String mqttPassword_;
    String commandTopic_;
    String acknowledgementTopic_;
    String inFlightEventId_;
    unsigned long lastPublishAt_ = 0;
    unsigned long lastHeartbeatAt_ = 0;
    SyncState state_ = SyncState::Offline;
    bool started_ = false;
    SyncStore store_;
    static SyncManager *instance_;
};
