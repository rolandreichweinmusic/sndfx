#pragma once

#include <alsa/asoundlib.h>

class AudioDevice {
public:
    AudioDevice(const char* device = "hw:CARD=UR22C,DEV=0",
                unsigned int rate = 48000,
                unsigned int channels = 1);
    ~AudioDevice();

    snd_pcm_t* capture() { return _capture; }
    snd_pcm_t* playback() { return _playback; }

private:
    void configure(snd_pcm_t* pcm, unsigned int rate, unsigned int channels);

    snd_pcm_t* _capture = nullptr;
    snd_pcm_t* _playback = nullptr;
};
