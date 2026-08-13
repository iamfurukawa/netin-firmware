#include "netin_display.h"

#include <cstring>

void NetinDisplay::begin() {
    tft_.init();
    tft_.setRotation(0);
    tft_.setTextWrap(false, false);
}

uint16_t NetinDisplay::background(Theme theme) const { return theme == Theme::Dark ? TFT_BLACK : TFT_WHITE; }
uint16_t NetinDisplay::foreground(Theme theme) const { return theme == Theme::Dark ? TFT_WHITE : TFT_BLACK; }
uint16_t NetinDisplay::muted(Theme theme) const { return theme == Theme::Dark ? TFT_DARKGREY : TFT_LIGHTGREY; }

uint16_t NetinDisplay::statusColor(PresenceStatus status) const {
    switch (status) {
        case PresenceStatus::Available: return TFT_GREEN;
        case PresenceStatus::Busy: return TFT_RED;
        case PresenceStatus::Focused: return TFT_MAGENTA;
        case PresenceStatus::Away: return TFT_ORANGE;
        case PresenceStatus::Invisible: return TFT_LIGHTGREY;
        case PresenceStatus::InCall: return TFT_CYAN;
        case PresenceStatus::Gaming: return TFT_BLUE;
        case PresenceStatus::Sleeping: return TFT_DARKGREEN;
        case PresenceStatus::DoNotDisturb: return TFT_DARKGREY;
    }
    return TFT_GREEN;
}

void NetinDisplay::clear(Theme theme) { tft_.fillScreen(panelColor(background(theme))); }

void NetinDisplay::text(int16_t x, int16_t y, const char *value, uint16_t fg, uint16_t bg, uint8_t size) {
    tft_.setTextColor(panelColor(fg), panelColor(bg));
    tft_.setTextSize(size);
    tft_.setCursor(x, y);
    tft_.print(value);
}

void NetinDisplay::button(int16_t x, int16_t y, int16_t w, int16_t h, const char *label, uint16_t fill, uint16_t fg) {
    tft_.fillRoundRect(x, y, w, h, 10, panelColor(fill));
    tft_.drawRoundRect(x, y, w, h, 10, panelColor(fg));
    const int16_t labelWidth = static_cast<int16_t>(strlen(label) * 12);
    text(x + (w - labelWidth) / 2, y + 17, label, fg, fill, 2);
}

void NetinDisplay::fillCircle(int16_t x, int16_t y, int16_t radius, uint16_t color) {
    tft_.fillCircle(x, y, radius, panelColor(color));
}

void NetinDisplay::heart(int16_t x, int16_t y, int16_t size, uint16_t color) {
    const uint16_t c = panelColor(color);
    const int16_t radius = size / 4;
    tft_.fillCircle(x - radius, y - radius / 2, radius, c);
    tft_.fillCircle(x + radius, y - radius / 2, radius, c);
    tft_.fillTriangle(x - size / 2, y - radius / 2, x + size / 2, y - radius / 2, x, y + size / 2, c);
}

void NetinDisplay::reactionIcon(int16_t x, int16_t y, int16_t size, const char *reaction, uint16_t color) {
    const uint16_t c = panelColor(color);
    if (strcmp(reaction, "Coracao") == 0) {
        heart(x, y, size, color);
    } else if (strcmp(reaction, "Risada") == 0) {
        tft_.fillCircle(x, y, size / 2, c);
        tft_.fillCircle(x - size / 6, y - size / 8, size / 18, panelColor(TFT_BLACK));
        tft_.fillCircle(x + size / 6, y - size / 8, size / 18, panelColor(TFT_BLACK));
        tft_.drawCircle(x, y + size / 10, size / 4, panelColor(TFT_BLACK));
    } else if (strcmp(reaction, "Fogo") == 0) {
        tft_.fillTriangle(x, y - size / 2, x - size / 3, y + size / 2, x + size / 3, y + size / 2, c);
        tft_.fillCircle(x, y + size / 4, size / 3, c);
    } else if (strcmp(reaction, "Brilho") == 0) {
        tft_.fillTriangle(x, y - size / 2, x - size / 7, y, x + size / 7, y, c);
        tft_.fillTriangle(x, y + size / 2, x - size / 7, y, x + size / 7, y, c);
        tft_.fillTriangle(x - size / 2, y, x, y - size / 7, x, y + size / 7, c);
        tft_.fillTriangle(x + size / 2, y, x, y - size / 7, x, y + size / 7, c);
    } else if (strcmp(reaction, "Festa") == 0) {
        tft_.fillCircle(x - size / 4, y - size / 4, size / 9, c);
        tft_.fillCircle(x + size / 4, y - size / 5, size / 9, c);
        tft_.fillCircle(x, y + size / 4, size / 9, c);
        tft_.drawLine(x - size / 2, y + size / 2, x + size / 2, y - size / 2, c);
        tft_.drawLine(x - size / 2, y - size / 6, x + size / 3, y + size / 2, c);
    } else if (strcmp(reaction, "Ola") == 0) {
        tft_.fillCircle(x, y + size / 8, size / 4, c);
        for (int8_t finger = -2; finger <= 2; ++finger) tft_.fillRoundRect(x + finger * size / 8 - size / 16, y - size / 2, size / 8, size / 2, size / 16, c);
    } else if (strcmp(reaction, "Palmas") == 0) {
        tft_.fillRoundRect(x - size / 3, y - size / 3, size / 4, size * 2 / 3, size / 10, c);
        tft_.fillRoundRect(x + size / 12, y - size / 3, size / 4, size * 2 / 3, size / 10, c);
    } else {
        // "Gostei" and unknown reactions use a clear, full-screen affirmative mark.
        tft_.fillRoundRect(x - size / 8, y - size / 2, size / 4, size * 3 / 4, size / 12, c);
        tft_.fillTriangle(x - size / 3, y - size / 8, x + size / 8, y - size / 8, x - size / 3, y + size / 3, c);
    }
}

void NetinDisplay::pokeIcon(int16_t x, int16_t y, int16_t size, uint16_t color) {
    const uint16_t c = panelColor(color);
    const int16_t radius = size / 3;
    tft_.drawCircle(x, y, radius, c);
    tft_.drawCircle(x, y, radius + 10, c);
    tft_.drawCircle(x, y, radius + 20, c);
    tft_.fillRoundRect(x - 7, y - radius / 2, 14, radius, 5, c);
    tft_.fillCircle(x, y + radius / 2 + 14, 8, c);
}

void NetinDisplay::backIcon(int16_t x, int16_t y, uint16_t color) {
    const uint16_t c = panelColor(color);
    tft_.drawLine(x + 8, y - 14, x - 6, y, c);
    tft_.drawLine(x + 7, y - 14, x - 7, y, c);
    tft_.drawLine(x - 6, y, x + 8, y + 14, c);
    tft_.drawLine(x - 7, y, x + 7, y + 14, c);
}

void NetinDisplay::menuIcon(int16_t x, int16_t y, uint16_t color) {
    const uint16_t c = panelColor(color);
    tft_.fillRoundRect(x - 14, y - 10, 28, 4, 2, c);
    tft_.fillRoundRect(x - 14, y - 2, 28, 4, 2, c);
    tft_.fillRoundRect(x - 14, y + 6, 28, 4, 2, c);
}

void NetinDisplay::settingsIcon(int16_t x, int16_t y, uint16_t color) {
    const uint16_t c = panelColor(color);
    // Eight teeth, ring and central hole form a conventional gear silhouette.
    tft_.fillRect(x - 3, y - 22, 6, 8, c);
    tft_.fillRect(x - 3, y + 14, 6, 8, c);
    tft_.fillRect(x - 22, y - 3, 8, 6, c);
    tft_.fillRect(x + 14, y - 3, 8, 6, c);
    tft_.fillRect(x - 17, y - 17, 7, 7, c);
    tft_.fillRect(x + 10, y - 17, 7, 7, c);
    tft_.fillRect(x - 17, y + 10, 7, 7, c);
    tft_.fillRect(x + 10, y + 10, 7, 7, c);
    tft_.drawCircle(x, y, 15, c);
    tft_.drawCircle(x, y, 9, c);
    tft_.fillCircle(x, y, 4, c);
}

void NetinDisplay::statusIcon(int16_t x, int16_t y, PresenceStatus status, uint16_t color, uint16_t backgroundColor) {
    const uint16_t c = panelColor(color);
    const uint16_t bg = panelColor(backgroundColor);
    switch (status) {
        case PresenceStatus::Available:
            tft_.fillCircle(x, y, 22, c);
            break;
        case PresenceStatus::Busy:
            tft_.drawCircle(x, y, 22, c);
            tft_.fillRect(x - 15, y - 4, 30, 8, c);
            break;
        case PresenceStatus::Focused:
            tft_.drawCircle(x, y, 23, c);
            tft_.drawCircle(x, y, 13, c);
            tft_.fillCircle(x, y, 4, c);
            break;
        case PresenceStatus::Away:
            tft_.drawCircle(x, y, 22, c);
            tft_.drawLine(x, y, x, y - 14, c);
            tft_.drawLine(x, y, x + 10, y + 7, c);
            break;
        case PresenceStatus::Invisible:
            tft_.drawCircle(x, y, 20, c);
            tft_.fillCircle(x, y, 5, c);
            tft_.drawLine(x - 25, y + 25, x + 25, y - 25, bg);
            tft_.drawLine(x - 23, y + 25, x + 25, y - 23, c);
            break;
        case PresenceStatus::InCall:
            tft_.drawLine(x - 15, y - 18, x - 2, y - 5, c);
            tft_.drawLine(x - 2, y - 5, x + 9, y + 10, c);
            tft_.drawLine(x - 18, y - 13, x - 11, y - 20, c);
            tft_.drawLine(x + 6, y + 13, x + 14, y + 5, c);
            break;
        case PresenceStatus::Gaming:
            tft_.drawRoundRect(x - 23, y - 14, 46, 28, 8, c);
            tft_.drawLine(x - 12, y, x - 2, y, c);
            tft_.drawLine(x - 7, y - 5, x - 7, y + 5, c);
            tft_.fillCircle(x + 10, y - 4, 2, c);
            tft_.fillCircle(x + 15, y + 4, 2, c);
            break;
        case PresenceStatus::Sleeping:
            tft_.drawLine(x - 16, y - 14, x - 2, y - 14, c);
            tft_.drawLine(x - 2, y - 14, x - 16, y + 6, c);
            tft_.drawLine(x - 16, y + 6, x - 2, y + 6, c);
            tft_.drawLine(x + 3, y - 5, x + 17, y - 5, c);
            tft_.drawLine(x + 17, y - 5, x + 3, y + 15, c);
            tft_.drawLine(x + 3, y + 15, x + 17, y + 15, c);
            break;
        case PresenceStatus::DoNotDisturb:
            tft_.drawCircle(x, y, 22, c);
            tft_.fillRect(x - 14, y - 4, 28, 8, c);
            break;
    }
}
