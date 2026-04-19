#pragma once

#include <QObject>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QTimer>
#include <QVariantList>
#include <qqmlintegration.h>
#include <functional>

class DeviceProxy : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int battery READ battery NOTIFY batteryChanged)
    Q_PROPERTY(bool charging READ charging NOTIFY batteryChanged)
    Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(int sidetone READ sidetone NOTIFY sidetoneChanged)
    Q_PROPERTY(bool micMuted READ micMuted NOTIFY micMuteChanged)
    Q_PROPERTY(int mixamp READ mixamp NOTIFY mixampChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY connectedChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(int noiseGate READ noiseGate NOTIFY noiseGateChanged)
    Q_PROPERTY(int sleepMode READ sleepMode NOTIFY sleepModeChanged)
    Q_PROPERTY(int notificationSound READ notificationSound NOTIFY notificationSoundChanged)
    Q_PROPERTY(int ledBrightness READ ledBrightness NOTIFY ledBrightnessChanged)
    Q_PROPERTY(QString firmware READ firmware NOTIFY connectedChanged)
    Q_PROPERTY(QString serialNumber READ serialNumber NOTIFY connectedChanged)
    Q_PROPERTY(int uptime READ uptime NOTIFY connectedChanged)
    Q_PROPERTY(bool btConnected READ btConnected NOTIFY btConnectedChanged)
    // Stream routing
    Q_PROPERTY(int routeStreamVol READ routeStreamVol NOTIFY routingChanged)
    Q_PROPERTY(bool routeStreamMute READ routeStreamMute NOTIFY routingChanged)
    Q_PROPERTY(int routeMicVol READ routeMicVol NOTIFY routingChanged)
    Q_PROPERTY(bool routeMicMute READ routeMicMute NOTIFY routingChanged)
    Q_PROPERTY(int routeGameVol READ routeGameVol NOTIFY routingChanged)
    Q_PROPERTY(bool routeGameMute READ routeGameMute NOTIFY routingChanged)
    Q_PROPERTY(int routeBtVol READ routeBtVol NOTIFY routingChanged)
    Q_PROPERTY(bool routeBtMute READ routeBtMute NOTIFY routingChanged)
    Q_PROPERTY(int routeVoiceVol READ routeVoiceVol NOTIFY routingChanged)
    Q_PROPERTY(bool routeVoiceMute READ routeVoiceMute NOTIFY routingChanged)
    Q_PROPERTY(QString btName READ btName NOTIFY btNameChanged)

public:
    explicit DeviceProxy(QObject* parent = nullptr);

    int battery() const { return m_battery; }
    bool charging() const { return m_charging; }
    int volume() const { return m_volume; }
    int sidetone() const { return m_sidetone; }
    bool micMuted() const { return m_micMuted; }
    int mixamp() const { return m_mixamp; }
    QString deviceName() const { return m_deviceName; }
    bool connected() const { return m_connected; }
    int noiseGate() const { return m_noiseGate; }
    int sleepMode() const { return m_sleepMode; }
    int notificationSound() const { return m_notificationSound; }
    int ledBrightness() const { return m_ledBrightness; }
    QString firmware() const { return m_firmware; }
    QString serialNumber() const { return m_serialNumber; }
    int uptime() const { return m_uptime; }
    bool btConnected() const { return m_btConnected; }
    int routeStreamVol() const { return m_routeStreamVol; }
    bool routeStreamMute() const { return m_routeStreamMute; }
    int routeMicVol() const { return m_routeMicVol; }
    bool routeMicMute() const { return m_routeMicMute; }
    int routeGameVol() const { return m_routeGameVol; }
    bool routeGameMute() const { return m_routeGameMute; }
    int routeBtVol() const { return m_routeBtVol; }
    bool routeBtMute() const { return m_routeBtMute; }
    int routeVoiceVol() const { return m_routeVoiceVol; }
    bool routeVoiceMute() const { return m_routeVoiceMute; }
    QString btName() const { return m_btName; }

    Q_INVOKABLE void setVolume(int level);
    Q_INVOKABLE void setSidetone(int level);
    Q_INVOKABLE void setMixamp(int level);
    Q_INVOKABLE void setNoiseGate(int mode);
    Q_INVOKABLE void setSleepMode(int minutes);
    Q_INVOKABLE void setNotificationSound(int level);
    Q_INVOKABLE void setLedBrightness(int brightness);
    Q_INVOKABLE void setEqualizerPreset(int preset);
    Q_INVOKABLE void setCustomEqualizer(int type, QVariantList bands);
    Q_INVOKABLE void factoryReset();
    Q_INVOKABLE void loadRouting();
    Q_INVOKABLE void setRouting(int streamVol, bool streamMute,
                                int micVol, bool micMute,
                                int gameVol, bool gameMute,
                                int btVol, bool btMute,
                                int voiceVol, bool voiceMute);
    Q_INVOKABLE QVariantList loadEqPresets();
    Q_INVOKABLE void saveEqPreset(const QString& name, int type, const QVariantList& bands);
    Q_INVOKABLE void deleteEqPreset(const QString& name);
    Q_INVOKABLE void renameEqPreset(const QString& oldName, const QString& newName);
    Q_INVOKABLE void moveEqPreset(const QString& name, int direction);
    Q_INVOKABLE QStringList deletedEqPresets();
    Q_INVOKABLE void refresh();

signals:
    void batteryChanged();
    void volumeChanged();
    void sidetoneChanged();
    void micMuteChanged();
    void mixampChanged();
    void connectedChanged();
    void noiseGateChanged();
    void sleepModeChanged();
    void notificationSoundChanged();
    void ledBrightnessChanged();
    void btConnectedChanged();
    void btNameChanged();
    void routingChanged();

private slots:
    void onBatteryChanged(int percent, bool charging);
    void onVolumeChanged(int level);
    void onMicMuteChanged(bool muted);
    void onMixampChanged(int level);
    void onPowerChanged(int state);
    void onBluetoothChanged(bool connected);

private:
    void pollAll();
    void pollBattery();
    void asyncGetInt(const QString& method, std::function<void(int)> callback);
    void asyncGetString(const QString& method, std::function<void(const QString&)> callback);

    QDBusInterface* m_iface = nullptr;

    int m_battery = -1;
    bool m_charging = false;
    int m_volume = -1;
    int m_sidetone = -1;
    bool m_micMuted = false;
    int m_mixamp = -1;
    QString m_deviceName;
    bool m_connected = false;
    int m_noiseGate = -1;
    int m_sleepMode = -1;
    int m_notificationSound = -1;
    int m_ledBrightness = -1;
    QString m_firmware;
    QString m_serialNumber;
    int m_uptime = -1;
    bool m_btConnected = false;
    QString m_btName;
    int m_routeStreamVol = -1; bool m_routeStreamMute = false;
    int m_routeMicVol = -1;    bool m_routeMicMute = true;
    int m_routeGameVol = -1;   bool m_routeGameMute = false;
    int m_routeBtVol = -1;     bool m_routeBtMute = true;
    int m_routeVoiceVol = -1;  bool m_routeVoiceMute = false;
};
