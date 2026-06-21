// PCM5102A

#include "DAC.h"
#include "i2s_output.pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"

// ---------------------------------------------------------------------------
// DIAGNOSTIC: set to 1 to run the ADC-liveness monitor; set to 0 for normal
// passthrough. (Temporary confirmation aid for the slave-mode input bring-up.)
//
//   * You hear LIVE INPUT audio -> the ADC is capturing: the slave-mode input
//     path works. Set this to 0 for plain passthrough.
//   * You hear a STEADY tone -> no capture has completed: the input path is
//     still not delivering samples (check the PCM1808 slave straps MD0/MD1 = GND
//     and the BCK/LRCK/DOUT/SCKI wiring).
// ---------------------------------------------------------------------------
#define SNDFX_DAC_ADC_MONITOR 1

// Defined in ADC.cpp: capture DMA completion count (0 => no samples captured yet).
extern volatile uint32_t g_adcCaptureCompletions;

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

    // Clock divider. The output SM runs 2 cycles per bit, 64 bits per frame
    // (32L + 32R) = 128 SM cycles per frame, so fs = clk_sys / (128 * div).
    //
    // Use an INTEGER divider, rounded the same way as the ADC's input clock
    // generator (ADC.cpp), so the two converters run at exactly the same fs and
    // stay sample-rate locked: the ADC's clkgen frame is 512 cycles with integer
    // divider D, this frame is 128 cycles with integer divider 4*D, and
    // 512*D == 128*(4*D) -> identical fs (clk_sys / 3072 at 150 MHz =
    // 48.83 kHz). If the rates differed, the ADC->DAC single-buffer hand-off
    // would periodically slip and click. An integer divider also removes the
    // PIO fractional-divider jitter from BCK (the PCM5102A's PLL would hide it
    // anyway, but it keeps both buses clean and exactly locked).
    const float sys_clk_hz = (float)clock_get_hz(clk_sys);
    const uint32_t div_int = (uint32_t)(sys_clk_hz / (48000.0f * 64.0f * 2.0f) + 0.5f);
    sm_config_set_clkdiv(&c, (float)div_int);

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
        // No captures yet -> emit a steady tone so a non-delivering input path
        // is audibly distinct from captured silence. Flips to passthrough below
        // the instant the first capture DMA completes.
        static unsigned tonePhase = 0;
        for (size_t i = 0; i < bufferSize; ++i) {
            const SampleType s = (tonePhase < kToneHalfPeriod) ? kToneAmplitude : -kToneAmplitude;
            if (++tonePhase >= 2 * kToneHalfPeriod)
                tonePhase = 0;
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
