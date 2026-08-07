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
        case Screen::Network: drawNetwork(); break;
        case Screen::Device: drawDevice(); break;
    }
}

void Ui::tick() {
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
    } else if (hit(event.x, event.y, kConfirmX, kConfirmY, kConfirmW, kConfirmH)) {
        applyCandidate();
    }
}
