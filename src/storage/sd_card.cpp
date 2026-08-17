#include "sd_card.h"

#include <SD.h>
#include <SPI.h>

namespace {
constexpr uint8_t kSdChipSelectPin = 5;
constexpr uint8_t kSdSckPin = 18;
constexpr uint8_t kSdMisoPin = 19;
constexpr uint8_t kSdMosiPin = 23;
constexpr uint32_t kSdFrequency = 4000000;
// FAT implementations on embedded targets may reject a filename that starts
// with a dot, so keep the temporary diagnostic file conventional and remove it
// immediately after verification.
constexpr char kDiagnosticPath[] = "/netin-sd-check.txt";
constexpr char kDiagnosticContent[] = "NETIN-SD-OK\n";
SPIClass sdSpi(VSPI);
}

bool SdCardManager::begin() {
    SD.end();
    pinMode(kSdChipSelectPin, OUTPUT);
    digitalWrite(kSdChipSelectPin, HIGH);
    sdSpi.begin(kSdSckPin, kSdMisoPin, kSdMosiPin, kSdChipSelectPin);
    if (!SD.begin(kSdChipSelectPin, sdSpi, kSdFrequency) || SD.cardType() == CARD_NONE) {
        state_ = SdCardState::Unavailable;
        detail_ = "Cartao nao encontrado";
        totalBytes_ = 0;
        usedBytes_ = 0;
        Serial.println("SD: mount failed");
        return false;
    }
    state_ = SdCardState::Ready;
    detail_ = "Pronto";
    updateUsage();
    Serial.printf("SD: ready, total=%llu used=%llu\n", totalBytes_, usedBytes_);
    return true;
}

bool SdCardManager::runDiagnostic() {
    // The SD card shares this board with the display. Mounting is deliberately a
    // boot-only operation: do not call SD.end()/SD.begin() from a UI action
    // while the display is active on the other SPI controller. The file access
    // below is the actual diagnostic and also refreshes a stale UI state.
    File output = SD.open(kDiagnosticPath, FILE_WRITE);
    if (!output) {
        state_ = SdCardState::Error;
        detail_ = "Falha ao escrever";
        return false;
    }
    const size_t written = output.print(kDiagnosticContent);
    output.close();
    if (written != strlen(kDiagnosticContent)) {
        SD.remove(kDiagnosticPath);
        state_ = SdCardState::Error;
        detail_ = "Escrita incompleta";
        return false;
    }
    File input = SD.open(kDiagnosticPath, FILE_READ);
    const String content = input ? input.readString() : "";
    if (input) input.close();
    const bool removed = SD.remove(kDiagnosticPath);
    if (content != kDiagnosticContent || !removed) {
        state_ = SdCardState::Error;
        detail_ = "Falha ao ler/remover";
        return false;
    }
    state_ = SdCardState::Ready;
    detail_ = "Leitura e escrita OK";
    updateUsage();
    Serial.println("SD: diagnostic passed");
    return true;
}

void SdCardManager::updateUsage() {
    totalBytes_ = SD.totalBytes();
    usedBytes_ = SD.usedBytes();
}
