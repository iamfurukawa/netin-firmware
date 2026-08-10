#pragma once

#include <Arduino.h>

#include "device/device_identity.h"
#include "network/network_manager.h"

enum class PairingState : uint8_t { Unpaired, Preparing, CodeReady, Paired, Offline, Error };

class PairingStore {
  public:
    String loadCredential() const;
    bool saveCredential(const String &credential) const;
    bool clearCredential() const;
};

class PairingManager {
  public:
    PairingManager(const DeviceIdentity &identity, NetworkManager &network, PairingStore &store)
        : identity_(identity), network_(network), store_(store) {}

    void begin();
    void tick();
    void requestCode();
    void invalidateCredential();
    PairingState state() const { return state_; }
    const String &code() const { return code_; }
    String credential() const { return store_.loadCredential(); }

  private:
    bool post(const char *path, String &response, int &statusCode);
    bool registerDevice(bool &paired);
    bool requestPairingCode();
    bool checkPairingStatus(bool &paired);
    bool requestCredential();
    void setError(const char *message);

    const DeviceIdentity &identity_;
    NetworkManager &network_;
    PairingStore &store_;
    PairingState state_ = PairingState::Unpaired;
    String code_;
    String error_;
    bool requestPending_ = false;
    unsigned long codeExpiresAt_ = 0;
    unsigned long lastStatusPollAt_ = 0;
};
