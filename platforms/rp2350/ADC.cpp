// PCM1808

#include "ADC.h"
#include "i2s_input.pio.h"
#include "mclk.pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

ADC* ADC::_instance = nullptr;

// Diagnostic (read by DAC.cpp): number of completed capture DMAs. Stays 0 if the
// PCM1808 is not clocking the input PIO (no BCK/LRCK), so capture never completes
// and this never advances. Lets the DAC distinguish "input not clocking" from
// "input clocking but data is zero".
volatile uint32_t g_adcCaptureCompletions = 0;

// Diagnostic (read by DAC.cpp): first dead link in the input clock chain.
//   0 = SCKI, BCK and LRCK all toggle (clocks present)
//   1 = SCKI (GPIO5) not toggling   -> RP2350 master-clock output dead/unwired
//   2 = SCKI ok, BCK (GPIO3) static -> PCM1808 not producing clocks (MD0/MD1 straps, SCKI wire)
//   3 = BCK ok, LRCK (GPIO4) static -> LRCK wiring
volatile uint32_t g_adcClockDiag = 0;

namespace {
// Sample a pin many times; return true if it ever changes state. The window
// (~10 ms at the -O0 build setting) spans hundreds of LRCK periods and many
// thousands of BCK/SCKI periods, so any live clock is detected reliably.
bool pinToggles(uint pin) {
    const bool first = gpio_get(pin);
    for (uint32_t i = 0; i < 300000; ++i) {
        if (gpio_get(pin) != first)
            return true;
    }
    return false;
}
} // namespace

ADC::ADC(PIO pio, uint sm, uint data_pin, uint bclk_pin, uint lrclk_pin,
         uint sck_pin, uint sck_sm)
    : _pio(pio), _sm(sm) {
    // The PCM1808 runs in master mode, deriving BCK and LRCK from its system
    // clock input (SCKI). Generate SCKI on a second state machine first so the
    // codec is already producing the I2S clocks by the time the input state
    // machine starts.
    //
    // SCKI = 256 * fs = 12.288 MHz for fs = 48 kHz. The mclk program toggles the
    // pin once per instruction (two instructions per period), so the state
    // machine runs at 2 * SCKI.
    //
    // Derive the divider from the *actual* system clock rather than assuming
    // 150 MHz: if runtime_init_clocks ever brings clk_sys up at a different
    // frequency, a hardcoded constant would skew SCKI (and thus the codec's fs)
    // without any obvious symptom.
    const float sys_clk_hz = (float)clock_get_hz(clk_sys);
    constexpr float scki_hz = 256.0f * 48000.0f; // 12.288 MHz
    uint sck_offset = pio_add_program(_pio, &mclk_program);
    mclk_program_init(_pio, sck_sm, sck_offset, sck_pin, sys_clk_hz / (2.0f * scki_hz));

    // ---- PCM1808 startup-reset workaround ----
    // The codec's 3.3 V supply comes up at board power-on, but SCKI only starts
    // here, hundreds of ms later, once the RP2350 has booted. A PCM1808 whose
    // internal power-on reset completed without a valid SCKI can latch into a
    // state where it never generates BCK/LRCK -- exactly the "SCKI toggles, BCK
    // static" symptom. Master mode is a pure SCKI divider (no PLL), so the only
    // lever we have is SCKI itself: now that the supply is long stable, hold
    // SCKI running briefly, then stop and restart it to present a clean clock
    // (re)start that re-triggers the codec's clock generation.
    busy_wait_us_32(100000);                       // 100 ms: supply + SCKI settle
    pio_sm_set_enabled(_pio, sck_sm, false);       // drop SCKI
    busy_wait_us_32(5000);                          // 5 ms with SCKI stopped
    pio_sm_set_enabled(_pio, sck_sm, true);        // clean SCKI restart (the "kick")
    busy_wait_us_32(5000);                          // let the codec re-acquire

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

    // Configure pins
    pio_gpio_init(_pio, data_pin);
    pio_gpio_init(_pio, bclk_pin);
    pio_gpio_init(_pio, lrclk_pin);
    pio_sm_set_consecutive_pindirs(_pio, _sm, data_pin, 1, false);   // data in
    pio_sm_set_consecutive_pindirs(_pio, _sm, bclk_pin, 2, false);   // bclk, lrclk in

    // Wait pin is LRCLK for left/right sync
    sm_config_set_jmp_pin(&c, lrclk_pin);

    // Clock divider: PIO clock = sys_clk / div
    // PCM1808 provides BCLK externally, PIO samples on it
    // Set div to 1 (PIO runs at sys clock, syncs to external BCLK via wait instructions)
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

    // ---- Diagnostic: detect which input clock (if any) is missing. ----
    // SCKI has been generated since mclk_program_init() above; give the PCM1808
    // a few ms to start dividing it into BCK/LRCK, then check each line. The
    // first one found static identifies the broken link in the clock chain.
    for (uint32_t s = 0; s < 200000; ++s) {
        asm volatile("" ::: "memory"); // settle; barrier keeps the loop at -O0+
    }
    if (!pinToggles(sck_pin))
        g_adcClockDiag = 1; // RP2350 SCKI output dead/unwired
    else if (!pinToggles(bclk_pin))
        g_adcClockDiag = 2; // PCM1808 not producing BCK (master straps / SCKI wire)
    else if (!pinToggles(lrclk_pin))
        g_adcClockDiag = 3; // LRCK missing
    else
        g_adcClockDiag = 0; // all input clocks present
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
