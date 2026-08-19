#pragma once

#include <Arduino.h>
#include <AnimatedGIF.h>
#include <SD.h>
#include <TFT_eSPI.h>

#include "storage/sd_card.h"

class MediaManager {
  public:
    MediaManager(TFT_eSPI &tft, SdCardManager &sdCard) : tft_(tft), sdCard_(sdCard) {}
    bool prepareTestImage();
    bool showTestImage();
    bool showActiveImage();
    bool showActiveMedia(const String &mimeType, const String &senderName);
    void tick();
    void closeActiveMedia();
    bool downloadMedia(const String &url, const String &deviceId, const String &credential, const String &expectedSha256, size_t expectedSize);
    const String &detail() const { return detail_; }

  private:
    static bool outputBlock(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t *pixels);
    static void gifDraw(GIFDRAW *draw);
    static void *gifOpen(const char *path, int32_t *size);
    static void gifClose(void *handle);
    static int32_t gifRead(GIFFILE *file, uint8_t *buffer, int32_t length);
    static int32_t gifSeek(GIFFILE *file, int32_t position);
    bool writeEmbeddedTestImage();
    bool showJpeg(const char *path);
    bool showGif(const char *path);
    void drawSenderOverlay();

    static MediaManager *activeRenderer_;
    TFT_eSPI &tft_;
    SdCardManager &sdCard_;
    AnimatedGIF gif_;
    File gifFile_;
    bool gifPlaying_ = false;
    uint32_t gifNextFrameAt_ = 0;
    int16_t gifOffsetX_ = 0;
    int16_t gifOffsetY_ = 0;
    String senderName_;
    String detail_ = "Imagem nao preparada";
};
