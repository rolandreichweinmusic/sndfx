#include "DAC.h"

#include "ADC.h"
#include <alsa/asoundlib.h>

DAC::DAC(Operation& input) : _device(ADC::device()), input(input) {}

void DAC::process() {
    input.process();
    _buffer = input.getBuffer();

    snd_pcm_t* pcm = _device.playback();
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm, _buffer.data(), _buffer.size());
    if (frames < 0) {
        snd_pcm_recover(pcm, static_cast<int>(frames), 0);
        snd_pcm_writei(pcm, _buffer.data(), _buffer.size());
    }
}
