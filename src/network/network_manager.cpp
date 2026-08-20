#include "network_manager.h"

#include <Preferences.h>
#include <esp_system.h>
#include <time.h>

namespace {
constexpr char kNamespace[] = "wifi";
constexpr unsigned long kConnectTimeoutMs = 15000;
constexpr unsigned long kPortalLifetimeMs = 10UL * 60UL * 1000UL;
constexpr unsigned long kMaxRetryDelayMs = 60000;

String keyFor(const char *prefix, uint8_t index) {
    return String(prefix) + index;
}

String portalPassword() {
    char value[9];
    snprintf(value, sizeof(value), "%08lu", static_cast<unsigned long>(esp_random() % 100000000));
    return String(value);
}

String htmlEscape(const String &value) {
    String escaped;
    escaped.reserve(value.length());
    for (const char character : value) {
        switch (character) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += character; break;
        }
    }
    return escaped;
}
}

bool NetworkStore::hasProfiles() const {
    String ssid;
    String password;
    return firstProfile(ssid, password);
}

uint8_t NetworkStore::loadProfiles(WifiProfile profiles[], uint8_t capacity) const {
    if (capacity == 0) return 0;
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return 0;
    uint8_t count = 0;
    for (uint8_t index = 0; index < kMaxProfiles && count < capacity; ++index) {
        const String ssid = prefs.getString(keyFor("ssid", index).c_str(), "");
        if (ssid.isEmpty()) continue;
        WifiProfile &profile = profiles[count++];
        profile.ssid = ssid;
        profile.password = prefs.getString(keyFor("pass", index).c_str(), "");
        profile.lastSuccess = prefs.getUInt(keyFor("last", index).c_str(), 0);
    }
    prefs.end();
    for (uint8_t left = 0; left < count; ++left) {
        for (uint8_t right = left + 1; right < count; ++right) {
            if (profiles[right].lastSuccess > profiles[left].lastSuccess) {
                const WifiProfile swap = profiles[left];
                profiles[left] = profiles[right];
                profiles[right] = swap;
            }
        }
    }
    return count;
}

bool NetworkStore::firstProfile(String &ssid, String &password) const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return false;
    for (uint8_t index = 0; index < kMaxProfiles; ++index) {
        const String savedSsid = prefs.getString(keyFor("ssid", index).c_str(), "");
        if (savedSsid.isEmpty()) continue;
        ssid = savedSsid;
        password = prefs.getString(keyFor("pass", index).c_str(), "");
        prefs.end();
        return true;
    }
    prefs.end();
    return false;
}

bool NetworkStore::willReplaceProfile(const String &ssid) const {
    if (ssid.isEmpty()) return false;
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return false;
    uint8_t count = 0;
    bool alreadySaved = false;
    for (uint8_t index = 0; index < kMaxProfiles; ++index) {
        const String savedSsid = prefs.getString(keyFor("ssid", index).c_str(), "");
        if (savedSsid.isEmpty()) continue;
        ++count;
        if (savedSsid == ssid) alreadySaved = true;
    }
    prefs.end();
    return !alreadySaved && count >= kMaxProfiles;
}

bool NetworkStore::saveProfile(const String &ssid, const String &password) const {
    if (ssid.isEmpty()) return false;
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;

    uint8_t target = 0;
    uint32_t oldestSuccess = UINT32_MAX;
    for (uint8_t index = 0; index < kMaxProfiles; ++index) {
        const String current = prefs.getString(keyFor("ssid", index).c_str(), "");
        if (current == ssid || current.isEmpty()) {
            target = index;
            break;
        }
        const uint32_t lastSuccess = prefs.getUInt(keyFor("last", index).c_str(), 0);
        if (lastSuccess < oldestSuccess) {
            oldestSuccess = lastSuccess;
            target = index;
        }
    }
    const uint32_t sequence = prefs.getUInt("successSeq", 0) + 1;
    const bool ok = prefs.putString(keyFor("ssid", target).c_str(), ssid) > 0 &&
                    prefs.putString(keyFor("pass", target).c_str(), password) >= 0 &&
                    prefs.putUInt(keyFor("last", target).c_str(), sequence) == sizeof(uint32_t) &&
                    prefs.putUInt("successSeq", sequence) == sizeof(uint32_t);
    prefs.end();
    return ok;
}

bool NetworkStore::markSuccess(const String &ssid) const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;
    const uint32_t sequence = prefs.getUInt("successSeq", 0) + 1;
    bool updated = false;
    for (uint8_t index = 0; index < kMaxProfiles; ++index) {
        if (prefs.getString(keyFor("ssid", index).c_str(), "") == ssid) {
            updated = prefs.putUInt(keyFor("last", index).c_str(), sequence) == sizeof(uint32_t) &&
                      prefs.putUInt("successSeq", sequence) == sizeof(uint32_t);
            break;
        }
    }
    prefs.end();
    return updated;
}

bool NetworkStore::clearProfiles() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;
    const bool ok = prefs.clear();
    prefs.end();
    return ok;
}

NetworkManager::NetworkManager(NetworkStore &store, const DeviceIdentity &identity)
    : store_(store), identity_(identity) {}

void NetworkManager::begin() {
    WiFi.mode(WIFI_STA);
    startSavedNetwork();
}

void NetworkManager::startSavedNetwork() {
    savedProfileCount_ = store_.loadProfiles(savedProfiles_, NetworkStore::kMaxProfiles);
    savedProfileIndex_ = 0;
    if (savedProfileCount_ == 0) {
        state_ = NetworkState::Unconfigured;
        return;
    }
    tryNextSavedNetwork();
}

void NetworkManager::tryNextSavedNetwork() {
    if (savedProfileIndex_ >= savedProfileCount_) {
        retryAt_ = millis() + retryDelayMs_;
        retryDelayMs_ = min(retryDelayMs_ * 2, kMaxRetryDelayMs);
        state_ = NetworkState::Unconfigured;
        return;
    }
    const WifiProfile &profile = savedProfiles_[savedProfileIndex_++];
    WiFi.begin(profile.ssid.c_str(), profile.password.c_str());
    connectStartedAt_ = millis();
    state_ = NetworkState::Connecting;
}

void NetworkManager::configurePortalRoutes() {
    server_.on("/", HTTP_GET, [this] { server_.send(200, "text/html; charset=utf-8", portalPage()); });
    server_.on("/save", HTTP_POST, [this] {
        const String ssid = server_.arg("ssid");
        const String password = server_.arg("password");
        if (ssid.isEmpty()) {
            server_.send(400, "text/plain", "SSID obrigatorio");
            return;
        }
        if (store_.willReplaceProfile(ssid) && server_.arg("replace") != "1") {
            const String page = "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
                "<title>Confirmar rede</title><style>body{font:16px sans-serif;max-width:28rem;margin:2rem auto;padding:1rem}button{box-sizing:border-box;width:100%;padding:.8rem;margin:.6rem 0}</style>"
                "<h1>Substituir rede salva?</h1><p>O Netin guarda ate 5 redes. Ao continuar, a rede usada ha mais tempo sera substituida.</p>"
                "<form method=post action=/save><input type=hidden name=ssid value='" + htmlEscape(ssid) + "'><input type=hidden name=password value='" + htmlEscape(password) + "'><input type=hidden name=replace value=1><button>Substituir e conectar</button></form>"
                "<a href='/'>Cancelar</a>";
            server_.send(200, "text/html; charset=utf-8", page);
            return;
        }
        pendingSsid_ = ssid;
        pendingPassword_ = password;
        portalConnectionFailed_ = false;
        WiFi.begin(ssid.c_str(), password.c_str());
        connectStartedAt_ = millis();
        state_ = NetworkState::Connecting;
        server_.send(200, "text/html; charset=utf-8", "<h1>Conectando...</h1><p>Volte para a tela do Netin.</p>");
    });
    server_.onNotFound([this] { server_.sendHeader("Location", "http://192.168.4.1/"); server_.send(302, "text/plain", ""); });
}

String NetworkManager::portalPage() const {
    String options;
    const int found = WiFi.scanNetworks();
    for (int index = 0; index < found; ++index) {
        options += "<option value=\"" + WiFi.SSID(index) + "\">" + WiFi.SSID(index) + "</option>";
    }
    return "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
           "<title>Netin Wi-Fi</title><style>body{font:16px sans-serif;max-width:28rem;margin:2rem auto;padding:1rem}input,button{box-sizing:border-box;width:100%;padding:.8rem;margin:.4rem 0}</style>"
           "<h1>Conectar Netin</h1><form method=post action=/save><label>Rede Wi-Fi</label><input name=ssid list=redes required><datalist id=redes>" + options +
           "</datalist><label>Senha</label><input name=password type=password><button>Conectar</button></form>";
}

void NetworkManager::startPortal() {
    if (portalRunning_) stopPortal();
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);
    portalSsid_ = "Netin-" + identity_.id.substring(0, 4);
    portalPassword_ = ::portalPassword();
    portalConnectionFailed_ = false;
    WiFi.softAP(portalSsid_.c_str(), portalPassword_.c_str());
    dns_.start(53, "*", WiFi.softAPIP());
    configurePortalRoutes();
    server_.begin();
    portalRunning_ = true;
    retryAt_ = 0;
    portalStartedAt_ = millis();
    state_ = NetworkState::Portal;
}

void NetworkManager::stopPortal() {
    if (portalRunning_) {
        dns_.stop();
        server_.stop();
    }

    WiFi.softAPdisconnect(true);
    delay(50);
    portalPassword_ = "";
    WiFi.mode(WIFI_STA);
    portalRunning_ = false;
}

void NetworkManager::forgetNetworks() {
    stopPortal();
    WiFi.disconnect(true, true);
    store_.clearProfiles();
    state_ = NetworkState::Unconfigured;
}

String NetworkManager::connectedSsid() const {
    return state_ == NetworkState::Connected ? WiFi.SSID() : "";
}

void NetworkManager::tick() {
    if (portalRunning_) {
        dns_.processNextRequest();
        server_.handleClient();
        if (millis() - portalStartedAt_ >= kPortalLifetimeMs) {
            stopPortal();
            startSavedNetwork();
            return;
        }
    }

    if (state_ == NetworkState::Connected && timeSyncStarted_ && !timeReady_ && time(nullptr) > 1700000000) {
        timeReady_ = true;
    }

    if (state_ == NetworkState::Connected && WiFi.status() != WL_CONNECTED) {
        retryAt_ = millis();
        state_ = NetworkState::Unconfigured;
    }

    if (state_ == NetworkState::Unconfigured && retryAt_ != 0 && static_cast<int32_t>(millis() - retryAt_) >= 0) {
        retryAt_ = 0;
        startSavedNetwork();
    }

    if (state_ != NetworkState::Connecting) return;
    if (WiFi.status() == WL_CONNECTED) {
        if (portalRunning_) {
            store_.saveProfile(pendingSsid_, pendingPassword_);
            pendingSsid_ = "";
            pendingPassword_ = "";
            stopPortal();
        }
        if (!portalRunning_) store_.markSuccess(WiFi.SSID());
        portalConnectionFailed_ = false;
        state_ = NetworkState::Connected;
        retryDelayMs_ = 5000;
        if (!timeSyncStarted_) {
            configTime(0, 0, "time.cloudflare.com", "pool.ntp.org");
            timeSyncStarted_ = true;
        }
        return;
    }
    if (millis() - connectStartedAt_ > kConnectTimeoutMs) {
        portalConnectionFailed_ = portalRunning_;
        if (portalRunning_) {
            state_ = NetworkState::Portal;
        } else {
            tryNextSavedNetwork();
        }
    }

}
