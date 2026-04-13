#include "pulseaudiosource.h"
#include <cstring>

PulseAudioSource::PulseAudioSource(QObject* parent) : QObject(parent) {
    connectToPulse();
}

PulseAudioSource::~PulseAudioSource() {
    if (m_mainloop) {
        pa_threaded_mainloop_stop(m_mainloop);
        if (m_context) {
            pa_context_disconnect(m_context);
            pa_context_unref(m_context);
        }
        pa_threaded_mainloop_free(m_mainloop);
    }
}

void PulseAudioSource::connectToPulse() {
    m_mainloop = pa_threaded_mainloop_new();
    if (!m_mainloop) return;

    pa_mainloop_api* api = pa_threaded_mainloop_get_api(m_mainloop);
    m_context = pa_context_new(api, "ratatoskr-gui");
    if (!m_context) return;

    pa_context_set_state_callback(m_context, onContextState, this);
    pa_context_connect(m_context, nullptr, PA_CONTEXT_NOFAIL, nullptr);
    pa_threaded_mainloop_start(m_mainloop);
}

void PulseAudioSource::onContextState(pa_context* ctx, void* userdata) {
    auto* self = static_cast<PulseAudioSource*>(userdata);
    pa_context_state_t state = pa_context_get_state(ctx);

    if (state == PA_CONTEXT_READY) {
        // Subscribe to source events for live updates
        pa_context_set_subscribe_callback(ctx, onSubscribeEvent, self);
        pa_context_subscribe(ctx, PA_SUBSCRIPTION_MASK_SOURCE, nullptr, nullptr);
        // Find A50 source
        self->findSource();
    }
}

void PulseAudioSource::findSource() {
    pa_context_get_source_info_list(m_context, onSourceInfo, this);
}

void PulseAudioSource::onSourceInfo(pa_context* ctx, const pa_source_info* info, int eol, void* userdata) {
    if (eol > 0 || !info) return;
    auto* self = static_cast<PulseAudioSource*>(userdata);

    // Match A50 input source (not monitor)
    if (info->name && std::strstr(info->name, "alsa_input") &&
        (std::strstr(info->name, "Logitech_A50") || std::strstr(info->name, "Astro"))) {

        self->m_sourceIndex = info->index;
        self->m_sourceName = QString::fromUtf8(info->description);
        self->m_channels = info->volume.channels;

        // Convert PA volume to percent
        pa_volume_t avg = pa_cvolume_avg(&info->volume);
        self->m_volume = qRound(avg * 100.0 / PA_VOLUME_NORM);
        self->m_muted = info->mute != 0;
        self->m_available = true;

        QMetaObject::invokeMethod(self, "availableChanged", Qt::QueuedConnection);
        QMetaObject::invokeMethod(self, "volumeChanged", Qt::QueuedConnection);
        QMetaObject::invokeMethod(self, "mutedChanged", Qt::QueuedConnection);
    }
}

void PulseAudioSource::onSubscribeEvent(pa_context* ctx, pa_subscription_event_type_t type, uint32_t idx, void* userdata) {
    auto* self = static_cast<PulseAudioSource*>(userdata);
    unsigned facility = type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    unsigned event = type & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

    if (facility == PA_SUBSCRIPTION_EVENT_SOURCE &&
        (event == PA_SUBSCRIPTION_EVENT_CHANGE || event == PA_SUBSCRIPTION_EVENT_NEW)) {
        // Re-query to get updated volume/mute
        if (self->m_sourceIndex != PA_INVALID_INDEX && idx == self->m_sourceIndex) {
            self->querySource();
        } else if (event == PA_SUBSCRIPTION_EVENT_NEW) {
            // New source appeared — might be A50 reconnect
            self->findSource();
        }
    } else if (facility == PA_SUBSCRIPTION_EVENT_SOURCE && event == PA_SUBSCRIPTION_EVENT_REMOVE) {
        if (self->m_sourceIndex != PA_INVALID_INDEX && idx == self->m_sourceIndex) {
            self->m_available = false;
            self->m_sourceIndex = PA_INVALID_INDEX;
            QMetaObject::invokeMethod(self, "availableChanged", Qt::QueuedConnection);
        }
    }
}

void PulseAudioSource::querySource() {
    if (m_sourceIndex == PA_INVALID_INDEX) return;
    pa_context_get_source_info_by_index(m_context, m_sourceIndex, onSourceInfo, this);
}

void PulseAudioSource::onSuccess(pa_context*, int, void*) {}

void PulseAudioSource::setVolume(int percent) {
    if (m_sourceIndex == PA_INVALID_INDEX || !m_context) return;

    percent = qBound(0, percent, 150);
    pa_volume_t vol = PA_VOLUME_NORM * percent / 100;

    pa_cvolume cvol;
    pa_cvolume_set(&cvol, m_channels, vol);

    pa_threaded_mainloop_lock(m_mainloop);
    pa_context_set_source_volume_by_index(m_context, m_sourceIndex, &cvol, onSuccess, this);
    pa_threaded_mainloop_unlock(m_mainloop);

    m_volume = percent;
    emit volumeChanged();
}

void PulseAudioSource::setMuted(bool mute) {
    if (m_sourceIndex == PA_INVALID_INDEX || !m_context) return;

    pa_threaded_mainloop_lock(m_mainloop);
    pa_context_set_source_mute_by_index(m_context, m_sourceIndex, mute ? 1 : 0, onSuccess, this);
    pa_threaded_mainloop_unlock(m_mainloop);

    m_muted = mute;
    emit mutedChanged();
}
