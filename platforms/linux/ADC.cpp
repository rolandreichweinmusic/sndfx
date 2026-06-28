#include "ADC.h"
#include "CPULoad.h"
#include "AudioDevice.h"
#include <alsa/asoundlib.h>

ADC::ADC() {}

void ADC::process() {
    snd_pcm_t* pcm = device().capture();
    {
        CPULoad::IdleGuard idleGuard(CPULoad::instance());
        snd_pcm_wait(pcm, 1000);
    }
    snd_pcm_sframes_t frames = snd_pcm_readi(pcm, _buffer.data(), _buffer.size());
    if (frames < 0) {
        snd_pcm_recover(pcm, static_cast<int>(frames), 0);
        snd_pcm_readi(pcm, _buffer.data(), _buffer.size());
    }
}
