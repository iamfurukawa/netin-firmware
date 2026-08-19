#include "settings_store.h"

#include <Preferences.h>

namespace {
constexpr char kNamespace[] = "netin";
}

UserSettings SettingsStore::load() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return {};

    const uint8_t schema = prefs.getUChar("schema", 0);
    const uint8_t status = prefs.getUChar("status", 0xFF);
    const uint8_t theme = prefs.getUChar("theme", 0xFF);
    prefs.end();

    if (schema != kSettingsSchemaVersion || !isValidStatus(status) || !isValidTheme(theme)) return {};

    UserSettings settings;
    settings.schemaVersion = schema;
    settings.status = static_cast<PresenceStatus>(status);
    settings.theme = static_cast<Theme>(theme);
    return settings;
}

bool SettingsStore::save(const UserSettings &settings) const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;

    const bool ok = prefs.putUChar("schema", kSettingsSchemaVersion) == sizeof(uint8_t) &&
                    prefs.putUChar("status", static_cast<uint8_t>(settings.status)) == sizeof(uint8_t) &&
                    prefs.putUChar("theme", static_cast<uint8_t>(settings.theme)) == sizeof(uint8_t);
    prefs.end();
    return ok;
}
