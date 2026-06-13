#include "AudioDevice.h"
#include <stdexcept>

AudioDevice::AudioDevice(const char* device, unsigned int rate, unsigned int channels) {
    int err = snd_pcm_open(&_capture, device, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0)
        throw std::runtime_error(snd_strerror(err));

    err = snd_pcm_open(&_playback, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
        throw std::runtime_error(snd_strerror(err));

    configure(_capture, rate, channels);
    configure(_playback, rate, channels);

    // Link streams for synchronized start
    err = snd_pcm_link(_capture, _playback);
    if (err < 0)
        throw std::runtime_error(snd_strerror(err));

    snd_pcm_prepare(_capture);
}

AudioDevice::~AudioDevice() {
    if (_capture) {
        snd_pcm_unlink(_capture);
        snd_pcm_drop(_capture);
        snd_pcm_close(_capture);
    }
    if (_playback) {
        snd_pcm_drain(_playback);
        snd_pcm_close(_playback);
    }
}

void AudioDevice::configure(snd_pcm_t* pcm, unsigned int rate, unsigned int channels) {
    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm, params);
    snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S32_LE);
    snd_pcm_hw_params_set_channels(pcm, params, channels);
    snd_pcm_hw_params_set_rate_near(pcm, params, &rate, nullptr);

    int err = snd_pcm_hw_params(pcm, params);
    if (err < 0)
        throw std::runtime_error(snd_strerror(err));
}
