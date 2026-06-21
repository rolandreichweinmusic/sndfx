// PCM5102A

#include "DAC.h"
#include "i2s_output.pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

DAC::DAC(Operation& input, PIO pio, uint sm, uint data_pin, uint bclk_pin)
    : input(input), _pio(pio), _sm(sm) {
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

    // Configure DMA
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
        _stereo.data(),           // read from stereo staging buffer
        _stereo.size(),           // transfer count (two words per frame)
        false                     // don't start yet
    );

    pio_sm_set_enabled(_pio, _sm, true);
}

void DAC::process() {
    input.process();
    const BufferType& samples = input.getBuffer();

    // The pipeline carries one mono channel, but the PCM5102A is clocked for
    // stereo I2S (two 32-bit words per frame). Duplicate each mono sample into
    // the left and right slots so both outputs carry the same audio and the
    // DAC consumes exactly one input sample per frame.
    for (size_t i = 0; i < samples.size(); ++i) {
        _stereo[2 * i]     = samples[i];
        _stereo[2 * i + 1] = samples[i];
    }

    // Start DMA transfer and wait for completion
    dma_channel_set_read_addr(_dma_chan, _stereo.data(), true);
    dma_channel_wait_for_finish_blocking(_dma_chan);
}
