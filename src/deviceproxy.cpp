#include "deviceproxy.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QByteArray>
#include <QDateTime>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QtMath>
#include <cmath>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

static const QString SERVICE   = "org.ratatoskr";
static const QString PATH      = "/org/ratatoskr/devices/a50_gen5";
static const QString INTERFACE = "org.ratatoskr.Device";

// EQ constants from A50 HID protocol (verified via G HUB USBPcap)
static const int HEADPHONE_FREQS[] = {20, 50, 125, 250, 500, 1000, 2500, 5000, 10000, 20000};
static const int MIC_FREQS[] = {80, 150, 250, 400, 800, 1500, 2500, 5000, 10000, 19000};
static const int EQ_GAIN_CENTER = 120;  // 0 dB = 120, 20 units per dB (range -6..+6 dB, bytes 0x00..0xF0)

// Q factor ↔ byte: Q = 0.031 + (byte/255)^0.8584 × 7.938
static uint8_t qFactorToByte(double q) {
    q = qBound(0.031, q, 7.969);
    double norm = (q - 0.031) / 7.938;
    return static_cast<uint8_t>(qBound(0, qRound(255.0 * std::pow(norm, 1.1650)), 255));
}

static QString presetsFilePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/ratatoskr";
    QDir().mkpath(dir);
    return dir + "/eq-presets.json";
}

static QString drainHistoryFilePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/ratatoskr";
    QDir().mkpath(dir);
    return dir + "/drain-history.jsonl";
}

static QString drainLockFilePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/ratatoskr";
    QDir().mkpath(dir);
    return dir + "/drain-history.lock";
}

DeviceProxy::DeviceProxy(QObject* parent) : QObject(parent) {
    auto bus = QDBusConnection::systemBus();

    m_iface = new QDBusInterface(SERVICE, PATH, INTERFACE, bus, this);

    // Connect D-Bus signals
    bus.connect(SERVICE, PATH, INTERFACE, "BatteryChanged",
                this, SLOT(onBatteryChanged(int,bool)));
    bus.connect(SERVICE, PATH, INTERFACE, "VolumeChanged",
                this, SLOT(onVolumeChanged(int)));
    bus.connect(SERVICE, PATH, INTERFACE, "MicMuteChanged",
                this, SLOT(onMicMuteChanged(bool)));
    bus.connect(SERVICE, PATH, INTERFACE, "MixampChanged",
                this, SLOT(onMixampChanged(int)));
    bus.connect(SERVICE, PATH, INTERFACE, "PowerChanged",
                this, SLOT(onPowerChanged(int)));
    bus.connect(SERVICE, PATH, INTERFACE, "BluetoothChanged",
                this, SLOT(onBluetoothChanged(bool)));
    bus.connect(SERVICE, PATH, INTERFACE, "EqualizerChanged",
                this, SIGNAL(equalizerChanged(quint32)));

    // Initial poll at startup
    QTimer::singleShot(100, this, &DeviceProxy::pollAll);

    // Battery poll every 5 minutes
    auto* batteryTimer = new QTimer(this);
    connect(batteryTimer, &QTimer::timeout, this, &DeviceProxy::pollBattery);
    batteryTimer->start(300000);

    // Try to become drain-history writer. If another process (the noctalia
    // plugin) already holds the lock, we stay a reader and retry every 30 s
    // — when the previous writer exits, the next attempt succeeds.
    tryAcquireDrainLock();
    auto* drainRetry = new QTimer(this);
    connect(drainRetry, &QTimer::timeout, this, [this]() {
        if (!m_isDrainWriter) tryAcquireDrainLock();
    });
    drainRetry->start(30000);
}

void DeviceProxy::tryAcquireDrainLock() {
    if (m_isDrainWriter) return;  // already writer
    QByteArray path = drainLockFilePath().toUtf8();
    int fd = ::open(path.constData(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (fd < 0) return;
    if (::flock(fd, LOCK_EX | LOCK_NB) == 0) {
        m_drainLockFd = fd;  // keep fd open for life of process
        m_isDrainWriter = true;
    } else {
        ::close(fd);
    }
}

void DeviceProxy::pollAll() {
    pollPower();
    asyncGetInt("GetVolume", [this](int v) { if (v >= 0) { m_volume = v; emit volumeChanged(); } });
    asyncGetInt("GetSidetone", [this](int v) { if (v >= 0) { m_sidetone = v; emit sidetoneChanged(); } });
    asyncGetInt("GetChatmix", [this](int v) { if (v >= 0) { m_mixamp = v; emit mixampChanged(); } });
    asyncGetInt("GetNoiseGate", [this](int v) { if (v >= 0) { m_noiseGate = v; emit noiseGateChanged(); } });
    asyncGetInt("GetSleepMode", [this](int v) { if (v >= 0) { m_sleepMode = v; emit sleepModeChanged(); } });
    asyncGetInt("GetNotificationSound", [this](int v) { if (v >= 0) { m_notificationSound = v; emit notificationSoundChanged(); } });
    asyncGetInt("GetLedBrightness", [this](int v) { if (v >= 0) { m_ledBrightness = v; emit ledBrightnessChanged(); } });
    asyncGetString("GetDeviceName", [this](const QString& s) { if (!s.isEmpty()) { m_deviceName = s; emit connectedChanged(); } });
    asyncGetString("GetFirmwareVersion", [this](const QString& s) { if (!s.isEmpty()) { m_firmware = s; emit connectedChanged(); } });
    asyncGetString("GetSerialNumber", [this](const QString& s) { if (!s.isEmpty()) { m_serialNumber = s; emit connectedChanged(); } });
    asyncGetInt("GetUptime", [this](int v) { if (v >= 0) { m_uptime = v; emit connectedChanged(); } });
    asyncGetString("GetBluetoothName", [this](const QString& s) { m_btName = s; emit btNameChanged(); });
    loadRouting();

    // Bluetooth status (returns bool from D-Bus)
    auto* btCall = new QDBusPendingCallWatcher(m_iface->asyncCall("GetBluetoothStatus"), this);
    connect(btCall, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<bool> reply = *w;
        if (!reply.isError()) {
            m_btConnected = reply.value();
            emit btConnectedChanged();
        }
        w->deleteLater();
    });

    // Mic mute (returns bool). Ratatoskrd queries HW flip-to-mute via 0c 2b
    // bit 0, so initial state is correct without waiting for a lever transition.
    auto* micCall = new QDBusPendingCallWatcher(m_iface->asyncCall("GetMicMute"), this);
    connect(micCall, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<bool> reply = *w;
        if (!reply.isError()) {
            m_micMuted = reply.value();
            emit micMuteChanged();
        }
        w->deleteLater();
    });

    pollBattery();
}

void DeviceProxy::asyncGetInt(const QString& method, std::function<void(int)> callback) {
    auto* call = new QDBusPendingCallWatcher(m_iface->asyncCall(method), this);
    connect(call, &QDBusPendingCallWatcher::finished, this, [callback](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<int> reply = *w;
        callback(reply.isError() ? -1 : reply.value());
        w->deleteLater();
    });
}

void DeviceProxy::asyncGetString(const QString& method, std::function<void(const QString&)> callback) {
    auto* call = new QDBusPendingCallWatcher(m_iface->asyncCall(method), this);
    connect(call, &QDBusPendingCallWatcher::finished, this, [callback](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<QString> reply = *w;
        callback(reply.isError() ? "" : reply.value());
        w->deleteLater();
    });
}

void DeviceProxy::pollPower() {
    // GetPower returns 1=on, 0=off, -1=unknown. m_connected reflects power state
    // (headset on/off) since basestation always responds even when headset is off.
    asyncGetInt("GetPower", [this](int v) {
        bool wasConnected = m_connected;
        m_connected = (v == 1);
        if (m_connected != wasConnected) {
            emit connectedChanged();
        }
    });
}

void DeviceProxy::pollBattery() {
    // Two callbacks fire in indeterminate order. Use a shared counter so the
    // drain-history sample is logged once both percent and charging are fresh.
    auto pending = std::make_shared<int>(2);
    auto onComplete = [this, pending]() {
        if (--*pending == 0) logBatterySample();
    };
    asyncGetInt("GetBatteryPercent", [this, onComplete](int v) {
        if (v >= 0) {
            m_battery = v;
            emit batteryChanged();
        }
        onComplete();
    });
    asyncGetInt("GetBatteryCharging", [this, onComplete](int v) {
        if (v >= 0) {
            m_charging = (v != 0);
            emit batteryChanged();
        }
        onComplete();
    });
}

void DeviceProxy::refresh() { pollAll(); }

// Steg 2: session-detector with signal-loss heuristic.
// Constants match the spec in ratatoskr_battery.md.
namespace {
constexpr int GRACE_SEC          = 180;       // signal-drop window
constexpr int GAP_SEC            = 600;       // 10 min — split session on data gap
constexpr int PCT_TOLERANCE      = 1;         // Δpct allowed during signal drop
constexpr int MIN_DURATION_SEC   = 30 * 60;   // ≥30 min for a valid session
constexpr int MIN_DRAIN_PCT      = 5;         // ≥5 % drain for a valid session
constexpr int JUMP_PCT           = 5;         // pct jump above this …
constexpr int JUMP_WINDOW_SEC    = 120;       // … within this window aborts session

struct CurSession {
    qint64 start_ts = 0;
    int    start_pct = 0;
    qint64 last_ts  = 0;
    int    last_pct = 0;
    QVariantList signal_drops;
};

QVariantMap finalizeSession(const CurSession& cur, qint64 end_ts, int end_pct) {
    qint64 dur_sec = end_ts - cur.start_ts;
    int drain_pct  = cur.start_pct - end_pct;
    double rate    = (dur_sec > 0) ? (drain_pct * 3600.0 / dur_sec) : 0.0;
    QVariantMap s;
    s["start_ts"]                = static_cast<qlonglong>(cur.start_ts);
    s["end_ts"]                  = static_cast<qlonglong>(end_ts);
    s["start_pct"]               = cur.start_pct;
    s["end_pct"]                 = end_pct;
    s["duration_min"]            = static_cast<int>(dur_sec / 60);
    s["drain_rate_pct_per_hour"] = std::round(rate * 1000.0) / 1000.0;
    s["signal_drops"]            = cur.signal_drops;
    s["valid"]                   = dur_sec >= MIN_DURATION_SEC && drain_pct >= MIN_DRAIN_PCT;
    return s;
}
} // namespace

QVariantList DeviceProxy::detectDrainSessions() {
    QFile file(drainHistoryFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    // Parse JSONL into (ts, pct, charging, powered) tuples.
    struct Sample { qint64 ts; int pct; bool charging; bool powered; };
    QList<Sample> samples;
    samples.reserve(4096);
    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        if (line.isEmpty()) continue;
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;
        QJsonObject o = doc.object();
        Sample s;
        s.ts       = static_cast<qint64>(o["ts"].toVariant().toLongLong());
        s.pct      = o["pct"].toInt();
        s.charging = o["charging"].toBool();
        s.powered  = o["powered"].toBool();
        samples.append(s);
    }
    file.close();
    if (samples.isEmpty()) return {};

    // Defensive sort — append-only writers should keep ts monotonic, but
    // writer-handover or clock skew could break that.
    std::sort(samples.begin(), samples.end(),
              [](const Sample& a, const Sample& b) { return a.ts < b.ts; });

    enum class State { Idle, InSession, PendingOff };
    State state = State::Idle;
    CurSession cur;
    qint64 pending_off_ts = 0;
    int    pending_off_pct = 0;
    QVariantList sessions;

    for (const Sample& s : samples) {
        switch (state) {
        case State::Idle:
            if (s.powered && !s.charging) {
                cur = CurSession{s.ts, s.pct, s.ts, s.pct, {}};
                state = State::InSession;
            }
            break;

        case State::InSession:
            if (s.charging) {
                sessions.append(finalizeSession(cur, cur.last_ts, cur.last_pct));
                state = State::Idle;
            } else if (!s.powered) {
                pending_off_ts = s.ts;
                pending_off_pct = s.pct;
                state = State::PendingOff;
            } else {
                qint64 gap = s.ts - cur.last_ts;
                int pct_jump = s.pct - cur.last_pct;
                if (gap > GAP_SEC) {
                    // Data gap — finalize old, start new.
                    sessions.append(finalizeSession(cur, cur.last_ts, cur.last_pct));
                    cur = CurSession{s.ts, s.pct, s.ts, s.pct, {}};
                } else if (pct_jump > JUMP_PCT && gap < JUMP_WINDOW_SEC) {
                    // Implausible upward jump — likely missed charging event.
                    sessions.append(finalizeSession(cur, cur.last_ts, cur.last_pct));
                    cur = CurSession{s.ts, s.pct, s.ts, s.pct, {}};
                } else {
                    cur.last_ts = s.ts;
                    cur.last_pct = s.pct;
                }
            }
            break;

        case State::PendingOff:
            if (s.charging) {
                // Was off, now charging — confirms real off (docked).
                sessions.append(finalizeSession(cur, pending_off_ts, pending_off_pct));
                state = State::Idle;
            } else if (s.powered) {
                qint64 grace = s.ts - pending_off_ts;
                int pct_diff = std::abs(s.pct - pending_off_pct);
                if (grace < GRACE_SEC && pct_diff <= PCT_TOLERANCE) {
                    // RF dropout — keep session alive, log the drop.
                    QVariantMap drop;
                    drop["ts"]         = static_cast<qlonglong>(pending_off_ts);
                    drop["duration_s"] = static_cast<int>(grace);
                    cur.signal_drops.append(drop);
                    cur.last_ts  = s.ts;
                    cur.last_pct = s.pct;
                    state = State::InSession;
                } else {
                    // Real off then on — finalize previous, start new at this sample.
                    sessions.append(finalizeSession(cur, pending_off_ts, pending_off_pct));
                    cur = CurSession{s.ts, s.pct, s.ts, s.pct, {}};
                    state = State::InSession;
                }
            }
            // If still !powered, stay in PendingOff (timeout-based finalize
            // happens on next powered:true OR end-of-stream).
            break;
        }
    }

    // Flush any in-flight session.
    if (state == State::InSession) {
        sessions.append(finalizeSession(cur, cur.last_ts, cur.last_pct));
    } else if (state == State::PendingOff) {
        sessions.append(finalizeSession(cur, pending_off_ts, pending_off_pct));
    }

    return sessions;
}

void DeviceProxy::logBatterySample() {
    if (!m_isDrainWriter) return;  // another process is the writer
    if (m_battery < 0) return;     // skip until first valid reading
    QJsonObject sample{
        {"ts",       static_cast<qint64>(QDateTime::currentSecsSinceEpoch())},
        {"pct",      m_battery},
        {"charging", m_charging},
        {"powered",  m_connected},
        {"writer",   QStringLiteral("gui")},
    };
    QFile file(drainHistoryFilePath());
    if (!file.open(QIODevice::Append | QIODevice::Text)) return;
    file.write(QJsonDocument(sample).toJson(QJsonDocument::Compact));
    file.write("\n");
}

// D-Bus signal handlers
void DeviceProxy::onBatteryChanged(int percent, bool charging) {
    m_battery = percent;
    m_charging = charging;
    emit batteryChanged();
    logBatterySample();
}

void DeviceProxy::onVolumeChanged(int level) {
    m_volume = level;
    emit volumeChanged();
}

void DeviceProxy::onMicMuteChanged(bool muted) {
    m_micMuted = muted;
    emit micMuteChanged();
}

void DeviceProxy::onMixampChanged(int level) {
    m_mixamp = level;
    emit mixampChanged();
}

void DeviceProxy::onPowerChanged(int state) {
    bool wasConnected = m_connected;
    m_connected = (state == 0);
    emit connectedChanged();
    // Log power transition to mark drain-history session boundaries.
    logBatterySample();
    // Re-poll on power-on so mic mute and other state reflect the newly
    // connected headset rather than stale values from before power-off.
    if (m_connected && !wasConnected) {
        QTimer::singleShot(500, this, &DeviceProxy::pollAll);
    }
}

void DeviceProxy::onBluetoothChanged(bool connected) {
    m_btConnected = connected;
    emit btConnectedChanged();
    // Re-poll BT name when connection state changes
    asyncGetString("GetBluetoothName", [this](const QString& s) { m_btName = s; emit btNameChanged(); });
}

// SET methods — all async to avoid blocking the Qt event loop
void DeviceProxy::setVolume(int level) {
    m_iface->asyncCall("SetVolume", level);
}

void DeviceProxy::setSidetone(int level) {
    m_iface->asyncCall("SetSidetone", level);
}

void DeviceProxy::setMixamp(int level) {
    m_iface->asyncCall("SetMixamp", level);
}

void DeviceProxy::setNoiseGate(int mode) {
    m_iface->asyncCall("SetNoiseGate", mode);
}

void DeviceProxy::setSleepMode(int minutes) {
    m_iface->asyncCall("SetInactiveTime", minutes);
}

void DeviceProxy::setNotificationSound(int level) {
    m_iface->asyncCall("SetNotificationSound", level);
}

void DeviceProxy::setLedBrightness(int brightness) {
    m_iface->asyncCall("SetLedBrightness", brightness);
}

void DeviceProxy::setEqualizerPreset(int preset) {
    m_iface->asyncCall("SetEqualizerPreset", preset);
}

void DeviceProxy::factoryReset() {
    // Daemon requires the current serial number as confirmation so a
    // mis-fired call cannot wipe a device the caller didn't mean to target.
    if (m_serialNumber.isEmpty()) return;
    m_iface->asyncCall("FactoryReset", m_serialNumber);
}

void DeviceProxy::loadRouting() {
    auto* call = new QDBusPendingCallWatcher(m_iface->asyncCall("GetRouting"), this);
    connect(call, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<int, bool, int, bool, int, bool, int, bool, int, bool> reply = *w;
        if (!reply.isError()) {
            m_routeStreamVol = reply.argumentAt(0).toInt();
            m_routeStreamMute = reply.argumentAt(1).toBool();
            m_routeMicVol = reply.argumentAt(2).toInt();
            m_routeMicMute = reply.argumentAt(3).toBool();
            m_routeGameVol = reply.argumentAt(4).toInt();
            m_routeGameMute = reply.argumentAt(5).toBool();
            m_routeBtVol = reply.argumentAt(6).toInt();
            m_routeBtMute = reply.argumentAt(7).toBool();
            m_routeVoiceVol = reply.argumentAt(8).toInt();
            m_routeVoiceMute = reply.argumentAt(9).toBool();
            emit routingChanged();
        }
        w->deleteLater();
    });
}

void DeviceProxy::setRouting(int streamVol, bool streamMute,
                              int micVol, bool micMute,
                              int gameVol, bool gameMute,
                              int btVol, bool btMute,
                              int voiceVol, bool voiceMute) {
    m_iface->asyncCall("SetRouting", streamVol, streamMute,
                       micVol, micMute, gameVol, gameMute,
                       btVol, btMute, voiceVol, voiceMute);
    // Optimistic update
    m_routeStreamVol = streamVol; m_routeStreamMute = streamMute;
    m_routeMicVol = micVol;       m_routeMicMute = micMute;
    m_routeGameVol = gameVol;     m_routeGameMute = gameMute;
    m_routeBtVol = btVol;         m_routeBtMute = btMute;
    m_routeVoiceVol = voiceVol;   m_routeVoiceMute = voiceMute;
    emit routingChanged();
}

void DeviceProxy::setCustomEqualizer(int type, QVariantList bands) {
    // bands: 10 elements, each either a number (gain dB, default Q=1.0)
    //        or an object {gain: float, q: float, freq: int (optional)}
    if (bands.size() != 10) return;

    const int* defaultFreqs = (type == 0) ? MIC_FREQS : HEADPHONE_FREQS;
    QByteArray bandData(50, 0);

    for (int i = 0; i < 10; i++) {
        double gain_db = 0.0;
        double q_factor = 1.0;
        int freq = defaultFreqs[i];

        QVariant v = bands[i];
        if (v.typeId() == QMetaType::QVariantMap) {
            QVariantMap m = v.toMap();
            gain_db = m.value("gain", 0.0).toDouble();
            q_factor = m.value("q", 1.0).toDouble();
            if (m.contains("freq"))
                freq = m.value("freq").toInt();
        } else {
            gain_db = v.toDouble();
        }

        gain_db = qBound(-6.0, gain_db, 6.0);
        auto gain_byte = static_cast<uint8_t>(qBound(0, qRound(EQ_GAIN_CENTER + gain_db * 20), 240));
        uint8_t q_byte = qFactorToByte(q_factor);
        int off = i * 5;
        bandData[off]     = static_cast<char>(freq >> 8);
        bandData[off + 1] = static_cast<char>(freq & 0xFF);
        bandData[off + 2] = static_cast<char>(q_byte);
        bandData[off + 3] = 0;
        bandData[off + 4] = static_cast<char>(gain_byte);
    }

    m_iface->asyncCall("SetCustomEqualizer", type, bandData);
}

void DeviceProxy::requestActiveEqualizerData(int type) {
    auto* call = new QDBusPendingCallWatcher(
        m_iface->asyncCall("GetActiveEqualizerData", type), this);
    connect(call, &QDBusPendingCallWatcher::finished, this,
            [this, type](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<QByteArray> reply = *w;
        QVariantList out;
        if (!reply.isError()) {
            const QByteArray& data = reply.value();
            for (int i = 0; i < data.size(); i++) {
                out.append(static_cast<int>(static_cast<uint8_t>(data[i])));
            }
        }
        emit activeEqualizerDataReady(type, out);
        w->deleteLater();
    });
}

QVariantList DeviceProxy::getEqualizerPresetData(int type, int index) {
    QDBusReply<QByteArray> reply = m_iface->call("GetEqualizerPresetData", type, index);
    QVariantList out;
    if (!reply.isValid()) return out;
    const QByteArray& data = reply.value();
    for (int i = 0; i < data.size(); i++) {
        out.append(static_cast<int>(static_cast<uint8_t>(data[i])));
    }
    return out;
}

int DeviceProxy::getEqualizerPresetCount(int type) {
    QDBusReply<int> reply = m_iface->call("GetEqualizerPresetCount", type);
    return reply.isValid() ? reply.value() : -1;
}

// EQ preset persistence (~/.config/ratatoskr/eq-presets.json)
// Format: {"presets": [...], "deleted": ["name1", "name2"]}
static QJsonObject readPresetsFile() {
    QFile file(presetsFilePath());
    if (!file.open(QIODevice::ReadOnly)) return {};
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    // Migrate old array format to new object format
    if (doc.isArray())
        return QJsonObject{{"presets", doc.array()}, {"deleted", QJsonArray()}};
    return doc.object();
}

static void writePresetsFile(const QJsonObject& obj) {
    QFile file(presetsFilePath());
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QVariantList DeviceProxy::loadEqPresets() {
    return readPresetsFile()["presets"].toArray().toVariantList();
}

void DeviceProxy::saveEqPreset(const QString& name, int type, const QVariantList& bands) {
    QJsonObject root = readPresetsFile();
    QJsonArray presets = root["presets"].toArray();
    QJsonArray filtered;
    for (const auto& p : presets) {
        if (p.toObject()["name"].toString() != name)
            filtered.append(p);
    }
    QJsonObject entry;
    entry["name"] = name;
    entry["type"] = type;
    entry["bands"] = QJsonArray::fromVariantList(bands);
    filtered.append(entry);
    // Remove from deleted list if re-saving
    QJsonArray deleted = root["deleted"].toArray();
    QJsonArray newDeleted;
    for (const auto& d : deleted) {
        if (d.toString() != name) newDeleted.append(d);
    }
    root["presets"] = filtered;
    root["deleted"] = newDeleted;
    writePresetsFile(root);
}

void DeviceProxy::deleteEqPreset(const QString& name) {
    QJsonObject root = readPresetsFile();
    QJsonArray presets = root["presets"].toArray();
    QJsonArray filtered;
    for (const auto& p : presets) {
        if (p.toObject()["name"].toString() != name)
            filtered.append(p);
    }
    // Track deleted name so seed doesn't re-add it
    QJsonArray deleted = root["deleted"].toArray();
    deleted.append(name);
    root["presets"] = filtered;
    root["deleted"] = deleted;
    writePresetsFile(root);
}

void DeviceProxy::moveEqPreset(const QString& name, int direction) {
    QJsonObject root = readPresetsFile();
    QJsonArray presets = root["presets"].toArray();
    int idx = -1;
    for (int i = 0; i < presets.size(); i++) {
        if (presets[i].toObject()["name"].toString() == name) { idx = i; break; }
    }
    int target = idx + direction;
    if (idx < 0 || target < 0 || target >= presets.size()) return;
    QJsonValue tmp = presets[idx];
    presets[idx] = presets[target];
    presets[target] = tmp;
    root["presets"] = presets;
    writePresetsFile(root);
}

QStringList DeviceProxy::deletedEqPresets() {
    QJsonArray deleted = readPresetsFile()["deleted"].toArray();
    QStringList result;
    for (const auto& d : deleted) result.append(d.toString());
    return result;
}

void DeviceProxy::renameEqPreset(const QString& oldName, const QString& newName) {
    QJsonObject root = readPresetsFile();
    QJsonArray presets = root["presets"].toArray();
    QJsonArray updated;
    for (const auto& p : presets) {
        QJsonObject m = p.toObject();
        if (m["name"].toString() == oldName)
            m["name"] = newName;
        updated.append(m);
    }
    // Mark old name as deleted so seed doesn't re-add it
    QJsonArray deleted = root["deleted"].toArray();
    deleted.append(oldName);
    root["presets"] = updated;
    root["deleted"] = deleted;
    writePresetsFile(root);
}
