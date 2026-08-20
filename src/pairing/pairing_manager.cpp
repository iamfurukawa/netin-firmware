#include "pairing_manager.h"

#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>

namespace {
constexpr char kNamespace[] = "pairing";
constexpr char kApiBaseUrl[] = "https://glados-server.13997906387.xyz";
constexpr char kHardwareTarget[] = "esp32-2432s024";
constexpr unsigned long kCodeLifetimeMs = 10UL * 60UL * 1000UL;
constexpr unsigned long kPollIntervalMs = 4000;

// Google Trust Services GTS Root R4, which validates the current Cloudflare
// certificate chain presented by netin-server. Keep this explicit rather than
// using setInsecure(): pairing credentials must not travel over unverified TLS.
constexpr char kApiRootCertificate[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi
QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR
HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW
BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D
9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8
p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD
-----END CERTIFICATE-----
)EOF";

String jsonString(const String &json, const char *key) {
    const String marker = String("\"") + key + "\":\"";
    const int start = json.indexOf(marker);
    if (start < 0) return "";
    const int valueStart = start + marker.length();
    const int valueEnd = json.indexOf('"', valueStart);
    return valueEnd < 0 ? "" : json.substring(valueStart, valueEnd);
}

}

const char *netinApiRootCertificate() { return kApiRootCertificate; }

String PairingStore::loadCredential() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return "";
    const String credential = prefs.getString("credential", "");
    prefs.end();
    return credential;
}

bool PairingStore::saveCredential(const String &credential) const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;
    const bool saved = prefs.putString("credential", credential) > 0;
    prefs.end();
    return saved;
}

bool PairingStore::clearCredential() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return false;
    const bool cleared = prefs.remove("credential");
    prefs.end();
    return cleared;
}

void PairingManager::begin() {
    if (!store_.loadCredential().isEmpty()) state_ = PairingState::Paired;
}

void PairingManager::requestCode() {
    if (network_.state() != NetworkState::Connected) {
        state_ = PairingState::Offline;
        return;
    }
    state_ = PairingState::Preparing;
    requestPending_ = true;
}

void PairingManager::invalidateCredential() {
    store_.clearCredential();
    code_ = "";
    requestPending_ = false;
    state_ = PairingState::Unpaired;
}

bool PairingManager::post(const char *path, String &response, int &statusCode) {
    WiFiClientSecure client;
    client.setCACert(kApiRootCertificate);
    HTTPClient http;
    const String url = String(kApiBaseUrl) + path;
    if (!http.begin(client, url)) return false;
    http.setTimeout(10000);
    http.addHeader("Content-Type", "application/json");
    const String body = String("{\"deviceId\":\"") + identity_.id + "\",\"bootstrapSecret\":\"" + identity_.bootstrapSecret + "\"}";
    statusCode = http.POST(body);
    if (statusCode > 0) response = http.getString();
    http.end();
    return statusCode > 0;
}

bool PairingManager::registerDevice(bool &paired) {
    WiFiClientSecure client;
    client.setCACert(kApiRootCertificate);
    HTTPClient http;
    if (!http.begin(client, String(kApiBaseUrl) + "/device/register")) return false;
    http.setTimeout(10000);
    http.addHeader("Content-Type", "application/json");
    const String body = String("{\"deviceId\":\"") + identity_.id + "\",\"bootstrapSecret\":\"" + identity_.bootstrapSecret + "\",\"hardwareTarget\":\"" + kHardwareTarget + "\"}";
    const int statusCode = http.POST(body);
    const String response = statusCode > 0 ? http.getString() : "";
    http.end();
    if (statusCode != 201) return false;
    paired = response.indexOf("\"paired\":true") >= 0;
    return true;
}

bool PairingManager::requestPairingCode() {
    String response;
    int statusCode = 0;
    if (!post("/device/pairing-code", response, statusCode) || statusCode != 200) return false;
    code_ = jsonString(response, "code");
    return !code_.isEmpty();
}

bool PairingManager::checkPairingStatus(bool &paired) {
    String response;
    int statusCode = 0;
    if (!post("/device/pairing-status", response, statusCode) || statusCode != 200) return false;
    paired = response.indexOf("\"paired\":true") >= 0;
    return true;
}

bool PairingManager::requestCredential() {
    String response;
    int statusCode = 0;
    if (!post("/device/credential", response, statusCode) || statusCode != 200) return false;
    const String credential = jsonString(response, "credential");
    return !credential.isEmpty() && store_.saveCredential(credential);
}

void PairingManager::setError(const char *message) {
    error_ = message;
    state_ = PairingState::Error;
}

void PairingManager::tick() {
    if (requestPending_) {
        if (!network_.timeReady()) return;
        requestPending_ = false;
        bool paired = false;
        if (!registerDevice(paired)) return setError("Falha ao registrar");
        if (paired) {
            if (!requestCredential()) return setError("Falha na credencial");
            state_ = PairingState::Paired;
            return;
        }
        if (!requestPairingCode()) return setError("Falha ao gerar codigo");
        codeExpiresAt_ = millis() + kCodeLifetimeMs;
        lastStatusPollAt_ = millis();
        state_ = PairingState::CodeReady;
        return;
    }

    if (state_ != PairingState::CodeReady) return;
    if (static_cast<int32_t>(millis() - codeExpiresAt_) >= 0) {
        code_ = "";
        state_ = PairingState::Unpaired;
        return;
    }
    if (millis() - lastStatusPollAt_ < kPollIntervalMs) return;
    lastStatusPollAt_ = millis();
    bool paired = false;
    if (!checkPairingStatus(paired)) return;
    if (paired) {
        if (!requestCredential()) return setError("Falha na credencial");
        code_ = "";
        state_ = PairingState::Paired;
    }
}
