#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "device/device_identity.h"

enum class NetworkState : uint8_t { Unconfigured, Connecting, Connected, Portal };

struct WifiProfile {
    String ssid;
    String password;
    uint32_t lastSuccess = 0;
};

class NetworkStore {
  public:
    static constexpr uint8_t kMaxProfiles = 5;
    bool hasProfiles() const;
    uint8_t loadProfiles(WifiProfile profiles[], uint8_t capacity) const;
    bool firstProfile(String &ssid, String &password) const;
    bool willReplaceProfile(const String &ssid) const;
    bool saveProfile(const String &ssid, const String &password) const;
    bool markSuccess(const String &ssid) const;
    bool clearProfiles() const;
};

class NetworkManager {
  public:
    NetworkManager(NetworkStore &store, const DeviceIdentity &identity);
    void begin();
    void tick();
    void startPortal();
    void forgetNetworks();
    NetworkState state() const { return state_; }
    const String &portalSsid() const { return portalSsid_; }
    const String &portalPassword() const { return portalPassword_; }
    bool portalConnectionFailed() const { return portalConnectionFailed_; }
    String connectedSsid() const;
    bool timeReady() const { return timeReady_; }

  private:
    void startSavedNetwork();
    void tryNextSavedNetwork();
    void stopPortal();
    void configurePortalRoutes();
    String portalPage() const;

    NetworkStore &store_;
    const DeviceIdentity &identity_;
    WebServer server_{80};
    DNSServer dns_;
    NetworkState state_ = NetworkState::Unconfigured;
    String portalSsid_;
    String portalPassword_;
    String pendingSsid_;
    String pendingPassword_;
    unsigned long connectStartedAt_ = 0;
    unsigned long portalStartedAt_ = 0;
    unsigned long retryAt_ = 0;
    unsigned long retryDelayMs_ = 5000;
    WifiProfile savedProfiles_[NetworkStore::kMaxProfiles];
    uint8_t savedProfileCount_ = 0;
    uint8_t savedProfileIndex_ = 0;
    bool portalRunning_ = false;
    bool portalConnectionFailed_ = false;
    bool timeSyncStarted_ = false;
    bool timeReady_ = false;
};
