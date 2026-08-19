#include "ui.h"

#include "sync/sync_manager.h"

namespace {
constexpr int16_t kScreenW = 240;
constexpr int16_t kScreenH = 320;
constexpr int16_t kStatusCardX = 0;
constexpr int16_t kStatusCardY = 0;
constexpr int16_t kStatusCardW = kScreenW;
constexpr int16_t kStatusCardH = kScreenH;
constexpr int16_t kBackX = 12;
constexpr int16_t kBackY = 264;
constexpr int16_t kBackW = 100;
constexpr int16_t kBackH = 44;
constexpr int16_t kNetworkY = 72;
constexpr int16_t kDeviceY = 136;
constexpr int16_t kSettingsRowH = 48;
constexpr int16_t kSdDiagnosticPairedY = 190;
constexpr int16_t kSdDiagnosticUnpairedY = 222;
constexpr uint32_t kReceivedContentTimeoutMs = 3UL * 60UL * 1000UL;

const char *networkLabel(NetworkState state) {
    switch (state) {
        case NetworkState::Unconfigured: return "Nao configurada";
        case NetworkState::Connecting: return "Conectando";
        case NetworkState::Connected: return "Conectada";
        case NetworkState::Portal: return "Portal ativo";
    }
    return "Desconhecida";
}

const char *pairingLabel(PairingState state) {
    switch (state) {
        case PairingState::Unpaired: return "Nao pareado";
        case PairingState::Preparing: return "Preparando...";
        case PairingState::CodeReady: return "Codigo de pareamento";
        case PairingState::Paired: return "Pareado";
        case PairingState::Offline: return "Conecte ao WiFi";
        case PairingState::Error: return "Falha ao conectar";
    }
    return "Desconhecido";
}

const char *sdLabel(SdCardState state) {
    switch (state) {
        case SdCardState::Ready: return "SD pronto";
        case SdCardState::Unavailable: return "SD indisponivel";
        case SdCardState::Error: return "SD com erro";
    }
    return "SD desconhecido";
}

void drawInteractionText(NetinDisplay &display, const String &text, uint16_t foreground, uint16_t background) {
    String remaining = text;
    for (uint8_t line = 0; line < 6 && !remaining.isEmpty(); ++line) {
        int16_t split = min(static_cast<int16_t>(18), static_cast<int16_t>(remaining.length()));
        if (split < remaining.length()) {
            const int16_t space = remaining.lastIndexOf(' ', split);
            if (space > 0) split = space;
        }
        display.text(18, 70 + line * 26, remaining.substring(0, split).c_str(), foreground, background, 2);
        remaining = remaining.substring(split);
        remaining.trim();
    }
}
}

bool Ui::hit(uint16_t x, uint16_t y, int16_t rx, int16_t ry, int16_t rw, int16_t rh) const {
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

void Ui::draw() {
    switch (screen_) {
        case Screen::Home: drawHome(); break;
        case Screen::Settings: drawSettings(); break;
        case Screen::Network: drawNetwork(); break;
        case Screen::Device: drawDevice(); break;
        case Screen::Interaction: drawInteraction(); break;
        case Screen::MediaReceived: break;
    }
}

void Ui::tick() {
    if (settings_.status != lastStatus_) {
        lastStatus_ = settings_.status;
        if (screen_ == Screen::Home) draw();
    }
    if (sync_.mediaRevision() != lastMediaRevision_) {
        lastMediaRevision_ = sync_.mediaRevision();
        if (sync_.mediaVisible()) {
            screen_ = Screen::MediaReceived;
            mediaOpenedAt_ = millis();
        }
        else if (screen_ == Screen::MediaReceived) {
            screen_ = Screen::Home;
            draw();
        }
    }
    if (sync_.interactionRevision() != lastInteractionRevision_ && screen_ != Screen::MediaReceived) {
        lastInteractionRevision_ = sync_.interactionRevision();
        screen_ = Screen::Interaction;
        interactionOpenedAt_ = millis();
        draw();
    }
    if (screen_ == Screen::MediaReceived && millis() - mediaOpenedAt_ >= kReceivedContentTimeoutMs) {
        sync_.dismissMedia();
        screen_ = Screen::Home;
        draw();
    }
    if (screen_ == Screen::Interaction && millis() - interactionOpenedAt_ >= kReceivedContentTimeoutMs) {
        const bool hasNext = sync_.dismissInteraction();
        lastInteractionRevision_ = sync_.interactionRevision();
        if (hasNext) {
            interactionOpenedAt_ = millis();
            draw();
        } else {
            screen_ = Screen::Home;
            draw();
        }
    }
    const NetworkState current = network_.state();
    if (current != lastNetworkState_) {
        lastNetworkState_ = current;
        if (screen_ == Screen::Network) draw();
    }
    const PairingState pairingState = pairing_.state();
    if (pairingState != lastPairingState_) {
        lastPairingState_ = pairingState;
        if (screen_ == Screen::Device) draw();
    }
}

void Ui::drawHome() {
    const Theme theme = settings_.theme;
    const uint16_t foreground = display_.foreground(theme);
    const uint16_t statusColor = display_.statusColor(settings_.status);
    display_.clear(theme);

    display_.button(kStatusCardX, kStatusCardY, kStatusCardW, kStatusCardH, "", statusColor, foreground);
    display_.statusIcon(120, 128, settings_.status, foreground, statusColor);
    const char *label = statusLabel(settings_.status);
    const int16_t labelX = 120 - static_cast<int16_t>(strlen(label) * 18) / 2;
    display_.text(labelX, 188, label, foreground, statusColor, 3);
}

void Ui::drawSettings() {
    const Theme theme = settings_.theme;
    const uint16_t background = display_.background(theme);
    const uint16_t foreground = display_.foreground(theme);
    display_.clear(theme);
    display_.text(22, 30, "Ajustes", foreground, background, 3);
    display_.button(20, kNetworkY, 200, kSettingsRowH, "Rede", display_.muted(theme), foreground);
    display_.button(20, kDeviceY, 200, kSettingsRowH, "Dispositivo", display_.muted(theme), foreground);
    display_.button(kBackX, kBackY, kBackW, kBackH, "Voltar", display_.muted(theme), foreground);
}

void Ui::drawNetwork() {
    const Theme theme = settings_.theme;
    const uint16_t background = display_.background(theme);
    const uint16_t foreground = display_.foreground(theme);
    display_.clear(theme);
    display_.text(20, 24, "Rede", foreground, background, 3);
    if (network_.state() == NetworkState::Portal) {
        display_.text(20, 62, "Conecte em", foreground, background, 1);
        display_.text(20, 80, network_.portalSsid().c_str(), foreground, background, 2);
        display_.text(20, 104, "Senha", foreground, background, 1);
        display_.text(20, 120, network_.portalPassword().c_str(), foreground, background, 2);
        if (network_.portalConnectionFailed()) {
            display_.text(20, 140, "Nao conectou. Tente de novo.", display_.statusColor(PresenceStatus::Busy), background, 1);
        }
    } else if (network_.state() == NetworkState::Connected) {
        display_.text(20, 62, "Conectada em", foreground, background, 1);
        display_.text(20, 82, network_.connectedSsid().c_str(), foreground, background, 2);
    } else {
        display_.text(20, 66, networkLabel(network_.state()), foreground, background, 2);
    }
    display_.button(20, 150, 200, 48, "Configurar WiFi", display_.muted(theme), foreground);
    display_.button(20, 208, 200, 40, confirmForgetNetworks_ ? "Confirmar apagar" : "Esquecer redes", confirmForgetNetworks_ ? display_.statusColor(PresenceStatus::Busy) : display_.background(theme), foreground);
    display_.button(kBackX, kBackY, kBackW, kBackH, confirmForgetNetworks_ ? "Cancelar" : "Voltar", display_.muted(theme), foreground);
}

void Ui::drawDevice() {
    const Theme theme = settings_.theme;
    const uint16_t background = display_.background(theme);
    const uint16_t foreground = display_.foreground(theme);
    display_.clear(theme);
    display_.text(20, 24, "Dispositivo", foreground, background, 3);
    display_.text(20, 64, "ID da placa", foreground, background, 1);
    display_.text(20, 82, identity_.id.substring(0, 8).c_str(), foreground, background, 2);
    const PairingState pairingState = pairing_.state();
    if (pairingState != PairingState::Paired) {
        display_.text(20, 110, pairingLabel(pairingState), foreground, background, 2);
    }
    if (pairingState == PairingState::CodeReady) {
        display_.text(20, 138, pairing_.code().c_str(), foreground, background, 2);
        display_.text(20, 170, "Adicione o codigo na PWA", foreground, background, 1);
    } else if (pairingState == PairingState::Paired) {
        display_.text(20, 118, "Pareado", display_.statusColor(PresenceStatus::Available), background, 2);
    } else {
        display_.button(20, 108, 200, 40, pairingState == PairingState::Preparing ? "Aguarde" : "Gerar codigo", display_.muted(theme), foreground);
    }
    const bool pairingUsesLowerArea = pairingState != PairingState::Paired;
    const int16_t sdTextY = pairingUsesLowerArea ? 194 : 154;
    const int16_t sdButtonY = pairingUsesLowerArea ? 222 : 190;
    const uint16_t sdColor = sdCard_.state() == SdCardState::Ready ? display_.statusColor(PresenceStatus::Available) : display_.statusColor(PresenceStatus::Busy);
    display_.text(20, sdTextY, sdLabel(sdCard_.state()), sdColor, background, 1);
    if (sdCard_.state() == SdCardState::Ready) {
        const uint32_t totalMb = static_cast<uint32_t>(sdCard_.totalBytes() / (1024ULL * 1024ULL));
        const uint32_t usedMb = static_cast<uint32_t>(sdCard_.usedBytes() / (1024ULL * 1024ULL));
        String capacity = String(usedMb) + " MB de " + String(totalMb) + " MB";
        display_.text(20, sdTextY + 14, capacity.c_str(), foreground, background, 1);
    } else {
        display_.text(20, sdTextY + 14, sdCard_.detail().c_str(), foreground, background, 1);
    }
    display_.button(20, sdButtonY, 200, 40, "Verificar SD", display_.muted(theme), foreground);
    display_.button(kBackX, kBackY, kBackW, kBackH, "Voltar", display_.muted(theme), foreground);
}

void Ui::drawInteraction() {
    const Theme theme = settings_.theme;
    const uint16_t background = display_.background(theme);
    const uint16_t foreground = display_.foreground(theme);
    const SocialInteraction &interaction = sync_.interaction();
    display_.clear(theme);
    display_.text(18, 18, interaction.senderName.c_str(), display_.statusColor(PresenceStatus::Focused), background, 2);
    if (interaction.type == "reaction") {
        display_.reactionIcon(120, 160, 140, interaction.content.c_str(), interaction.content == "Coracao" ? TFT_RED : display_.statusColor(PresenceStatus::Focused));
        return;
    }
    if (interaction.type == "poke") {
        display_.text(48, 70, "Cutucou voce", TFT_ORANGE, background, 2);
        display_.pokeIcon(120, 172, 140, TFT_ORANGE);
        return;
    }
    drawInteractionText(display_, interaction.content, foreground, background);
    display_.button(12, 258, 216, 48, "Voltar", display_.muted(theme), foreground);
}

void Ui::handle(const TouchEvent &event) {
    if (event.type == TouchEventType::None) return;

    if (screen_ == Screen::Home) {
        if (event.type != TouchEventType::Tap) return;
        screen_ = Screen::Settings;
        draw();
        return;
    }

    if (event.type != TouchEventType::Tap) return;

    if (screen_ == Screen::Interaction) {
        if (sync_.interaction().type == "reaction" || sync_.interaction().type == "poke" || hit(event.x, event.y, 12, 258, 216, 48)) {
            const bool hasNext = sync_.dismissInteraction();
            lastInteractionRevision_ = sync_.interactionRevision();
            if (hasNext) {
                interactionOpenedAt_ = millis();
            } else screen_ = Screen::Home;
            draw();
        }
        return;
    }

    if (screen_ == Screen::MediaReceived) {
        sync_.dismissMedia();
        screen_ = Screen::Home;
        draw();
        return;
    }

    if (screen_ == Screen::Settings) {
        if (hit(event.x, event.y, 20, kNetworkY, 200, kSettingsRowH)) {
            screen_ = Screen::Network;
            lastNetworkState_ = network_.state();
            confirmForgetNetworks_ = false;
            draw();
        } else if (hit(event.x, event.y, 20, kDeviceY, 200, kSettingsRowH)) {
            screen_ = Screen::Device;
            lastPairingState_ = pairing_.state();
            draw();
        } else if (hit(event.x, event.y, kBackX, kBackY, kBackW, kBackH)) {
            screen_ = Screen::Home;
            draw();
        }
    } else if (screen_ == Screen::Network && hit(event.x, event.y, 20, 150, 200, 48)) {
        network_.startPortal();
        lastNetworkState_ = network_.state();
        draw();
    } else if (screen_ == Screen::Device && hit(event.x, event.y, 20, pairing_.state() == PairingState::Paired ? kSdDiagnosticPairedY : kSdDiagnosticUnpairedY, 200, 40)) {
        sdCard_.runDiagnostic();
        draw();
    } else if (screen_ == Screen::Device && pairing_.state() != PairingState::Paired && hit(event.x, event.y, 20, 108, 200, 40)) {
        pairing_.requestCode();
        lastPairingState_ = pairing_.state();
        draw();
    } else if (screen_ == Screen::Network && hit(event.x, event.y, 20, 208, 200, 40)) {
        if (confirmForgetNetworks_) {
            network_.forgetNetworks();
            lastNetworkState_ = network_.state();
            confirmForgetNetworks_ = false;
        } else {
            confirmForgetNetworks_ = true;
        }
        draw();
    } else if (hit(event.x, event.y, kBackX, kBackY, kBackW, kBackH)) {
        if (screen_ == Screen::Network && confirmForgetNetworks_) {
            confirmForgetNetworks_ = false;
            draw();
            return;
        }
        if (screen_ == Screen::Network || screen_ == Screen::Device) {
            screen_ = Screen::Settings;
        } else screen_ = Screen::Home;
        draw();
    }
}
