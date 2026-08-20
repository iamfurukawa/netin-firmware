#pragma once

#include <Arduino.h>
#include <mqtt_client.h>

#include "app/app_types.h"
#include "device/device_identity.h"
#include "network/network_manager.h"
#include "pairing/pairing_manager.h"
#include "storage/settings_store.h"
#include "storage/social_store.h"
#include "media/media_manager.h"

enum class SyncState : uint8_t { Offline, Connecting, Connected, Revoked };

struct SocialInteraction {
    String eventId;
    String senderName;
    String type;
    String content;
};

class SyncManager {
  public:
    SyncManager(const DeviceIdentity &identity, NetworkManager &network, PairingManager &pairing, SettingsStore &settingsStore, UserSettings &settings, MediaManager &media)
        : identity_(identity), network_(network), pairing_(pairing), settingsStore_(settingsStore), settings_(settings), media_(media) {}

    void begin();
    void tick();
    void stop();
    SyncState state() const { return state_; }
    const SocialInteraction &interaction() const { return interaction_; }
    uint32_t interactionRevision() const { return interactionRevision_; }
    bool dismissInteraction();
    bool mediaVisible() const { return mediaVisible_; }
    uint32_t mediaRevision() const { return mediaRevision_; }
    void dismissMedia();

  private:
    static esp_err_t mqttEvent(esp_mqtt_event_handle_t event);
    void handleEvent(esp_mqtt_event_handle_t event);
    void handleCommand(const char *payload, size_t length);
    void handleSocialEvent(const String &body);
    bool handleMediaEvent(const String &body, const String &reactionId = "", bool socialDelivery = false);
    void processPendingMedia();
    void publishHeartbeat();
    void publishSocialAcknowledgement(const String &eventId);
    void publishMediaResult(const String &eventId, bool success, const char *code = nullptr);
    void connect();

    struct PendingMedia {
        String eventId;
        String senderName;
        String url;
        String hash;
        String mimeType;
        String reactionId;
        size_t size = 0;
        bool socialDelivery = false;
    };

    const DeviceIdentity &identity_;
    NetworkManager &network_;
    PairingManager &pairing_;
    SettingsStore &settingsStore_;
    UserSettings &settings_;
    MediaManager &media_;
    esp_mqtt_client_handle_t client_ = nullptr;
    String mqttUsername_;
    String mqttPassword_;
    String commandTopic_;
    unsigned long lastHeartbeatAt_ = 0;
    SyncState state_ = SyncState::Offline;
    SocialStore socialStore_;
    SocialInteraction interaction_;
    uint32_t interactionRevision_ = 0;
    static constexpr uint8_t kInteractionQueueCapacity = 3;
    SocialInteraction interactionQueue_[kInteractionQueueCapacity];
    uint8_t interactionQueueCount_ = 0;
    static constexpr uint8_t kMediaQueueCapacity = 3;
    PendingMedia mediaQueue_[kMediaQueueCapacity];
    uint8_t mediaQueueCount_ = 0;
    bool mediaVisible_ = false;
    uint32_t mediaRevision_ = 0;
    String lastCompletedMediaEventId_;
    static SyncManager *instance_;
};
