#pragma once

#include <Arduino.h>

enum class SdCardState : uint8_t { Unavailable, Ready, Error };

class SdCardManager {
  public:
    bool begin();
    bool runDiagnostic();
    SdCardState state() const { return state_; }
    uint64_t totalBytes() const { return totalBytes_; }
    uint64_t usedBytes() const { return usedBytes_; }
    const String &detail() const { return detail_; }

  private:
    void updateUsage();

    SdCardState state_ = SdCardState::Unavailable;
    uint64_t totalBytes_ = 0;
    uint64_t usedBytes_ = 0;
    String detail_ = "Nao iniciado";
};
