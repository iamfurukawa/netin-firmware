#include <Arduino.h>
#include <TFT_eSPI.h>

// Netin's first hardware-safe screen. The panel needs ILI9341_2_DRIVER; that
// sequence is stable on this board but displays the inverse RGB565 value.
static TFT_eSPI tft;
static constexpr uint16_t panelColor(uint16_t color) {
    return static_cast<uint16_t>(~color);
}

// Calibrated on this physical board in portrait orientation (2026-07-25).
static uint16_t touchCalibration[] = {652, 2994, 423, 3361, 3};
static constexpr uint16_t kTouchThreshold = 600;
static constexpr uint8_t kRgbLedRedPin = 4;
static constexpr uint8_t kRgbLedGreenPin = 16;
static constexpr uint8_t kRgbLedBluePin = 17;
static constexpr int kButtonX = 24;
static constexpr int kButtonY = 236;
static constexpr int kButtonW = 192;
static constexpr int kButtonH = 52;

static bool buttonWasPressed = false;
static uint32_t updateCount = 0;
static uint32_t lastRedraw = 0;

static void text(int x, int y, const char *value, uint16_t foreground, uint16_t background, uint8_t size = 1) {
    tft.setTextColor(panelColor(foreground), panelColor(background));
    tft.setTextSize(size);
    tft.setCursor(x, y);
    tft.print(value);
}

static void drawButton(bool pressed) {
    const uint16_t fill = pressed ? TFT_NAVY : TFT_BLUE;
    tft.fillRoundRect(kButtonX, kButtonY, kButtonW, kButtonH, 10, panelColor(fill));
    tft.drawRoundRect(kButtonX, kButtonY, kButtonW, kButtonH, 10, panelColor(TFT_WHITE));
    text(pressed ? 54 : 66, kButtonY + 17, pressed ? "Atualizando" : "Atualizar", TFT_WHITE, fill, 2);
}

static void drawScreen() {
    tft.fillScreen(panelColor(TFT_BLACK));

    tft.fillRoundRect(12, 12, 216, 52, 12, panelColor(TFT_BLUE));
    text(28, 23, "NETIN", TFT_WHITE, TFT_BLUE, 3);
    text(155, 31, "PAINEL", TFT_CYAN, TFT_BLUE, 1);

    tft.fillRoundRect(12, 78, 216, 132, 12, panelColor(TFT_DARKGREY));
    text(28, 94, "Conexao", TFT_LIGHTGREY, TFT_DARKGREY, 1);
    text(28, 113, "Aguardando Wi-Fi", TFT_WHITE, TFT_DARKGREY, 2);
    tft.fillCircle(202, 119, 7, panelColor(TFT_ORANGE));

    text(28, 156, "IP", TFT_LIGHTGREY, TFT_DARKGREY, 1);
    text(28, 174, "--. --. --. --", TFT_WHITE, TFT_DARKGREY, 2);
    text(28, 208, "Toque no botao para testar", TFT_CYAN, TFT_BLACK, 1);

    drawButton(false);
    text(28, 302, "Atualizacoes: 0", TFT_DARKGREY, TFT_BLACK, 1);
}

static void drawCounter() {
    tft.fillRect(20, 298, 205, 16, panelColor(TFT_BLACK));
    char message[32];
    snprintf(message, sizeof(message), "Atualizacoes: %lu", static_cast<unsigned long>(updateCount));
    text(28, 302, message, TFT_DARKGREY, TFT_BLACK, 1);
}

void setup() {
    Serial.begin(115200);
    // The onboard RGB LED is active-low. Keeping all channels HIGH turns it off.
    pinMode(kRgbLedRedPin, OUTPUT);
    pinMode(kRgbLedGreenPin, OUTPUT);
    pinMode(kRgbLedBluePin, OUTPUT);
    digitalWrite(kRgbLedRedPin, HIGH);
    digitalWrite(kRgbLedGreenPin, HIGH);
    digitalWrite(kRgbLedBluePin, HIGH);

    tft.init();
    tft.setRotation(0);
    tft.setTextWrap(false, false);

    tft.setTouch(touchCalibration);
    drawScreen();
}

void loop() {
    uint16_t x = 0;
    uint16_t y = 0;
    const bool touching = tft.getTouch(&x, &y, kTouchThreshold);
    const bool overButton = touching && x >= kButtonX && x < kButtonX + kButtonW &&
                            y >= kButtonY && y < kButtonY + kButtonH;

    if (overButton && !buttonWasPressed) {
        buttonWasPressed = true;
        drawButton(true);
        Serial.printf("Botao pressionado em x=%u y=%u\n", x, y);
    }

    if (!touching && buttonWasPressed) {
        buttonWasPressed = false;
        ++updateCount;
        drawButton(false);
        drawCounter();
    }

    if (millis() - lastRedraw > 50) {
        lastRedraw = millis();
    }
}
