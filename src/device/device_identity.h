#pragma once

#include <Arduino.h>

struct DeviceIdentity {
    String id;
    String bootstrapSecret;
};

class DeviceIdentityStore {
  public:
    DeviceIdentity loadOrCreate() const;
};
