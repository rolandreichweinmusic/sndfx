#pragma once

#include <alsa/asoundlib.h>

class AudioDevice {
public:
    // "plughw" (not raw "hw") so ALSA converts the device's native channel
    // count / format / rate to the mono S32 48 kHz this pipeline assumes. A raw
    // "hw" device does no conversion, so on a stereo-only interface like the
    // UR22C the streams would silently run in stereo and the mono read/write
    // loop would misread the interleaved samples -> heavy distortion.
    AudioDevice(const char* device = "plughw:CARD=UR22C,DEV=0",
                unsigned int rate = 48000,
                unsigned int channels = 1);
    ~AudioDevice();

    snd_pcm_t* capture() { return _capture; }
    snd_pcm_t* playback() { return _playback; }

private:
    void configure(snd_pcm_t* pcm, unsigned int rate, unsigned int channels);
    void primePlayback(unsigned int channels);

    snd_pcm_t* _capture = nullptr;
    snd_pcm_t* _playback = nullptr;
};
