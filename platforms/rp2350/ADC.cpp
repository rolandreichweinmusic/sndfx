// PCM1808

#include "ADC.h"
#include "i2s_input.pio.h"
#include "mclk.pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

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
    // A 12 MHz crystal cannot synthesise 12.288 MHz exactly through the PLL, so
    // this divider is fractional: SCKI carries a small (~0.03%) frequency offset
    // and some jitter. The PCM1808 (via SCKI) and PCM5102A (via clk_sys) both
    // derive their timing from clk_sys, so the ADC and DAC sample rates stay
    // locked to each other regardless of the absolute value.
    constexpr float sys_clk_hz = 150000000.0f; // RP2350 default (runtime_init_clocks)
    constexpr float scki_hz = 256.0f * 48000.0f; // 12.288 MHz
    uint sck_offset = pio_add_program(_pio, &mclk_program);
    mclk_program_init(_pio, sck_sm, sck_offset, sck_pin, sys_clk_hz / (2.0f * scki_hz));

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

    // Configure DMA
    _dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dma_cfg = dma_channel_get_default_config(_dma_chan);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_cfg, false);
    channel_config_set_write_increment(&dma_cfg, true);
    channel_config_set_dreq(&dma_cfg, pio_get_dreq(_pio, _sm, false));

    dma_channel_configure(
        _dma_chan,
        &dma_cfg,
        _buffer.data(),           // write to buffer
        &_pio->rxf[_sm],          // read from PIO RX FIFO
        _buffer.size(),           // transfer count
        false                     // don't start yet
    );

    pio_sm_set_enabled(_pio, _sm, true);
}

void ADC::process() {
    // Start DMA transfer and wait for completion
    dma_channel_set_write_addr(_dma_chan, _buffer.data(), true);
    dma_channel_wait_for_finish_blocking(_dma_chan);
}
