#include "ui.h"

#include "sync/sync_manager.h"

namespace {
constexpr int16_t kScreenW = 240;
constexpr int16_t kScreenH = 320;
constexpr int16_t kMenuButtonX = 12;
constexpr int16_t kMenuButtonY = 12;
constexpr int16_t kMenuButtonSize = 48;
constexpr int16_t kStatusCardX = 0;
constexpr int16_t kStatusCardY = kMenuButtonY + kMenuButtonSize;
constexpr int16_t kStatusCardW = kScreenW;
constexpr int16_t kStatusCardH = kScreenH - kStatusCardY;
constexpr int16_t kPickerRowX = 12;
constexpr int16_t kPickerRowY = 68;
constexpr int16_t kPickerRowW = 164;
constexpr int16_t kPickerRowH = 48;
constexpr int16_t kPickerRowGap = 8;
constexpr int16_t kPickerRowStep = kPickerRowH + kPickerRowGap;
constexpr int16_t kPickerListBottom = 310;
constexpr int16_t kPickerVisibleHeight = kPickerListBottom - kPickerRowY;
constexpr int16_t kPickerContentHeight = kStatusCount * kPickerRowStep - kPickerRowGap;
constexpr int16_t kPickerMaxScroll = kPickerContentHeight - kPickerVisibleHeight;
constexpr int16_t kPickerBackX = 12;
constexpr int16_t kPickerBackY = 12;
constexpr int16_t kPickerBackSize = 48;
constexpr int16_t kPickerScrollX = 184;
constexpr int16_t kPickerScrollUpY = 68;
constexpr int16_t kPickerScrollDownY = 236;
constexpr int16_t kPickerScrollSize = 44;
constexpr int16_t kBackX = 12;
constexpr int16_t kBackY = 264;
constexpr int16_t kBackW = 100;
constexpr int16_t kBackH = 44;
constexpr int16_t kThemeY = 72;
constexpr int16_t kNetworkY = 128;
constexpr int16_t kDeviceY = 184;
constexpr int16_t kSettingsRowH = 48;

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
        case Screen::StatusPicker: drawStatusPicker(); break;
        case Screen::Settings: drawSettings(); break;
        case Screen::Network: drawNetwork(); break;
        case Screen::Device: drawDevice(); break;
        case Screen::Interaction: drawInteraction(); break;
    }
}

void Ui::tick() {
    if (settings_.status != lastStatus_) {
        lastStatus_ = settings_.status;
        if (screen_ == Screen::Home) draw();
    }
    const uint8_t pendingCount = sync_.pendingCount();
    if (pendingCount != lastPendingCount_) {
        lastPendingCount_ = pendingCount;
        if (screen_ == Screen::Home) draw();
    }
    if (sync_.interactionRevision() != lastInteractionRevision_) {
        lastInteractionRevision_ = sync_.interactionRevision();
        screen_ = Screen::Interaction;
        draw();
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

    // The status starts below the menu so the top-left control stays easy to hit.
    display_.button(kStatusCardX, kStatusCardY, kStatusCardW, kStatusCardH, "", statusColor, foreground);
    display_.statusIcon(120, 158, settings_.status, foreground, statusColor);
    const char *label = statusLabel(settings_.status);
    const int16_t labelX = 120 - static_cast<int16_t>(strlen(label) * 18) / 2;
    display_.text(labelX, 214, label, foreground, statusColor, 3);

    if (statusQueueFull_) {
        display_.text(58, 278, "Fila cheia: status local", foreground, statusColor, 1);
    } else if (sync_.pendingCount() > 0) {
        display_.text(76, 278, "Sincronizando...", foreground, statusColor, 1);
    }

    display_.button(kMenuButtonX, kMenuButtonY, kMenuButtonSize, kMenuButtonSize, "", display_.muted(theme), foreground);
    display_.menuIcon(kMenuButtonX + kMenuButtonSize / 2, kMenuButtonY + kMenuButtonSize / 2, foreground);
}

void Ui::drawStatusPicker() {
    const Theme theme = settings_.theme;
    const uint16_t foreground = display_.foreground(theme);
    display_.clear(theme);
    display_.button(kPickerBackX, kPickerBackY, kPickerBackSize, kPickerBackSize, "", display_.muted(theme), foreground);
    display_.backIcon(kPickerBackX + kPickerBackSize / 2, kPickerBackY + kPickerBackSize / 2, foreground);
    const uint16_t upColor = pickerScroll_ > 0 ? display_.muted(theme) : display_.background(theme);
    const uint16_t downColor = pickerScroll_ < kPickerMaxScroll ? display_.muted(theme) : display_.background(theme);
    display_.button(kPickerScrollX, kPickerScrollUpY, kPickerScrollSize, kPickerScrollSize, "^", upColor, foreground);
    display_.button(kPickerScrollX, kPickerScrollDownY, kPickerScrollSize, kPickerScrollSize, "v", downColor, foreground);

    for (uint8_t index = 0; index < kStatusCount; ++index) {
        const auto status = static_cast<PresenceStatus>(index);
        const int16_t y = kPickerRowY + index * kPickerRowStep - pickerScroll_;
        if (y < kPickerRowY || y + kPickerRowH > kPickerListBottom) continue;
        const uint16_t color = display_.statusColor(status);
        display_.button(kPickerRowX, y, kPickerRowW, kPickerRowH, "", color, foreground);
        display_.statusIcon(38, y + 24, status, foreground, color);
        const char *label = statusLabel(status);
        const uint8_t textSize = strlen(label) > 10 ? 1 : 2;
        const int16_t textY = textSize == 1 ? y + 20 : y + 15;
        display_.text(68, textY, label, foreground, color, textSize);
        if (status == settings_.status) {
            display_.fillCircle(158, y + 27, 6, foreground);
        }
    }
}

void Ui::drawSettings() {
    const Theme theme = settings_.theme;
    const uint16_t background = display_.background(theme);
    const uint16_t foreground = display_.foreground(theme);
    display_.clear(theme);
    display_.text(22, 30, "Ajustes", foreground, background, 3);
    display_.button(20, kThemeY, 200, kSettingsRowH, theme == Theme::Dark ? "Tema claro" : "Tema escuro", display_.muted(theme), foreground);
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
    display_.text(20, 80, identity_.id.substring(0, 8).c_str(), foreground, background, 1);
    const PairingState pairingState = pairing_.state();
    if (pairingState != PairingState::Paired) {
        display_.text(20, 108, pairingLabel(pairingState), foreground, background, 1);
    }
    if (pairingState == PairingState::CodeReady) {
        display_.text(20, 126, pairing_.code().c_str(), foreground, background, 2);
        display_.text(20, 158, "Adicione o codigo na PWA", foreground, background, 1);
    } else if (pairingState == PairingState::Paired) {
        display_.text(20, 118, "Pareado", display_.statusColor(PresenceStatus::Available), background, 2);
    } else {
        display_.button(20, 160, 200, 48, pairingState == PairingState::Preparing ? "Aguarde" : "Gerar codigo", display_.muted(theme), foreground);
    }
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

void Ui::applyStatus(PresenceStatus status) {
    const bool changed = status != settings_.status;
    if (changed) ++settings_.statusChangeCount;
    settings_.status = status;
    store_.save(settings_);
    lastStatus_ = status;
    statusQueueFull_ = changed && !sync_.enqueueStatus(status);
    lastPendingCount_ = sync_.pendingCount();
    screen_ = Screen::Home;
    draw();
}

void Ui::handle(const TouchEvent &event) {
    if (event.type == TouchEventType::None) return;

    if (screen_ == Screen::Home) {
        if (event.type != TouchEventType::Tap) return;
        if (hit(event.x, event.y, kMenuButtonX, kMenuButtonY, kMenuButtonSize, kMenuButtonSize)) {
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
        if (event.type != TouchEventType::Tap) return;
        if (hit(event.x, event.y, kPickerBackX, kPickerBackY, kPickerBackSize, kPickerBackSize)) {
            screen_ = Screen::Home;
            draw();
            return;
        }
        if (hit(event.x, event.y, kPickerScrollX, kPickerScrollUpY, kPickerScrollSize, kPickerScrollSize) && pickerScroll_ > 0) {
            pickerScroll_ -= kPickerRowStep;
            if (pickerScroll_ < 0) pickerScroll_ = 0;
            draw();
            return;
        }
        if (hit(event.x, event.y, kPickerScrollX, kPickerScrollDownY, kPickerScrollSize, kPickerScrollSize) && pickerScroll_ < kPickerMaxScroll) {
            pickerScroll_ += kPickerRowStep;
            if (pickerScroll_ > kPickerMaxScroll) pickerScroll_ = kPickerMaxScroll;
            draw();
            return;
        }
        for (uint8_t index = 0; index < kStatusCount; ++index) {
            const int16_t y = kPickerRowY + index * kPickerRowStep - pickerScroll_;
            if (y < kPickerRowY || y + kPickerRowH > kPickerListBottom) continue;
            if (hit(event.x, event.y, kPickerRowX, y, kPickerRowW, kPickerRowH)) {
                applyStatus(static_cast<PresenceStatus>(index));
                return;
            }
        }
        return;
    }

    if (event.type != TouchEventType::Tap) return;

    if (screen_ == Screen::Interaction) {
        if (sync_.interaction().type == "reaction" || sync_.interaction().type == "poke" || hit(event.x, event.y, 12, 258, 216, 48)) {
            sync_.dismissInteraction();
            screen_ = Screen::Home;
            draw();
        }
        return;
    }

    if (screen_ == Screen::Settings) {
        if (hit(event.x, event.y, 20, kThemeY, 200, kSettingsRowH)) {
            settings_.theme = settings_.theme == Theme::Dark ? Theme::Light : Theme::Dark;
            store_.save(settings_);
            draw();
        } else if (hit(event.x, event.y, 20, kNetworkY, 200, kSettingsRowH)) {
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
    } else if (screen_ == Screen::Device && hit(event.x, event.y, 20, 160, 200, 48)) {
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
        } else {
            screen_ = Screen::StatusPicker;
        }
        draw();
    }
}
