#include "media_manager.h"

#include <HTTPClient.h>
#include <SD.h>
#include <TJpg_Decoder.h>
#include <WiFiClientSecure.h>
#include <mbedtls/sha256.h>

#include "pairing/pairing_manager.h"

namespace {
constexpr char kActiveMediaPath[] = "/media-atual";
constexpr int16_t kSenderOverlayHeight = 22;
}

MediaManager *MediaManager::activeRenderer_ = nullptr;

bool MediaManager::showActiveMedia(const String &mimeType, const String &senderName) {
    return showMedia(kActiveMediaPath, mimeType, senderName);
}

bool MediaManager::showMedia(const char *path, const String &mimeType, const String &senderName) {
    closeActiveMedia();
    senderName_ = senderName.substring(0, 24);
    if (mimeType == "image/gif") return showGif(path);
    if (mimeType == "image/jpeg") return showJpeg(path);
    detail_ = "Midia nao suportada";
    return false;
}

bool MediaManager::showReactionMedia(const String &reactionId, const String &url, const String &deviceId, const String &credential, const String &mimeType, const String &expectedSha256, size_t expectedSize, const String &senderName) {
    if (reactionId.length() != 36) {
        detail_ = "Reacao invalida";
        return false;
    }
    const String path = String("/reaction-") + reactionId;
    if (!hasCachedReaction(path, expectedSha256) && (!downloadToPath(url, deviceId, credential, expectedSha256, expectedSize, path) || !saveReactionHash(path, expectedSha256))) return false;
    return showMedia(path.c_str(), mimeType, senderName);
}

bool MediaManager::showJpeg(const char *path) {
    if (!SD.exists(path)) {
        detail_ = "Midia nao encontrada";
        Serial.println("Media: active file missing");
        return false;
    }
    tft_.fillScreen(TFT_WHITE);
    activeRenderer_ = this;
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(outputBlock);
    const JRESULT result = TJpgDec.drawSdJpg(0, 0, path);
    activeRenderer_ = nullptr;
    if (result != JDR_OK) {
        detail_ = "Falha ao abrir JPEG";
        Serial.printf("Media: JPEG decoder failed (%d)\n", static_cast<int>(result));
        return false;
    }
    drawSenderOverlay();
    detail_ = "JPEG exibido";
    return true;
}

bool MediaManager::showGif(const char *path) {
    if (!SD.exists(path)) {
        detail_ = "Midia nao encontrada";
        return false;
    }
    tft_.fillScreen(TFT_WHITE);
    activeRenderer_ = this;
    gif_.begin(GIF_PALETTE_RGB565_BE);
    if (!gif_.open(path, gifOpen, gifClose, gifRead, gifSeek, gifDraw)) {
        activeRenderer_ = nullptr;
        detail_ = "Falha ao abrir GIF";
        return false;
    }
    gifOffsetX_ = max<int16_t>(0, (tft_.width() - gif_.getCanvasWidth()) / 2);
    gifOffsetY_ = max<int16_t>(0, (tft_.height() - gif_.getCanvasHeight()) / 2);
    gifNextFrameAt_ = 0;
    drawSenderOverlay();
    gifPlaying_ = true;
    detail_ = "GIF exibido";
    return true;
}

void MediaManager::tick() {
    if (!gifPlaying_) return;
    if (gifNextFrameAt_ != 0 && static_cast<int32_t>(millis() - gifNextFrameAt_) < 0) return;
    int delayMs = 0;
    const int result = gif_.playFrame(false, &delayMs);
    if (result < 0) {
        gif_.reset();
        gifNextFrameAt_ = millis() + 100;
        return;
    }
    if (result == 0) gif_.reset();
    gifNextFrameAt_ = millis() + constrain(delayMs, 20, 500);
}

void MediaManager::closeActiveMedia() {
    if (gifPlaying_) gif_.close();
    gifPlaying_ = false;
    gifNextFrameAt_ = 0;
    senderName_ = "";
    activeRenderer_ = nullptr;
}

void MediaManager::drawSenderOverlay() {
    if (senderName_.isEmpty()) return;
    tft_.fillRect(0, 0, 240, kSenderOverlayHeight, TFT_WHITE);
    tft_.setTextColor(TFT_BLACK, TFT_WHITE);
    tft_.setTextSize(1);
    tft_.setCursor(8, 7);
    tft_.print("De " + senderName_);
}

bool MediaManager::downloadMedia(const String &url, const String &deviceId, const String &credential, const String &expectedSha256, size_t expectedSize) {
    return downloadToPath(url, deviceId, credential, expectedSha256, expectedSize, kActiveMediaPath);
}

bool MediaManager::downloadToPath(const String &url, const String &deviceId, const String &credential, const String &expectedSha256, size_t expectedSize, const String &destination) {
    if (sdCard_.state() != SdCardState::Ready) {
        detail_ = "SD indisponivel";
        return false;
    }
    WiFiClientSecure client;
    client.setCACert(netinApiRootCertificate());
    HTTPClient http;
    if (!http.begin(client, url)) {
        detail_ = "Download falhou";
        return false;
    }
    http.setTimeout(15000);
    http.addHeader("X-Netin-Device-Id", deviceId);
    http.addHeader("Authorization", String("Bearer ") + credential);
    const int status = http.GET();
    if (status != HTTP_CODE_OK) {
        Serial.printf("Media: HTTP %d\n", status);
        http.end();
        detail_ = "Download falhou";
        return false;
    }
    const int length = http.getSize();
    if (length < 1 || static_cast<size_t>(length) != expectedSize) {
        http.end();
        detail_ = "Tamanho invalido";
        return false;
    }
    const String temporary = destination + ".part";
    const String backup = destination + ".bak";
    if (SD.exists(temporary)) SD.remove(temporary);
    File output = SD.open(temporary, FILE_WRITE);
    if (!output) {
        http.end();
        detail_ = "Falha no SD";
        return false;
    }
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    mbedtls_sha256_starts_ret(&hash, 0);
    WiFiClient *stream = http.getStreamPtr();
    uint8_t buffer[512];
    size_t written = 0;
    while (http.connected() && written < expectedSize) {
        const size_t available = stream->available();
        if (!available) {
            delay(1);
            continue;
        }
        const size_t read = stream->readBytes(buffer, min(available, sizeof(buffer)));
        if (!read || output.write(buffer, read) != read) break;
        mbedtls_sha256_update_ret(&hash, buffer, read);
        written += read;
    }
    uint8_t digest[32];
    mbedtls_sha256_finish_ret(&hash, digest);
    mbedtls_sha256_free(&hash);
    output.close();
    http.end();
    char actual[65];
    for (uint8_t i = 0; i < 32; ++i) snprintf(actual + i * 2, 3, "%02x", digest[i]);
    if (written != expectedSize || expectedSha256 != actual) {
        Serial.printf("Media: integrity failed bytes=%u expected=%u hash=%s\n", static_cast<unsigned>(written), static_cast<unsigned>(expectedSize), actual);
        SD.remove(temporary);
        detail_ = "Arquivo invalido";
        return false;
    }
    if (SD.exists(backup)) SD.remove(backup);
    if (SD.exists(destination)) SD.rename(destination, backup);
    if (!SD.rename(temporary, destination)) {
        if (SD.exists(backup)) SD.rename(backup, destination);
        detail_ = "Falha no cache";
        return false;
    }
    SD.remove(backup);
    detail_ = "Midia pronta";
    return true;
}

bool MediaManager::hasCachedReaction(const String &path, const String &expectedSha256) {
    if (!SD.exists(path) || !SD.exists(path + ".sha")) return false;
    File metadata = SD.open(path + ".sha", FILE_READ);
    if (!metadata) return false;
    const String storedHash = metadata.readString();
    metadata.close();
    return storedHash == expectedSha256;
}

bool MediaManager::saveReactionHash(const String &path, const String &expectedSha256) {
    const String temporary = path + ".sha.part";
    const String destination = path + ".sha";
    if (SD.exists(temporary)) SD.remove(temporary);
    File metadata = SD.open(temporary, FILE_WRITE);
    if (!metadata || metadata.print(expectedSha256) != expectedSha256.length()) {
        if (metadata) metadata.close();
        SD.remove(temporary);
        detail_ = "Falha no cache";
        return false;
    }
    metadata.close();
    if (SD.exists(destination)) SD.remove(destination);
    if (!SD.rename(temporary, destination)) {
        SD.remove(temporary);
        detail_ = "Falha no cache";
        return false;
    }
    return true;
}

bool MediaManager::outputBlock(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t *pixels) {
    if (!activeRenderer_ || y >= activeRenderer_->tft_.height()) return false;
    const uint32_t count = static_cast<uint32_t>(width) * height;
    for (uint32_t index = 0; index < count; ++index) pixels[index] = static_cast<uint16_t>(~pixels[index]);
    activeRenderer_->tft_.pushImage(x, y, width, height, pixels);
    return true;
}

void MediaManager::gifDraw(GIFDRAW *draw) {
    if (!activeRenderer_) return;
    const int16_t y = activeRenderer_->gifOffsetY_ + draw->iY + draw->y;
    const int16_t startX = activeRenderer_->gifOffsetX_ + draw->iX;
    const int16_t width = min<int16_t>(draw->iWidth, activeRenderer_->tft_.width() - startX);
    if (y < kSenderOverlayHeight || y >= activeRenderer_->tft_.height() || startX < 0 || startX >= activeRenderer_->tft_.width() || width <= 0) return;
    uint16_t pixels[240];
    uint8_t *source = draw->pPixels;
    bool hasTransparency = draw->ucHasTransparency;
    if (draw->ucDisposalMethod == 2 && hasTransparency) {
        for (int16_t index = 0; index < width; ++index) {
            if (source[index] == draw->ucTransparent) source[index] = draw->ucBackground;
        }
        hasTransparency = false;
    }
    int16_t index = 0;
    while (index < width) {
        while (hasTransparency && index < width && source[index] == draw->ucTransparent) ++index;
        const int16_t runStart = index;
        uint16_t count = 0;
        while (index < width && (!hasTransparency || source[index] != draw->ucTransparent)) {
            pixels[count++] = static_cast<uint16_t>(~draw->pPalette[source[index++]]);
        }
        if (count) activeRenderer_->tft_.pushImage(startX + runStart, y, count, 1, pixels);
    }
}

void *MediaManager::gifOpen(const char *path, int32_t *size) {
    if (!activeRenderer_) return nullptr;
    activeRenderer_->gifFile_ = SD.open(path, FILE_READ);
    if (!activeRenderer_->gifFile_) return nullptr;
    *size = static_cast<int32_t>(activeRenderer_->gifFile_.size());
    return &activeRenderer_->gifFile_;
}

void MediaManager::gifClose(void *handle) {
    if (handle) static_cast<File *>(handle)->close();
}

int32_t MediaManager::gifRead(GIFFILE *file, uint8_t *buffer, int32_t length) {
    File *input = static_cast<File *>(file->fHandle);
    const int32_t available = file->iSize - file->iPos;
    const int32_t request = min(length, max(0, available));
    if (!input || request <= 0) return 0;
    const int32_t read = input->read(buffer, request);
    file->iPos = static_cast<int32_t>(input->position());
    return read;
}

int32_t MediaManager::gifSeek(GIFFILE *file, int32_t position) {
    File *input = static_cast<File *>(file->fHandle);
    if (!input || !input->seek(position)) return -1;
    file->iPos = static_cast<int32_t>(input->position());
    return file->iPos;
}
