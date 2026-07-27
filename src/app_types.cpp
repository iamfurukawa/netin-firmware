#include "app_types.h"

bool isValidStatus(uint8_t value) {
    return value < kStatusCount;
}

bool isValidTheme(uint8_t value) {
    return value <= static_cast<uint8_t>(Theme::Light);
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
