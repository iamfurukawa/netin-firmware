#include "social_store.h"

#include <Preferences.h>

namespace {
constexpr char kNamespace[] = "social-v1";

String keyFor(uint8_t index) {
    return String("id") + index;
}
}

bool SocialStore::contains(const String &eventId) const {
    if (eventId.isEmpty()) return false;
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return false;
    const uint8_t count = prefs.getUChar("count", 0);
    bool found = false;
    for (uint8_t index = 0; index < count && index < kMaxRememberedEvents; ++index) {
        if (prefs.getString(keyFor(index).c_str(), "") == eventId) {
            found = true;
            break;
        }
    }
    prefs.end();
    return found;
}

bool SocialStore::remember(const String &eventId) const {
    if (eventId.isEmpty()) return false;
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;
    uint8_t count = prefs.getUChar("count", 0);
    if (count > kMaxRememberedEvents) count = 0;
    for (uint8_t index = 0; index < count; ++index) {
        if (prefs.getString(keyFor(index).c_str(), "") == eventId) {
            prefs.end();
            return true;
        }
    }
    const uint8_t last = count < kMaxRememberedEvents ? count : kMaxRememberedEvents - 1;
    for (uint8_t index = last; index > 0; --index) {
        prefs.putString(keyFor(index).c_str(), prefs.getString(keyFor(index - 1).c_str(), ""));
    }
    const bool saved = prefs.putString(keyFor(0).c_str(), eventId) > 0;
    if (count < kMaxRememberedEvents) prefs.putUChar("count", count + 1);
    prefs.end();
    return saved;
}
