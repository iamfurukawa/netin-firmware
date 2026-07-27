#pragma once

#include "app/app_types.h"
#include "display/netin_display.h"
#include "input/touch_input.h"
#include "storage/settings_store.h"

class Ui {
  public:
    Ui(NetinDisplay &display, SettingsStore &store, UserSettings &settings)
        : display_(display), store_(store), settings_(settings) {}
    void draw();
    void handle(const TouchEvent &event);

  private:
    enum class Screen : uint8_t { Home, StatusPicker, StatusConfirm, Settings };
    bool hit(uint16_t x, uint16_t y, int16_t rx, int16_t ry, int16_t rw, int16_t rh) const;
    void drawHome();
    void drawStatusPicker();
    void drawStatusConfirm();
    void drawSettings();
    void applyCandidate();

    NetinDisplay &display_;
    SettingsStore &store_;
    UserSettings &settings_;
    Screen screen_ = Screen::Home;
    PresenceStatus candidate_ = PresenceStatus::Available;
    int16_t pickerScroll_ = 0;
};
