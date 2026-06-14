#include "AudioDevice.h"
#include <etl/array.h>
#include <etl/exception.h>
#include <cerrno>
#include <cstdint>

namespace {
// Period matched to Operation::bufferSize (the per-call transfer size), with a
// few periods of headroom in the ring buffer to absorb scheduling jitter. An
// undersized buffer is the usual cause of xruns in a synchronous loop.
constexpr snd_pcm_uframes_t framesPerPeriod = 256;
constexpr unsigned int periodsPerBuffer = 4;

// Upper bound on channels for the stack-allocated silence buffer used to prime
// playback. The Linux test harness runs mono/stereo.
constexpr unsigned int maxChannels = 2;

// ETL-style exception for ALSA initialisation failures. snd_strerror() returns
// pointers to static strings, so storing the reason pointer is safe.
class audio_device_error : public etl::exception {
public:
    audio_device_error(string_type reason, string_type file, numeric_type line)
        : exception(reason, file, line) {}
};
} // namespace

AudioDevice::AudioDevice(const char* device, unsigned int rate, unsigned int channels) {
    int err = snd_pcm_open(&_capture, device, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0)
        throw audio_device_error(snd_strerror(err), __FILE__, __LINE__);

    err = snd_pcm_open(&_playback, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
        throw audio_device_error(snd_strerror(err), __FILE__, __LINE__);

    configure(_capture, rate, channels);
    configure(_playback, rate, channels);

    // Link streams for synchronized start. Not every device supports this:
    // dmix/dsnoop and the PulseAudio plugin return -ENOSYS ("Function not
    // implemented"). Linking is only an optimization, so fall back to starting
    // the streams independently instead of aborting.
    err = snd_pcm_link(_capture, _playback);
    if (err < 0 && err != -ENOSYS)
        throw audio_device_error(snd_strerror(err), __FILE__, __LINE__);
    bool linked = (err >= 0);

    snd_pcm_prepare(_capture);
    if (!linked)
        snd_pcm_prepare(_playback); // not linked: prepare playback separately

    // Fill the playback buffer with silence before the streams run. In this
    // synchronous capture->process->playback loop the first read blocks while a
    // whole period is captured; without a cushion the playback ring empties in
    // that window and underruns immediately, which then stalls the loop long
    // enough for the capture ring to overrun too.
    primePlayback(channels);

    // Start explicitly (rather than on the first read/write) so playback begins
    // with the silence cushion we just queued. When linked, starting capture
    // starts playback as well.
    snd_pcm_start(_capture);
    if (!linked)
        snd_pcm_start(_playback);
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

    // Pin the period and buffer sizes; the _near calls write back the values
    // actually selected by the device.
    snd_pcm_uframes_t period = framesPerPeriod;
    snd_pcm_hw_params_set_period_size_near(pcm, params, &period, nullptr);
    snd_pcm_uframes_t buffer = period * periodsPerBuffer;
    snd_pcm_hw_params_set_buffer_size_near(pcm, params, &buffer);

    int err = snd_pcm_hw_params(pcm, params);
    if (err < 0)
        throw audio_device_error(snd_strerror(err), __FILE__, __LINE__);

    // Only start a stream when explicitly told to, so priming the playback
    // buffer with silence cannot trip an early auto-start.
    snd_pcm_sw_params_t* sw;
    snd_pcm_sw_params_alloca(&sw);
    snd_pcm_sw_params_current(pcm, sw);
    snd_pcm_sw_params_set_start_threshold(pcm, sw, buffer);
    snd_pcm_sw_params_set_avail_min(pcm, sw, period);
    err = snd_pcm_sw_params(pcm, sw);
    if (err < 0)
        throw audio_device_error(snd_strerror(err), __FILE__, __LINE__);
}

void AudioDevice::primePlayback(unsigned int channels) {
    snd_pcm_uframes_t buffer = 0;
    snd_pcm_uframes_t period = 0;
    if (snd_pcm_get_params(_playback, &buffer, &period) < 0 || period == 0)
        return;

    // One period of silence on the stack. We pinned the period to
    // framesPerPeriod and expect at most maxChannels, so a fixed-capacity ETL
    // array suffices; bail out rather than overrun it if the device granted an
    // unexpected geometry.
    if (period > framesPerPeriod || channels > maxChannels)
        return;

    // Queue silence up to one period short of full, leaving the explicit start
    // (start threshold == full buffer) as the single trigger for playback.
    etl::array<int32_t, framesPerPeriod * maxChannels> silence{};
    for (snd_pcm_uframes_t primed = period; primed < buffer; primed += period) {
        snd_pcm_sframes_t written = snd_pcm_writei(_playback, silence.data(), period);
        if (written < 0) {
            snd_pcm_recover(_playback, static_cast<int>(written), 1);
            break;
        }
    }
}
