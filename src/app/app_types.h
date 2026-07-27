#pragma once

#include <Arduino.h>

enum class PresenceStatus : uint8_t {
    Available = 0,
    Busy,
    Focused,
    Away,
    Invisible,
    InCall,
    Gaming,
    Sleeping,
    DoNotDisturb,
};

enum class Theme : uint8_t { Dark = 0, Light };

struct UserSettings {
    uint8_t schemaVersion = 1;
    PresenceStatus status = PresenceStatus::Available;
    Theme theme = Theme::Dark;
    uint32_t statusChangeCount = 0;
};

constexpr uint8_t kSettingsSchemaVersion = 1;
constexpr uint8_t kStatusCount = static_cast<uint8_t>(PresenceStatus::DoNotDisturb) + 1;

bool isValidStatus(uint8_t value);
bool isValidTheme(uint8_t value);
const char *statusLabel(PresenceStatus status);
