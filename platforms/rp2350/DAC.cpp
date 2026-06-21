// PCM5102A

#include "DAC.h"
#include "i2s_output.pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"

// ---------------------------------------------------------------------------
// DIAGNOSTIC: set to 1 to run the ADC-liveness monitor; set to 0 for normal
// passthrough.
//
// The tone test already proved the output chain (PIO -> DMA -> completion IRQ
// re-arm -> PCM5102A -> wiring -> speaker) is healthy, so the remaining silence
// is in the capture path. This monitor makes the ADC's status audible:
//
//   * You hear LIVE INPUT audio -> the ADC is capturing. The original silence
//     came from the blocking capture wait stalling the loop; the non-blocking
//     ADC::process() is the real fix -- make it permanent (set this to 0).
//   * You hear a repeating pattern of N short BEEPS -> no capture has completed;
//     N identifies the first dead link in the input clock chain:
//        1 beep  -> SCKI (GPIO5) not toggling: RP2350 master-clock output dead
//                   or unwired.
//        2 beeps -> SCKI ok but BCK (GPIO3) static: PCM1808 is not producing
//                   clocks -> master-mode straps MD0/MD1 or the SCKI wire to
//                   the codec.
//        3 beeps -> BCK ok but LRCK (GPIO4) static: LRCK wiring.
//   * You hear a STEADY tone -> all input clocks are present but capture still
//     never completes: data line (GPIO2) wiring or PIO routing.
// ---------------------------------------------------------------------------
#define SNDFX_DAC_ADC_MONITOR 1

// Defined in ADC.cpp: capture DMA completion count (0 => input not clocking) and
// the input-clock detector result (0 = clocks present, 1/2/3 = first dead link).
extern volatile uint32_t g_adcCaptureCompletions;
extern volatile uint32_t g_adcClockDiag;

namespace {
// ~1 kHz square wave at 48 kHz: 24 samples high + 24 low = 48-sample period.
// Amplitude is well below full scale (2^31) to stay at a safe listening level.
constexpr Operation::SampleType kToneAmplitude = 1 << 26; // ~ -30 dBFS
constexpr unsigned kToneHalfPeriod = 24;
} // namespace

DAC* DAC::_instance = nullptr;

DAC::DAC(Operation& input, PIO pio, uint sm, uint data_pin, uint bclk_pin)
    : input(input), _pio(pio), _sm(sm) {
    _instance = this;

    // Both staging buffers start silent so the bit clock can free-run from the
    // very first frame while the pipeline fills in real audio.
    _stereo[0].fill(0);
    _stereo[1].fill(0);

    // Load PIO program
    _offset = pio_add_program(_pio, &i2s_output_program);

    // Configure PIO state machine for I2S output.
    // Philips I2S format: data is shifted out MSB-first within a 32-bit slot,
    // with the MSB delayed one BCLK after the LRCLK edge. That 1-bit delay is
    // produced inside the PIO program (i2s_output.pio); here we only set the
    // pins, shift direction and width.
    pio_sm_config c = i2s_output_program_get_default_config(_offset);
    sm_config_set_out_pins(&c, data_pin, 1);
    sm_config_set_sideset_pins(&c, bclk_pin);
    sm_config_set_out_shift(&c, false, true, 32); // shift left (MSB-first), autopull at 32 bits
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    // Configure pins
    pio_gpio_init(_pio, data_pin);
    pio_gpio_init(_pio, bclk_pin);
    pio_gpio_init(_pio, bclk_pin + 1); // LRCLK
    pio_sm_set_consecutive_pindirs(_pio, _sm, data_pin, 1, true);   // data out
    pio_sm_set_consecutive_pindirs(_pio, _sm, bclk_pin, 2, true);   // bclk, lrclk out

    // Clock divider for 48kHz sample rate
    // PIO runs 2 cycles per bit, 64 bits per frame (32L + 32R)
    // BCLK = 48000 * 64 = 3.072 MHz
    // PIO clock = 2 * BCLK = 6.144 MHz
    // div = 150 MHz / 6.144 MHz ≈ 24.41
    constexpr float sys_clk_hz = 150000000.0f; // RP2350 default
    float div = sys_clk_hz / (48000.0f * 64.0f * 2.0f);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(_pio, _sm, _offset, &c);

    // Configure DMA for continuous double-buffered playback. The channel
    // streams one stereo buffer, then the completion ISR re-arms it with the
    // other buffer, so the TX FIFO (and therefore BCLK/LRCLK) never stalls.
    _dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dma_cfg = dma_channel_get_default_config(_dma_chan);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_cfg, true);
    channel_config_set_write_increment(&dma_cfg, false);
    channel_config_set_dreq(&dma_cfg, pio_get_dreq(_pio, _sm, true));

    dma_channel_configure(
        _dma_chan,
        &dma_cfg,
        &_pio->txf[_sm],          // write to PIO TX FIFO
        _stereo[0].data(),        // read from the first staging buffer
        _stereo[0].size(),        // transfer count (two words per frame)
        false                     // started below, after the IRQ is wired
    );

    // Buffer 0 plays first (silence). The first completion ISR frees it and
    // hands process() the buffer to fill, so startup is driven entirely by the
    // ISR (no race on the flag against the constructor's initial values).
    _playIndex = 0;
    _freeIndex = 1;
    _freeReady = false;

    // Route this channel's completion to DMA_IRQ_1 and install the handler.
    dma_channel_set_irq1_enabled(_dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_1, &DAC::dma_isr);
    irq_set_enabled(DMA_IRQ_1, true);

    // Pre-fill the TX FIFO before clocking, then enable the SM so the first
    // frame goes out with data already queued.
    dma_channel_start(_dma_chan);
    pio_sm_set_enabled(_pio, _sm, true);
}

void DAC::dma_isr() {
    _instance->onDmaComplete();
}

void DAC::onDmaComplete() {
    if (!dma_channel_get_irq1_status(_dma_chan))
        return;
    dma_channel_acknowledge_irq1(_dma_chan);

    const unsigned finished = _playIndex;
    const unsigned next = finished ^ 1u;

    // Re-arm immediately so the bit clock never stops. The next buffer was
    // filled by process() during the previous frame; the TX FIFO bridges the
    // few cycles of interrupt latency until this transfer takes over.
    dma_channel_transfer_from_buffer_now(_dma_chan, _stereo[next].data(), _stereo[next].size());

    _playIndex = next;
    _freeIndex = finished;
    _freeReady = true;
}

void DAC::process() {
    // Pull the latest input audio. With the non-blocking ADC this never stalls
    // the loop; the DAC's own DMA/ISR keeps the bit clock running regardless.
    input.process();
    const BufferType& samples = input.getBuffer();

    // Wait until the ISR releases the buffer that is not currently playing.
    // This blocking wait paces the loop to the DAC's frame rate; the DMA keeps
    // streaming the other buffer meanwhile, so the bit clock stays continuous.
    while (!_freeReady)
        tight_loop_contents();

    const unsigned idx = _freeIndex;
    _freeReady = false;
    StereoBuffer& dst = _stereo[idx];

#if SNDFX_DAC_ADC_MONITOR
    if (g_adcCaptureCompletions == 0) {
        // No captures yet: sound the clock-detector result. code 0 -> steady
        // tone (clocks present, capture still failing); code N -> N short beeps
        // identifying the first dead clock line (see the header comment).
        const unsigned code = g_adcClockDiag;
        bool toneOn = true;
        if (code != 0) {
            constexpr unsigned beepFrames = 8;  // ~21 ms tone burst
            constexpr unsigned gapFrames  = 8;  // ~21 ms gap between beeps
            constexpr unsigned groupGap   = 32; // ~85 ms gap between groups
            const unsigned slot = beepFrames + gapFrames;
            const unsigned cycle = code * slot + groupGap;
            static unsigned frameCtr = 0;
            const unsigned pos = frameCtr % cycle;
            if (++frameCtr >= cycle)
                frameCtr = 0;
            toneOn = false;
            for (unsigned k = 0; k < code; ++k) {
                const unsigned start = k * slot;
                if (pos >= start && pos < start + beepFrames) {
                    toneOn = true;
                    break;
                }
            }
        }

        static unsigned tonePhase = 0;
        for (size_t i = 0; i < bufferSize; ++i) {
            SampleType s = 0;
            if (toneOn) {
                s = (tonePhase < kToneHalfPeriod) ? kToneAmplitude : -kToneAmplitude;
                if (++tonePhase >= 2 * kToneHalfPeriod)
                    tonePhase = 0;
            } else {
                tonePhase = 0;
            }
            dst[2 * i]     = s;
            dst[2 * i + 1] = s;
        }
        return;
    }
#endif

    // The pipeline carries one mono channel, but the PCM5102A is clocked for
    // stereo I2S (two 32-bit words per frame). Duplicate each mono sample into
    // the left and right slots so both outputs carry the same audio and the
    // DAC consumes exactly one input sample per frame.
    for (size_t i = 0; i < samples.size(); ++i) {
        dst[2 * i]     = samples[i];
        dst[2 * i + 1] = samples[i];
    }
}
