#include "sync_manager.h"

#include <cstring>
#include <time.h>

namespace {
constexpr char kBrokerUri[] = "wss://netin-mqtt.13997906387.xyz/mqtt";
constexpr unsigned long kHeartbeatIntervalMs = 60000;

// Google Trust Services GTS Root R4, used by the Cloudflare certificate chain.
constexpr char kBrokerRootCertificate[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AATzdHOna
ItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyiQHY7qca4R9
gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvRHYqjQjBAMA4
GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBSATNbrdP9J
NqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D9J+uHXqnLrmvT
/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8p/SgguMh1YQdc
4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD
-----END CERTIFICATE-----
)EOF";

String jsonString(const String &json, const char *key) {
    const String marker = String("\"") + key + "\":\"";
    const int start = json.indexOf(marker);
    if (start < 0) return "";
    const int valueStart = start + marker.length();
    const int valueEnd = json.indexOf('"', valueStart);
    return valueEnd < 0 ? "" : json.substring(valueStart, valueEnd);
}

String randomUuid() {
    const uint32_t first = esp_random();
    const uint16_t second = static_cast<uint16_t>(esp_random());
    const uint16_t third = static_cast<uint16_t>((esp_random() & 0x0FFF) | 0x4000);
    const uint16_t fourth = static_cast<uint16_t>((esp_random() & 0x3FFF) | 0x8000);
    const uint32_t fifthA = esp_random();
    const uint16_t fifthB = static_cast<uint16_t>(esp_random());
    char value[37];
    snprintf(value, sizeof(value), "%08lx-%04x-%04x-%04x-%08lx%04x", static_cast<unsigned long>(first), second, third, fourth, static_cast<unsigned long>(fifthA), fifthB);
    return String(value);
}

String formatUtc(uint64_t epoch) {
    time_t value = static_cast<time_t>(epoch);
    tm timeInfo = {};
    gmtime_r(&value, &timeInfo);
    char timestamp[25];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeInfo);
    return String(timestamp);
}

String reactionLabel(const String &reaction) {
    if (reaction == "👍") return "Gostei";
    if (reaction == "❤️") return "Coracao";
    if (reaction == "😂") return "Risada";
    if (reaction == "🎉") return "Festa";
    if (reaction == "👋") return "Ola";
    if (reaction == "👏") return "Palmas";
    if (reaction == "🔥") return "Fogo";
    if (reaction == "✨") return "Brilho";
    return "Reacao";
}
}

SyncManager *SyncManager::instance_ = nullptr;

void SyncManager::begin() {
    instance_ = this;
}

void SyncManager::tick() {
    if (pairing_.state() != PairingState::Paired || network_.state() != NetworkState::Connected || !network_.timeReady()) {
        if (client_) stop();
        return;
    }
    if (!client_) connect();
    if (state_ == SyncState::Connected) {
        publishHeartbeat();
        publishNextPending();
        processPendingMedia();
    }
}

void SyncManager::stop() {
    if (!client_) return;
    esp_mqtt_client_stop(client_);
    esp_mqtt_client_destroy(client_);
    client_ = nullptr;
    started_ = false;
    if (state_ != SyncState::Revoked) state_ = SyncState::Offline;
}

void SyncManager::connect() {
    mqttPassword_ = pairing_.credential();
    if (mqttPassword_.isEmpty()) return;
    mqttUsername_ = String("device-") + identity_.id;
    commandTopic_ = String("netin/v1/devices/") + identity_.id + "/commands";
    acknowledgementTopic_ = String("netin/v1/devices/") + identity_.id + "/ack";

    esp_mqtt_client_config_t config = {};
    config.uri = kBrokerUri;
    config.client_id = identity_.id.c_str();
    config.username = mqttUsername_.c_str();
    config.password = mqttPassword_.c_str();
    config.cert_pem = kBrokerRootCertificate;
    config.keepalive = 30;
    config.buffer_size = 768;
    config.reconnect_timeout_ms = 5000;
    config.event_handle = mqttEvent;

    client_ = esp_mqtt_client_init(&config);
    if (!client_) return;
    state_ = SyncState::Connecting;
    if (esp_mqtt_client_start(client_) != ESP_OK) stop();
    else started_ = true;
}

esp_err_t SyncManager::mqttEvent(esp_mqtt_event_handle_t event) {
    if (instance_) instance_->handleEvent(event);
    return ESP_OK;
}

void SyncManager::handleEvent(esp_mqtt_event_handle_t event) {
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            state_ = SyncState::Connected;
            lastHeartbeatAt_ = 0;
            esp_mqtt_client_subscribe(event->client, commandTopic_.c_str(), 1);
            esp_mqtt_client_subscribe(event->client, acknowledgementTopic_.c_str(), 1);
            break;
        case MQTT_EVENT_DATA:
            if (event->total_data_len != event->data_len || event->data_len <= 0 || !event->topic) break;
            if (String(event->topic, event->topic_len) == commandTopic_) handleCommand(event->data, event->data_len);
            else if (String(event->topic, event->topic_len) == acknowledgementTopic_) handleAcknowledgement(event->data, event->data_len);
            break;
        case MQTT_EVENT_ERROR:
            if (event->error_handle && event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED &&
                (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_BAD_USERNAME ||
                 event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED)) {
                pairing_.invalidateCredential();
                state_ = SyncState::Revoked;
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            if (state_ != SyncState::Revoked) state_ = SyncState::Connecting;
            break;
        default:
            break;
    }
}

void SyncManager::publishHeartbeat() {
    if (!client_ || (lastHeartbeatAt_ != 0 && millis() - lastHeartbeatAt_ < kHeartbeatIntervalMs)) return;
    const String topic = String("netin/v1/devices/") + identity_.id + "/events";
    const String payload = String("{\"protocolVersion\":1,\"type\":\"heartbeat\"}");
    if (esp_mqtt_client_publish(client_, topic.c_str(), payload.c_str(), 0, 0, 0) >= 0) lastHeartbeatAt_ = millis();
}

void SyncManager::handleCommand(const char *payload, size_t length) {
    const String body(payload, length);
    const String type = jsonString(body, "type");
    if (type == "status_sync") {
        PresenceStatus status;
        if (!statusFromWireName(jsonString(body, "status"), status)) return;
        settings_.status = status;
        settingsStore_.save(settings_);
    } else if (type == "social_event") {
        handleSocialEvent(body);
    } else if (type == "media_event") {
        handleMediaEvent(body);
    }
}

void SyncManager::handleMediaEvent(const String &body) {
    const String eventId = jsonString(body, "eventId");
    const String url = jsonString(body, "downloadUrl");
    const String hash = jsonString(body, "sha256");
    const String mimeType = jsonString(body, "kind");
    const String sizeMarker = "\"size\":";
    const int sizeStart = body.indexOf(sizeMarker);
    if (eventId.isEmpty() || url.isEmpty() || hash.length() != 64 || sizeStart < 0 || (mimeType != "image/jpeg" && mimeType != "image/gif")) return;
    const size_t size = static_cast<size_t>(body.substring(sizeStart + sizeMarker.length()).toInt());
    const size_t maximumSize = mimeType == "image/gif" ? 2 * 1024 * 1024 : 150 * 1024;
    if (!size || size > maximumSize) return;
    if (eventId == lastCompletedMediaEventId_) return;
    for (uint8_t index = 0; index < mediaQueueCount_; ++index) {
        if (mediaQueue_[index].eventId == eventId) return;
    }
    if (mediaQueueCount_ >= kMediaQueueCapacity) {
        publishMediaResult(eventId, false, "media_queue_full");
        return;
    }
    PendingMedia &next = mediaQueue_[mediaQueueCount_++];
    next.eventId = eventId;
    next.url = url;
    next.hash = hash;
    next.mimeType = mimeType;
    next.size = size;
}

void SyncManager::processPendingMedia() {
    if (mediaVisible_ || mediaQueueCount_ == 0) return;
    const PendingMedia current = mediaQueue_[0];
    const bool downloaded = media_.downloadMedia(current.url, identity_.id, pairing_.credential(), current.hash, current.size);
    const bool ok = downloaded && media_.showActiveMedia(current.mimeType);
    publishMediaResult(current.eventId, ok, ok ? nullptr : "download_failed");
    for (uint8_t index = 1; index < mediaQueueCount_; ++index) mediaQueue_[index - 1] = mediaQueue_[index];
    --mediaQueueCount_;
    if (ok) {
        lastCompletedMediaEventId_ = current.eventId;
        mediaVisible_ = true;
        ++mediaRevision_;
    }
}

void SyncManager::dismissMedia() {
    if (!mediaVisible_) return;
    mediaVisible_ = false;
    media_.closeActiveMedia();
    ++mediaRevision_;
}

void SyncManager::publishMediaResult(const String &eventId, bool success, const char *code) {
    if (!client_) return;
    const String topic = String("netin/v1/devices/") + identity_.id + "/events";
    String payload = String("{\"protocolVersion\":1,\"type\":\"") + (success ? "media_ack" : "media_failed") + "\",\"eventId\":\"" + eventId + "\"";
    if (!success) payload += String(",\"code\":\"") + (code ? code : "download_failed") + "\"";
    payload += "}";
    esp_mqtt_client_publish(client_, topic.c_str(), payload.c_str(), 0, 1, 0);
}

void SyncManager::handleSocialEvent(const String &body) {
    const String eventId = jsonString(body, "eventId");
    const String interactionType = jsonString(body, "interactionType");
    if (eventId.isEmpty() || interactionType.isEmpty()) return;

    if (!socialStore_.contains(eventId)) {
        SocialInteraction next;
        next.eventId = eventId;
        next.senderName = jsonString(body, "name");
        if (next.senderName.isEmpty()) next.senderName = "Alguem";
        next.senderName = next.senderName.substring(0, 24);
        next.type = interactionType;
        if (interactionType == "message") next.content = jsonString(body, "text").substring(0, 80);
        else if (interactionType == "reaction") next.content = reactionLabel(jsonString(body, "reaction"));
        else if (interactionType == "poke") next.content = "Cutucou voce";
        else return;
        if (next.content.isEmpty()) return;
        if (!socialStore_.remember(eventId)) return;
        interaction_ = next;
        ++interactionRevision_;
    }
    publishSocialAcknowledgement(eventId);
}

void SyncManager::dismissInteraction() {
    interaction_ = SocialInteraction{};
}

bool SyncManager::enqueueStatus(PresenceStatus status) {
    PendingStatusEvent event;
    event.eventId = randomUuid();
    event.status = status;
    event.deviceVersion = store_.nextDeviceVersion();
    event.createdAt = static_cast<uint64_t>(time(nullptr));
    return store_.enqueue(event);
}

void SyncManager::publishNextPending() {
    if (!client_) return;
    if (lastPublishAt_ != 0 && millis() - lastPublishAt_ < 5000) return;
    PendingStatusEvent event;
    if (!store_.first(event)) return;
    const String topic = String("netin/v1/devices/") + identity_.id + "/events";
    const String payload = String("{\"protocolVersion\":1,\"eventId\":\"") + event.eventId +
        "\",\"type\":\"status_changed\",\"status\":\"" + statusWireName(event.status) +
        "\",\"createdAt\":\"" + formatUtc(event.createdAt) + "\",\"deviceVersion\":" + event.deviceVersion + "}";
    if (esp_mqtt_client_publish(client_, topic.c_str(), payload.c_str(), 0, 1, 0) >= 0) {
        inFlightEventId_ = event.eventId;
        lastPublishAt_ = millis();
    }
}

void SyncManager::handleAcknowledgement(const char *payload, size_t length) {
    const String body(payload, length);
    if (jsonString(body, "type") != "status_ack") return;
    const String eventId = jsonString(body, "eventId");
    if (eventId.isEmpty() || eventId != inFlightEventId_) return;
    if (store_.acknowledge(eventId)) {
        inFlightEventId_ = "";
        lastPublishAt_ = 0;
    }
}

void SyncManager::publishSocialAcknowledgement(const String &eventId) {
    if (!client_ || eventId.isEmpty()) return;
    const String topic = String("netin/v1/devices/") + identity_.id + "/events";
    const String payload = String("{\"protocolVersion\":1,\"type\":\"social_ack\",\"eventId\":\"") + eventId + "\"}";
    esp_mqtt_client_publish(client_, topic.c_str(), payload.c_str(), 0, 1, 0);
}
