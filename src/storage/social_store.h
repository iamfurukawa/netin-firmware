#pragma once

#include <Arduino.h>

class SocialStore {
  public:
    static constexpr uint8_t kMaxRememberedEvents = 16;

    bool contains(const String &eventId) const;
    bool remember(const String &eventId) const;
};
