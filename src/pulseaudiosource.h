#pragma once

#include <QObject>
#include <QString>
#include <qqmlintegration.h>
#include <pulse/pulseaudio.h>

class PulseAudioSource : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QString sourceName READ sourceName NOTIFY availableChanged)

public:
    explicit PulseAudioSource(QObject* parent = nullptr);
    ~PulseAudioSource();

    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    bool available() const { return m_available; }
    QString sourceName() const { return m_sourceName; }

    Q_INVOKABLE void setVolume(int percent);
    Q_INVOKABLE void setMuted(bool mute);

signals:
    void volumeChanged();
    void mutedChanged();
    void availableChanged();

private:
    void connectToPulse();
    void findSource();
    void querySource();

    // PulseAudio callbacks (static, forward to instance)
    static void onContextState(pa_context* ctx, void* userdata);
    static void onSourceInfo(pa_context* ctx, const pa_source_info* info, int eol, void* userdata);
    static void onSubscribeEvent(pa_context* ctx, pa_subscription_event_type_t type, uint32_t idx, void* userdata);
    static void onSuccess(pa_context* ctx, int success, void* userdata);

    pa_threaded_mainloop* m_mainloop = nullptr;
    pa_context* m_context = nullptr;

    int m_volume = 100;
    bool m_muted = false;
    bool m_available = false;
    QString m_sourceName;
    uint32_t m_sourceIndex = PA_INVALID_INDEX;
    int m_channels = 1;
};
