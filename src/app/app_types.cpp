#include "app_types.h"

bool isValidStatus(uint8_t value) {
    return value < kStatusCount;
}

const char *statusLabel(PresenceStatus status) {
    switch (status) {
        case PresenceStatus::Available: return "Disponivel";
        case PresenceStatus::Busy: return "Ocupado";
        case PresenceStatus::Focused: return "Focado";
        case PresenceStatus::Away: return "Ausente";
        case PresenceStatus::Invisible: return "Invisivel";
        case PresenceStatus::InCall: return "Em chamada";
        case PresenceStatus::Gaming: return "Jogando";
        case PresenceStatus::Sleeping: return "Dormindo";
        case PresenceStatus::DoNotDisturb: return "Nao perturbe";
    }
    return "Disponivel";
}

const char *statusWireName(PresenceStatus status) {
    static constexpr const char *kNames[] = {"available", "busy", "focused", "away", "invisible", "in_call", "gaming", "sleeping", "do_not_disturb"};
    const uint8_t index = static_cast<uint8_t>(status);
    return index < kStatusCount ? kNames[index] : kNames[0];
}

bool statusFromWireName(const String &value, PresenceStatus &status) {
    for (uint8_t index = 0; index < kStatusCount; ++index) {
        if (value == statusWireName(static_cast<PresenceStatus>(index))) {
            status = static_cast<PresenceStatus>(index);
            return true;
        }
    }
    return false;
}
