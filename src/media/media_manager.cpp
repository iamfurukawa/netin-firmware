#include "media_manager.h"

#include <SD.h>
#include <TJpg_Decoder.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/sha256.h>

#include "pairing/pairing_manager.h"

namespace {
constexpr char kTestImagePath[] = "/netin-test.jpg";
constexpr char kActiveMediaPath[] = "/media-atual";
constexpr char kTemporaryImagePath[] = "/media-nova.part";
constexpr char kBackupImagePath[] = "/media-anterior.bak";
constexpr size_t kTestImageSize = 2029;
constexpr char kTestImageBase64[] = "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";

int base64Value(char value) {
    if (value >= 'A' && value <= 'Z') return value - 'A';
    if (value >= 'a' && value <= 'z') return value - 'a' + 26;
    if (value >= '0' && value <= '9') return value - '0' + 52;
    if (value == '+') return 62;
    if (value == '/') return 63;
    return -1;
}
}  // namespace

MediaManager *MediaManager::activeRenderer_ = nullptr;

bool MediaManager::prepareTestImage() {
    if (sdCard_.state() != SdCardState::Ready) {
        detail_ = "SD indisponivel";
        return false;
    }
    File existing = SD.open(kTestImagePath, FILE_READ);
    if (existing) {
        const size_t size = existing.size();
        existing.close();
        if (size == kTestImageSize) {
            detail_ = "Imagem pronta no SD";
            return true;
        }
        SD.remove(kTestImagePath);
    }
    return writeEmbeddedTestImage();
}

bool MediaManager::writeEmbeddedTestImage() {
    SD.remove(kTestImagePath);
    File output = SD.open(kTestImagePath, FILE_WRITE);
    if (!output) {
        detail_ = "Falha ao gravar imagem";
        return false;
    }
    uint32_t buffer = 0;
    uint8_t bits = 0;
    size_t count = 0;
    for (const char *cursor = kTestImageBase64; *cursor; ++cursor) {
        const int value = base64Value(*cursor);
        if (value < 0) continue;
        buffer = (buffer << 6) | static_cast<uint32_t>(value);
        bits += 6;
        while (bits >= 8) {
            bits -= 8;
            if (output.write(static_cast<uint8_t>(buffer >> bits)) != 1) {
                output.close();
                SD.remove(kTestImagePath);
                detail_ = "Falha ao gravar imagem";
                return false;
            }
            ++count;
        }
    }
    output.close();
    if (count != kTestImageSize) {
        SD.remove(kTestImagePath);
        detail_ = "Imagem de teste invalida";
        return false;
    }
    detail_ = "Imagem pronta no SD";
    return true;
}

bool MediaManager::showTestImage() {
    if (!prepareTestImage()) return false;
    // TFT_eSPI output is physically inverted on this panel. White is therefore
    // the logical dark backdrop used by the rest of the interface.
    tft_.fillScreen(TFT_WHITE);
    activeRenderer_ = this;
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(outputBlock);
    const JRESULT result = TJpgDec.drawSdJpg(0, 0, kTestImagePath);
    activeRenderer_ = nullptr;
    if (result != JDR_OK) {
        detail_ = "Falha ao abrir JPEG";
        return false;
    }
    detail_ = "JPEG exibido";
    return true;
}

bool MediaManager::showActiveImage() { return showJpeg(kActiveMediaPath); }

bool MediaManager::showActiveMedia(const String &mimeType) {
    closeActiveMedia();
    if (mimeType == "image/gif") return showGif(kActiveMediaPath);
    if (mimeType == "image/jpeg") return showJpeg(kActiveMediaPath);
    detail_ = "Midia nao suportada";
    return false;
}

bool MediaManager::showJpeg(const char *path) {
    if (!SD.exists(path)) { detail_ = "Midia nao encontrada"; Serial.println("Media: active file missing"); return false; }
    tft_.fillScreen(TFT_WHITE);
    activeRenderer_ = this;
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(outputBlock);
    const JRESULT result = TJpgDec.drawSdJpg(0, 0, path);
    activeRenderer_ = nullptr;
    if (result != JDR_OK) { detail_ = "Falha ao abrir JPEG"; Serial.printf("Media: JPEG decoder failed (%d)\n", static_cast<int>(result)); return false; }
    Serial.printf("Media: JPEG rendered %s\n", path);
    detail_ = "JPEG exibido";
    return true;
}

bool MediaManager::showGif(const char *path) {
    if (!SD.exists(path)) { detail_ = "Midia nao encontrada"; return false; }
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
    gifPlaying_ = true;
    detail_ = "GIF exibido";
    return true;
}

void MediaManager::tick() {
    if (!gifPlaying_) return;
    if (!gif_.playFrame(true, nullptr)) gif_.reset();
}

void MediaManager::closeActiveMedia() {
    if (!gifPlaying_) return;
    gif_.close();
    gifPlaying_ = false;
    activeRenderer_ = nullptr;
}

bool MediaManager::downloadMedia(const String &url, const String &deviceId, const String &credential, const String &expectedSha256, size_t expectedSize) {
    if (sdCard_.state() != SdCardState::Ready) { detail_ = "SD indisponivel"; return false; }
    WiFiClientSecure client;
    client.setCACert(netinApiRootCertificate());
    HTTPClient http;
    if (!http.begin(client, url)) { detail_ = "Download falhou"; return false; }
    http.setTimeout(15000);
    http.addHeader("X-Netin-Device-Id", deviceId);
    http.addHeader("Authorization", String("Bearer ") + credential);
    const int status = http.GET();
    if (status != HTTP_CODE_OK) { Serial.printf("Media: HTTP %d\n", status); http.end(); detail_ = "Download falhou"; return false; }
    const int length = http.getSize();
    if (length < 1 || static_cast<size_t>(length) != expectedSize) { http.end(); detail_ = "Tamanho invalido"; return false; }
    if (SD.exists(kTemporaryImagePath)) SD.remove(kTemporaryImagePath);
    File output = SD.open(kTemporaryImagePath, FILE_WRITE);
    if (!output) { http.end(); detail_ = "Falha no SD"; return false; }
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    mbedtls_sha256_starts_ret(&hash, 0);
    WiFiClient *stream = http.getStreamPtr();
    uint8_t buffer[512]; size_t written = 0;
    while (http.connected() && written < expectedSize) {
        const size_t available = stream->available();
        if (!available) { delay(1); continue; }
        const size_t read = stream->readBytes(buffer, min(available, sizeof(buffer)));
        if (!read || output.write(buffer, read) != read) break;
        mbedtls_sha256_update_ret(&hash, buffer, read); written += read;
    }
    uint8_t digest[32]; mbedtls_sha256_finish_ret(&hash, digest); mbedtls_sha256_free(&hash);
    output.close(); http.end();
    char actual[65]; for (uint8_t i = 0; i < 32; ++i) snprintf(actual + i * 2, 3, "%02x", digest[i]);
    if (written != expectedSize || expectedSha256 != actual) { Serial.printf("Media: integrity failed bytes=%u expected=%u hash=%s\n", static_cast<unsigned>(written), static_cast<unsigned>(expectedSize), actual); SD.remove(kTemporaryImagePath); detail_ = "Arquivo invalido"; return false; }
    if (SD.exists(kBackupImagePath)) SD.remove(kBackupImagePath);
    if (SD.exists(kActiveMediaPath)) SD.rename(kActiveMediaPath, kBackupImagePath);
    if (!SD.rename(kTemporaryImagePath, kActiveMediaPath)) { if (SD.exists(kBackupImagePath)) SD.rename(kBackupImagePath, kActiveMediaPath); detail_ = "Falha no cache"; return false; }
    SD.remove(kBackupImagePath); detail_ = "Midia pronta"; return true;
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
    if (y < 0 || y >= activeRenderer_->tft_.height() || startX < 0 || startX >= activeRenderer_->tft_.width() || width <= 0) return;
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
