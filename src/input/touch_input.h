#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

enum class TouchEventType : uint8_t { None, Tap, SwipeUp, SwipeDown };

struct TouchEvent {
    TouchEventType type = TouchEventType::None;
    uint16_t x = 0;
    uint16_t y = 0;
};

class TouchInput {
  public:
    explicit TouchInput(TFT_eSPI &tft) : tft_(tft) {}
    TouchEvent poll();

  private:
    TFT_eSPI &tft_;
    bool pressed_ = false;
    uint16_t downX_ = 0;
    uint16_t downY_ = 0;
    uint16_t lastX_ = 0;
    uint16_t lastY_ = 0;
    uint32_t downAt_ = 0;
    uint32_t lockUntil_ = 0;
};
