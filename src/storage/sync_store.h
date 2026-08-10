#pragma once

#include <Arduino.h>

#include "app/app_types.h"

struct PendingStatusEvent {
    String eventId;
    PresenceStatus status = PresenceStatus::Available;
    uint32_t deviceVersion = 0;
    uint64_t createdAt = 0;
};

class SyncStore {
  public:
    static constexpr uint8_t kMaxPendingEvents = 20;

    uint8_t count() const;
    bool enqueue(const PendingStatusEvent &event);
    bool first(PendingStatusEvent &event) const;
    bool acknowledge(const String &eventId);
    uint32_t nextDeviceVersion() const;
};
