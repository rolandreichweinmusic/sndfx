#include "ADC.h"
#include "AudioDevice.h"
#include <alsa/asoundlib.h>

AudioDevice ADC::_device;

ADC::ADC() {}

void ADC::process() {
    snd_pcm_t* pcm = _device.capture();
    snd_pcm_sframes_t frames = snd_pcm_readi(pcm, _buffer.data(), _buffer.size());
    if (frames < 0) {
        snd_pcm_recover(pcm, static_cast<int>(frames), 0);
        snd_pcm_readi(pcm, _buffer.data(), _buffer.size());
    }
}
