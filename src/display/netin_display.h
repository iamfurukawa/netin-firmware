#pragma once

#include <TFT_eSPI.h>

#include "app/app_types.h"

class NetinDisplay {
  public:
    explicit NetinDisplay(TFT_eSPI &tft) : tft_(tft) {}
    void begin();
    void clear(Theme theme);
    void text(int16_t x, int16_t y, const char *value, uint16_t fg, uint16_t bg, uint8_t size = 1);
    void button(int16_t x, int16_t y, int16_t w, int16_t h, const char *label, uint16_t fill, uint16_t fg);
    void fillCircle(int16_t x, int16_t y, int16_t radius, uint16_t color);
    void heart(int16_t x, int16_t y, int16_t size, uint16_t color);
    void reactionIcon(int16_t x, int16_t y, int16_t size, const char *reaction, uint16_t color);
    void backIcon(int16_t x, int16_t y, uint16_t color);
    void menuIcon(int16_t x, int16_t y, uint16_t color);
    void settingsIcon(int16_t x, int16_t y, uint16_t color);
    void statusIcon(int16_t x, int16_t y, PresenceStatus status, uint16_t color, uint16_t background);
    uint16_t background(Theme theme) const;
    uint16_t foreground(Theme theme) const;
    uint16_t muted(Theme theme) const;
    uint16_t statusColor(PresenceStatus status) const;

  private:
    static constexpr uint16_t panelColor(uint16_t color) { return static_cast<uint16_t>(~color); }
    TFT_eSPI &tft_;
};
