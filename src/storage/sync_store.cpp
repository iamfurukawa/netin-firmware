#include "sync_store.h"

#include <Preferences.h>

namespace {
constexpr char kNamespace[] = "sync-v1";

String keyFor(const char *prefix, uint8_t index) {
    return String(prefix) + index;
}

void removeAt(Preferences &prefs, uint8_t index) {
    prefs.remove(keyFor("id", index).c_str());
    prefs.remove(keyFor("st", index).c_str());
    prefs.remove(keyFor("ver", index).c_str());
    prefs.remove(keyFor("at", index).c_str());
}
}

uint8_t SyncStore::count() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return 0;
    const uint8_t value = prefs.getUChar("count", 0);
    prefs.end();
    return value <= kMaxPendingEvents ? value : 0;
}

bool SyncStore::enqueue(const PendingStatusEvent &event) {
    if (event.eventId.isEmpty() || !isValidStatus(static_cast<uint8_t>(event.status))) return false;
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;
    const uint8_t current = prefs.getUChar("count", 0);
    if (current >= kMaxPendingEvents) {
        prefs.end();
        return false;
    }
    const bool ok = prefs.putString(keyFor("id", current).c_str(), event.eventId) > 0 &&
                    prefs.putUChar(keyFor("st", current).c_str(), static_cast<uint8_t>(event.status)) == sizeof(uint8_t) &&
                    prefs.putUInt(keyFor("ver", current).c_str(), event.deviceVersion) == sizeof(uint32_t) &&
                    prefs.putULong64(keyFor("at", current).c_str(), event.createdAt) == sizeof(uint64_t) &&
                    prefs.putUChar("count", current + 1) == sizeof(uint8_t);
    prefs.end();
    return ok;
}

bool SyncStore::first(PendingStatusEvent &event) const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return false;
    const uint8_t current = prefs.getUChar("count", 0);
    if (current == 0 || current > kMaxPendingEvents) {
        prefs.end();
        return false;
    }
    const uint8_t status = prefs.getUChar("st0", 0xFF);
    event.eventId = prefs.getString("id0", "");
    event.status = static_cast<PresenceStatus>(status);
    event.deviceVersion = prefs.getUInt("ver0", 0);
    event.createdAt = prefs.getULong64("at0", 0);
    prefs.end();
    return !event.eventId.isEmpty() && isValidStatus(status);
}

bool SyncStore::acknowledge(const String &eventId) {
    PendingStatusEvent current;
    if (!first(current) || current.eventId != eventId) return false;
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;
    const uint8_t count = prefs.getUChar("count", 0);
    for (uint8_t index = 1; index < count; ++index) {
        prefs.putString(keyFor("id", index - 1).c_str(), prefs.getString(keyFor("id", index).c_str(), ""));
        prefs.putUChar(keyFor("st", index - 1).c_str(), prefs.getUChar(keyFor("st", index).c_str(), 0));
        prefs.putUInt(keyFor("ver", index - 1).c_str(), prefs.getUInt(keyFor("ver", index).c_str(), 0));
        prefs.putULong64(keyFor("at", index - 1).c_str(), prefs.getULong64(keyFor("at", index).c_str(), 0));
    }
    removeAt(prefs, count - 1);
    const bool ok = prefs.putUChar("count", count - 1) == sizeof(uint8_t);
    prefs.end();
    return ok;
}

uint32_t SyncStore::nextDeviceVersion() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return 0;
    const uint32_t next = prefs.getUInt("version", 0) + 1;
    prefs.putUInt("version", next);
    prefs.end();
    return next;
}
