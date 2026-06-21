// PCM1808

#include "ADC.h"
#include "i2s_input.pio.h"
#include "i2s_clkgen.pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"

ADC* ADC::_instance = nullptr;

// Liveness flag (read by DAC.cpp): number of completed capture DMAs. While this
// stays 0 the input path has produced no samples yet; the DAC plays a tone so a
// non-capturing input is audibly distinct from captured silence.
volatile uint32_t g_adcCaptureCompletions = 0;

ADC::ADC(PIO pio, uint sm, uint data_pin, uint bclk_pin, uint lrclk_pin,
         PIO clk_pio, uint clk_sm)
    : _pio(pio), _sm(sm) {
    // The PCM1808 runs in SLAVE mode: the RP2350 is the I2S clock master for the
    // input path and generates SCKI, BCK and LRCK itself. Crucially, all three
    // come from a SINGLE state machine (see i2s_clkgen.pio), so they are exactly
    // phase-coherent at the 256 : 64 : 1 ratio the PCM1808's decimation filter
    // requires. (Generating SCKI and BCK/LRCK from two independent state
    // machines, each with its own fractional clk_sys divider, made the SCKI:LRCK
    // ratio drift and the captured audio was heavily distorted.)
    //
    // The clock generator runs on its own PIO (clk_pio, default pio2): its frame
    // is a full 512 instructions, which together with the 9-instruction input
    // program would not fit in one PIO's 32-instruction memory. Pin state is
    // global, so the input SM on `pio` reads BCK/LRCK back from the GPIOs that
    // clk_pio drives.
    //
    // Derive the divider from the *actual* system clock rather than assuming
    // 150 MHz: if runtime_init_clocks ever brings clk_sys up at a different
    // frequency, a hardcoded constant would skew the clocks (and thus fs)
    // without any obvious symptom.
    const float sys_clk_hz = (float)clock_get_hz(clk_sys);

    // The clkgen program is an exact 512-SM-cycle frame (two 256-cycle halves),
    // so SCKI = 256 * fs, BCK = 64 * fs and LRCK = fs (ratios exact by
    // construction). BCK/LRCK/SCKI are emitted on bclk_pin..bclk_pin+2
    // (GPIO3 = BCK, GPIO4 = LRCK, GPIO5 = SCKI).
    //
    // Use an INTEGER divider. The PCM1808 has no PLL, so it converts directly on
    // SCKI and is very sensitive to SCKI jitter. A PIO *fractional* divider
    // dithers the SM clock between two clk_sys periods every cycle, putting large
    // cycle-to-cycle jitter on SCKI; the delta-sigma modulator turns that into
    // audible distortion. (The PCM5102A DAC is immune because it has a PLL that
    // cleans its clock.) Rounding to an integer divider makes SCKI jitter-free at
    // the cost of a slightly off-nominal fs -- e.g. 150 MHz / (512 * 6) =
    // 48.83 kHz instead of 48 kHz, a +1.7% pitch offset that is irrelevant for a
    // live effects passthrough (there is no pitch reference). The 256:64:1 ratio
    // is unaffected. A true 48 kHz with the lowest jitter needs an external
    // 12.288 MHz oscillator on SCKI (see README).
    //
    // The matching DAC (DAC.cpp) rounds its divider the same way, to exactly
    // 4 * this one, so the ADC and DAC run at the *same* fs (sample-rate locked,
    // sys / 3072 each) -- otherwise the capture and playback rates drift and the
    // single-buffer hand-off periodically slips.
    const uint32_t clk_div_int = (uint32_t)(sys_clk_hz / (512.0f * 48000.0f) + 0.5f);
    uint clk_offset = pio_add_program(clk_pio, &i2s_clkgen_program);
    i2s_clkgen_program_init(clk_pio, clk_sm, clk_offset, bclk_pin, (float)clk_div_int);

    // Load PIO program
    _offset = pio_add_program(_pio, &i2s_input_program);

    // Configure PIO state machine for I2S input.
    // Philips I2S format: data is MSB-first within a 32-bit slot, with the MSB
    // delayed one BCLK after the LRCLK edge. That 1-bit delay is handled inside
    // the PIO program (i2s_input.pio); here we only set the shift direction and
    // width so the captured slot lands MSB-justified in the ISR.
    pio_sm_config c = i2s_input_program_get_default_config(_offset);
    sm_config_set_in_pins(&c, data_pin);
    sm_config_set_in_shift(&c, false, true, 32); // shift left (MSB-first), autopush at 32 bits
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    // Configure pins. Only DOUT (data_pin / GPIO2) is owned by the input SM as
    // an input. BCK (bclk_pin) and LRCK (bclk_pin + 1) are driven by the clkgen
    // SM above; the input SM reads them back through the PIO input synchronisers
    // (wait pin / jmp pin) to stay frame-aligned, so it must NOT claim them as
    // its own (input) pindirs here.
    pio_gpio_init(_pio, data_pin);
    pio_sm_set_consecutive_pindirs(_pio, _sm, data_pin, 1, false);   // data in

    // Wait pin is LRCLK for left/right sync
    sm_config_set_jmp_pin(&c, lrclk_pin);

    // Clock divider: PIO clock = sys_clk / div. The input SM samples the (now
    // RP2350-generated) BCLK via wait instructions, so it runs at full speed.
    sm_config_set_clkdiv(&c, 1.0f);

    pio_sm_init(_pio, _sm, _offset, &c);

    _instance = this;

    // Start the capture buffers and the published buffer silent so a not-yet-
    // live (or never-live) ADC yields clean silence rather than indeterminate
    // memory.
    _capture[0].fill(0);
    _capture[1].fill(0);
    _buffer.fill(0);

    // Configure DMA for continuous double-buffered capture. The channel fills
    // one buffer, then the completion ISR re-arms it into the other, so the RX
    // FIFO is drained without gaps and no samples are dropped.
    _dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dma_cfg = dma_channel_get_default_config(_dma_chan);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_cfg, false);
    channel_config_set_write_increment(&dma_cfg, true);
    channel_config_set_dreq(&dma_cfg, pio_get_dreq(_pio, _sm, false));

    dma_channel_configure(
        _dma_chan,
        &dma_cfg,
        _capture[0].data(),       // write to the first capture buffer
        &_pio->rxf[_sm],          // read from PIO RX FIFO
        _capture[0].size(),       // transfer count
        false                     // started below, after the IRQ is wired
    );

    // Buffer 0 fills first; nothing is ready until the first completion.
    _captureIndex = 0;
    _readyIndex = 0;
    _readyValid = false;

    // Route this channel's completion to DMA_IRQ_0 and install the handler.
    dma_channel_set_irq0_enabled(_dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, &ADC::dma_isr);
    irq_set_enabled(DMA_IRQ_0, true);

    // Arm capture, then enable the SM so samples flow into the RX FIFO.
    dma_channel_start(_dma_chan);
    pio_sm_set_enabled(_pio, _sm, true);
}

void ADC::dma_isr() {
    _instance->onDmaComplete();
}

void ADC::onDmaComplete() {
    if (!dma_channel_get_irq0_status(_dma_chan))
        return;
    dma_channel_acknowledge_irq0(_dma_chan);

    const unsigned filled = _captureIndex;
    const unsigned next = filled ^ 1u;

    // Re-arm immediately into the other buffer so the RX FIFO keeps draining.
    // The FIFO bridges the few cycles of interrupt latency until this transfer
    // takes over.
    dma_channel_transfer_to_buffer_now(_dma_chan, _capture[next].data(), _capture[next].size());

    _captureIndex = next;
    _readyIndex = filled;
    _readyValid = true;
    g_adcCaptureCompletions = g_adcCaptureCompletions + 1;
}

void ADC::process() {
    // Non-blocking: publish the most recently captured buffer if the DMA has
    // completed since the last call; otherwise leave the previous contents in
    // place. A blocking wait here would hang the entire pipeline if the PCM1808
    // is not clocking the input PIO, because the capture DMA would never
    // complete -- which is exactly the failure being diagnosed.
    if (_readyValid) {
        const unsigned idx = _readyIndex;
        _readyValid = false;
        _buffer = _capture[idx];
    }
}
