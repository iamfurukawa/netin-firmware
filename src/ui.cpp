#include "ui.h"

namespace {
constexpr int16_t kStatusCardX = 16;
constexpr int16_t kStatusCardY = 20;
constexpr int16_t kStatusCardW = 208;
constexpr int16_t kStatusCardH = 216;
constexpr int16_t kSettingsButtonX = 96;
constexpr int16_t kSettingsButtonY = 260;
constexpr int16_t kSettingsButtonSize = 48;
constexpr int16_t kPickerRowX = 12;
constexpr int16_t kPickerRowY = 10;
constexpr int16_t kPickerRowW = 216;
constexpr int16_t kPickerRowH = 48;
constexpr int16_t kPickerRowGap = 8;
constexpr int16_t kPickerRowStep = kPickerRowH + kPickerRowGap;
constexpr int16_t kPickerListBottom = 254;
constexpr int16_t kPickerVisibleHeight = kPickerListBottom - kPickerRowY;
constexpr int16_t kPickerContentHeight = kStatusCount * kPickerRowStep - kPickerRowGap;
constexpr int16_t kPickerMaxScroll = kPickerContentHeight - kPickerVisibleHeight;
constexpr int16_t kBackX = 12;
constexpr int16_t kBackY = 264;
constexpr int16_t kBackW = 100;
constexpr int16_t kBackH = 44;
constexpr int16_t kScrollUpX = 124;
constexpr int16_t kScrollDownX = 178;
constexpr int16_t kScrollButtonW = 50;
constexpr int16_t kConfirmX = 128;
constexpr int16_t kConfirmY = 264;
constexpr int16_t kConfirmW = 100;
constexpr int16_t kConfirmH = 44;
}

bool Ui::hit(uint16_t x, uint16_t y, int16_t rx, int16_t ry, int16_t rw, int16_t rh) const {
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

void Ui::draw() {
    switch (screen_) {
        case Screen::Home: drawHome(); break;
        case Screen::StatusPicker: drawStatusPicker(); break;
        case Screen::StatusConfirm: drawStatusConfirm(); break;
        case Screen::Settings: drawSettings(); break;
    }
}

void Ui::drawHome() {
    const Theme theme = settings_.theme;
    const uint16_t foreground = display_.foreground(theme);
    const uint16_t statusColor = display_.statusColor(settings_.status);
    display_.clear(theme);

    // The card is the visual focus; there is deliberately no header or footer.
    display_.button(kStatusCardX, kStatusCardY, kStatusCardW, kStatusCardH, "", statusColor, foreground);
    display_.statusIcon(120, 105, settings_.status, foreground, statusColor);
    const char *label = statusLabel(settings_.status);
    const int16_t labelX = 120 - static_cast<int16_t>(strlen(label) * 18) / 2;
    display_.text(labelX, 160, label, foreground, statusColor, 3);

    display_.button(kSettingsButtonX, kSettingsButtonY, kSettingsButtonSize, kSettingsButtonSize, "", display_.muted(theme), foreground);
    display_.settingsIcon(120, 284, foreground);
}

void Ui::drawStatusPicker() {
    const Theme theme = settings_.theme;
    const uint16_t foreground = display_.foreground(theme);
    display_.clear(theme);

    for (uint8_t index = 0; index < kStatusCount; ++index) {
        const auto status = static_cast<PresenceStatus>(index);
        const int16_t y = kPickerRowY + index * kPickerRowStep - pickerScroll_;
        if (y < kPickerRowY || y + kPickerRowH > kPickerListBottom) continue;
        const uint16_t color = display_.statusColor(status);
        display_.button(kPickerRowX, y, kPickerRowW, kPickerRowH, "", color, foreground);
        display_.statusIcon(38, y + 24, status, foreground, color);
        display_.text(68, y + 15, statusLabel(status), foreground, color, 2);
        if (status == settings_.status) {
            display_.fillCircle(204, y + 27, 6, foreground);
        }
    }
    display_.button(kBackX, kBackY, kBackW, kBackH, "Voltar", display_.muted(theme), foreground);
    const uint16_t upColor = pickerScroll_ > 0 ? display_.muted(theme) : display_.background(theme);
    const uint16_t downColor = pickerScroll_ < kPickerMaxScroll ? display_.muted(theme) : display_.background(theme);
    display_.button(kScrollUpX, kBackY, kScrollButtonW, kBackH, "^", upColor, foreground);
    display_.button(kScrollDownX, kBackY, kScrollButtonW, kBackH, "v", downColor, foreground);
}

void Ui::drawStatusConfirm() {
    const Theme theme = settings_.theme;
    const uint16_t background = display_.background(theme);
    const uint16_t foreground = display_.foreground(theme);
    const uint16_t color = display_.statusColor(candidate_);
    display_.clear(theme);
    display_.text(34, 48, "Usar este status?", foreground, background, 2);
    display_.button(24, 92, 192, 116, "", color, foreground);
    display_.statusIcon(120, 130, candidate_, foreground, color);
    const char *label = statusLabel(candidate_);
    display_.text(120 - static_cast<int16_t>(strlen(label) * 12) / 2, 170, label, foreground, color, 2);
    display_.button(kBackX, kBackY, kBackW, kBackH, "Voltar", display_.muted(theme), foreground);
    display_.button(kConfirmX, kConfirmY, kConfirmW, kConfirmH, "Aplicar", color, foreground);
}

void Ui::drawSettings() {
    const Theme theme = settings_.theme;
    const uint16_t background = display_.background(theme);
    const uint16_t foreground = display_.foreground(theme);
    display_.clear(theme);
    display_.text(22, 30, "Ajustes", foreground, background, 3);
    display_.button(20, 96, 200, 56, theme == Theme::Dark ? "Tema claro" : "Tema escuro", display_.muted(theme), foreground);
    display_.button(kBackX, kBackY, kBackW, kBackH, "Voltar", display_.muted(theme), foreground);
}

void Ui::applyCandidate() {
    if (candidate_ != settings_.status) ++settings_.statusChangeCount;
    settings_.status = candidate_;
    store_.save(settings_);
    screen_ = Screen::Home;
    draw();
}

void Ui::handle(const TouchEvent &event) {
    if (event.type != TouchEventType::Tap) return;

    if (screen_ == Screen::Home) {
        if (hit(event.x, event.y, kSettingsButtonX, kSettingsButtonY, kSettingsButtonSize, kSettingsButtonSize)) {
            screen_ = Screen::Settings;
            draw();
        } else if (hit(event.x, event.y, kStatusCardX, kStatusCardY, kStatusCardW, kStatusCardH)) {
            screen_ = Screen::StatusPicker;
            pickerScroll_ = 0;
            draw();
        }
        return;
    }

    if (screen_ == Screen::StatusPicker) {
        if (hit(event.x, event.y, kBackX, kBackY, kBackW, kBackH)) {
            screen_ = Screen::Home;
            draw();
            return;
        }
        if (hit(event.x, event.y, kScrollUpX, kBackY, kScrollButtonW, kBackH) && pickerScroll_ > 0) {
            pickerScroll_ -= kPickerRowStep;
            if (pickerScroll_ < 0) pickerScroll_ = 0;
            draw();
            return;
        }
        if (hit(event.x, event.y, kScrollDownX, kBackY, kScrollButtonW, kBackH) && pickerScroll_ < kPickerMaxScroll) {
            pickerScroll_ += kPickerRowStep;
            if (pickerScroll_ > kPickerMaxScroll) pickerScroll_ = kPickerMaxScroll;
            draw();
            return;
        }
        for (uint8_t index = 0; index < kStatusCount; ++index) {
            const int16_t y = kPickerRowY + index * kPickerRowStep - pickerScroll_;
            if (y < kPickerRowY || y + kPickerRowH > kPickerListBottom) continue;
            if (hit(event.x, event.y, kPickerRowX, y, kPickerRowW, kPickerRowH)) {
                candidate_ = static_cast<PresenceStatus>(index);
                screen_ = Screen::StatusConfirm;
                draw();
                return;
            }
        }
        return;
    }

    if (screen_ == Screen::Settings) {
        if (hit(event.x, event.y, 20, 96, 200, 56)) {
            settings_.theme = settings_.theme == Theme::Dark ? Theme::Light : Theme::Dark;
            store_.save(settings_);
            draw();
        } else if (hit(event.x, event.y, kBackX, kBackY, kBackW, kBackH)) {
            screen_ = Screen::Home;
            draw();
        }
    } else if (hit(event.x, event.y, kBackX, kBackY, kBackW, kBackH)) {
        screen_ = Screen::StatusPicker;
        draw();
    } else if (hit(event.x, event.y, kConfirmX, kConfirmY, kConfirmW, kConfirmH)) {
        applyCandidate();
    }
}
