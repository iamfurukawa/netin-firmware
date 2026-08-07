#include "device_identity.h"

#include <Preferences.h>
#include <esp_system.h>

namespace {
constexpr char kNamespace[] = "identity";

String randomHex(size_t byteCount) {
    String value;
    value.reserve(byteCount * 2);
    for (size_t index = 0; index < byteCount; ++index) {
        const uint8_t byte = static_cast<uint8_t>(esp_random());
        const char hex[] = "0123456789abcdef";
        value += hex[byte >> 4];
        value += hex[byte & 0x0F];
    }
    return value;
}

String randomUuid() {
    const String hex = randomHex(16);
    return hex.substring(0, 8) + "-" + hex.substring(8, 12) + "-" +
           hex.substring(12, 16) + "-" + hex.substring(16, 20) + "-" +
           hex.substring(20, 32);
}
}

DeviceIdentity DeviceIdentityStore::loadOrCreate() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return {};

    DeviceIdentity identity;
    identity.id = prefs.getString("id", "");
    identity.bootstrapSecret = prefs.getString("secret", "");
    if (identity.id.isEmpty() || identity.bootstrapSecret.length() < 32) {
        identity.id = randomUuid();
        identity.bootstrapSecret = randomHex(32);
        prefs.putString("id", identity.id);
        prefs.putString("secret", identity.bootstrapSecret);
    }
    prefs.end();
    return identity;
}
