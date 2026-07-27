#include "touch_input.h"

namespace {
constexpr uint16_t kThreshold = 600;
constexpr uint16_t kMaxTapMovement = 12;
constexpr uint16_t kMinSwipeMovement = 28;
constexpr uint32_t kMaxTapDurationMs = 700;
constexpr uint32_t kTapLockoutMs = 180;
}

TouchEvent TouchInput::poll() {
    uint16_t x = 0;
    uint16_t y = 0;
    const bool touching = tft_.getTouch(&x, &y, kThreshold);

    if (touching && !pressed_) {
        pressed_ = true;
        downX_ = x;
        downY_ = y;
        lastX_ = x;
        lastY_ = y;
        downAt_ = millis();
        return {};
    }

    if (touching) {
        lastX_ = x;
        lastY_ = y;
        return {};
    }

    if (!pressed_) return {};

    pressed_ = false;
    const uint32_t now = millis();
    const uint16_t dx = lastX_ > downX_ ? lastX_ - downX_ : downX_ - lastX_;
    const uint16_t dy = lastY_ > downY_ ? lastY_ - downY_ : downY_ - lastY_;
    if (now < lockUntil_ || now - downAt_ > kMaxTapDurationMs) return {};

    if (dy >= kMinSwipeMovement && dy > dx) {
        lockUntil_ = now + kTapLockoutMs;
        TouchEvent event;
        event.type = lastY_ < downY_ ? TouchEventType::SwipeUp : TouchEventType::SwipeDown;
        event.x = lastX_;
        event.y = lastY_;
        return event;
    }

    if (dx > kMaxTapMovement || dy > kMaxTapMovement) return {};

    lockUntil_ = now + kTapLockoutMs;
    TouchEvent event;
    event.type = TouchEventType::Tap;
    event.x = downX_;
    event.y = downY_;
    return event;
}
