#pragma once

#include "app_types.h"

class SettingsStore {
  public:
    UserSettings load() const;
    bool save(const UserSettings &settings) const;
};
