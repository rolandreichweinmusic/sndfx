Target device specific implementation

## PCM1808 ADC wiring

The PCM1808 is connected to the RP2350 over I2S and is operated in **slave
mode**: the RP2350 generates the bit clock (BCK) and word clock (LRCK) and the
PCM1808 clocks its data out synchronously to them. A dedicated PIO state machine
drives BCK/LRCK (see [i2s_clkgen.pio](i2s_clkgen.pio)) and a second state machine
samples the data (see [i2s_input.pio](i2s_input.pio) and [ADC.cpp](ADC.cpp)).

> Why slave mode: the PCM1808 was originally run in master mode, generating
> BCK/LRCK from SCKI itself. The PCM1808 has no PLL, so its master clock output
> inherited the jitter of the (fractional-divider) SCKI and sat at the margin of
> what the RP2350 input PIO could reliably sample -- capture was intermittent.
> Driving BCK/LRCK from the RP2350 makes the input framing synchronous to
> `clk_sys` and deterministic. SCKI is still supplied to the codec; its jitter
> now only affects ADC SNR, not whether frames are captured.

### I2S signal connections

The pins below are the defaults declared in
[ADC.h](ADC.h#L10). BCK and LRCK are consecutive GPIOs because the clock
generator drives them from a 2-pin side-set group (`bclk_pin` and `bclk_pin + 1`).

| PCM1808 pin | Signal              | RP2350 GPIO | Direction (RP2350) |
| ----------- | ------------------- | ----------- | ------------------ |
| DOUT        | Serial data (SD)    | GPIO2       | Input              |
| BCK         | Bit clock           | GPIO3       | Output             |
| LRCK        | Left/right word clk | GPIO4       | Output             |

The PCM1808 is a **24-bit** converter strapped for **Philips I2S** format
(FMT = GND). It receives BCK = 64 fs (32 bit-clocks per channel slot) and places
the 24-bit two's-complement sample in the most-significant bits of each 32-bit
slot, with the low 8 bits driven to zero. Philips I2S delays the MSB by one BCK
after each LRCK edge (the "1-bit delay"); the input PIO
([i2s_input.pio](i2s_input.pio)) discards that leading bit so the sample lands
MSB-justified in the `int32_t` pipeline (sign preserved at bit 31). The effective
resolution is 24-bit.

### Clock and mode pins

The PCM1808 still needs a system clock (SCKI). The RP2350 generates SCKI with a
dedicated PIO state machine (see [mclk.pio](mclk.pio) and [ADC.cpp](ADC.cpp)), so
no external oscillator is needed. In slave mode the codec accepts BCK/LRCK as
inputs and auto-detects the SCKI ratio:

| PCM1808 pin | Purpose                          | Connection                              |
| ----------- | -------------------------------- | --------------------------------------- |
| SCKI        | System clock input (256 fs)      | GPIO5 (12.288 MHz from RP2350 PIO)      |
| MD0 / MD1   | Master/slave + SCKI ratio select | Strap for **slave** mode: both **GND**  |
| FMT         | Audio data format select         | Strap for I2S format: GND               |

> Verify MD0/MD1 against your PCM1808 datasheet's mode table: master mode at
> 256 fs is both pins high, and slave mode (SCKI ratio auto-detected) is both
> pins low. This firmware requires **slave mode**, so tie **MD0 and MD1 to GND**.

### Power and analog

| PCM1808 pin   | Connection                                    |
| ------------- | --------------------------------------------- |
| VDD / VCC     | 3.3 V                                         |
| AGND / DGND   | Ground                                        |
| VINL / VINR   | Analog audio inputs (left / right)            |
| VREF / VCOM   | Decoupling capacitors per PCM1808 datasheet   |

> Note: the audio pipeline runs at **fs = 48 kHz** with 32-bit I2S frames
> (32 bits left + 32 bits right), so BCK = 64 fs = 3.072 MHz and SCKI = 256 fs =
> 12.288 MHz. The RP2350 generates both: a `pio0` state machine drives BCK/LRCK
> on GPIO3/GPIO4 (same divider as the DAC output path), and another toggles
> GPIO5 for SCKI. Because a 12 MHz crystal cannot reach 12.288 MHz exactly
> through the PLL, the SCKI divider is fractional, giving a small (~0.03%)
> frequency offset and some jitter; for best ADC SNR a dedicated low-jitter
> 12.288 MHz oscillator on SCKI is preferable. BCK/LRCK (ADC and DAC) and SCKI
> all derive from `clk_sys`, so every clock in the system stays frequency-locked.

## PCM5102A DAC wiring

The PCM5102A is connected to the RP2350 over I2S and is operated in **slave
mode**: the RP2350 PIO state machine generates the bit clock (BCK) and word
clock (LRCK) and shifts data out synchronously (see
[i2s_output.pio](i2s_output.pio) and [DAC.cpp](DAC.cpp)). The PCM5102A is
strapped for **Philips I2S** format (FMT = GND), so the PIO drives the MSB one
BCK after each LRCK edge (the I2S "1-bit delay") to match the ADC input path.

### I2S signal connections

The pins below are the defaults declared in
[DAC.h](DAC.h#L10). BCK and LRCK must be consecutive GPIOs because the PIO
program drives them from a 2-pin side-set group (`bclk_pin` and `bclk_pin + 1`).
The data pin (DIN) is independent and uses its own GPIO.

| PCM5102A pin | Signal              | RP2350 GPIO | Direction (RP2350) |
| ------------ | ------------------- | ----------- | ------------------ |
| DIN          | Serial data         | GPIO26      | Output             |
| BCK          | Bit clock           | GPIO27      | Output             |
| LRCK         | Left/right word clk | GPIO28      | Output             |

### Mode and system clock pins

The PCM5102A generates its own internal clocks from BCK using its on-board PLL,
so no external system clock (SCKI) is required:

| PCM5102A pin | Purpose                       | Connection                          |
| ------------ | ----------------------------- | ----------------------------------- |
| SCK          | System clock input            | GND (enables internal PLL)          |
| FMT          | Audio data format select      | GND (I2S format)                    |
| XSMT         | Soft mute (active low)        | 3.3 V (un-muted)                    |
| FLT          | Low latency filter            | 3.3V (low latency)                  |
| DEMP         | De-emphasis select            | GND (off)                           |

### Power and analog

| PCM5102A pin  | Connection                                    |
| ------------- | --------------------------------------------- |
| VDD / 3V3     | 3.3 V                                         |
| GND           | Ground                                        |
| OUTL / OUTR   | Analog audio outputs (left / right)           |

> Note: the audio pipeline runs at **fs = 48 kHz** with 32-bit I2S frames
> (32 bits left + 32 bits right), giving BCK = 48000 × 64 = 3.072 MHz.
>
> The DSP pipeline is **mono** — the PCM1808 ADC captures only the left I2S
> slot. Because the PCM5102A is clocked for stereo I2S (two 32-bit words per
> frame), [DAC.cpp](DAC.cpp) duplicates each mono sample into the left and
> right slots, so both analog outputs carry the same signal and exactly one
> input sample is consumed per output frame (ADC and DAC sample rates stay
> matched).
>
> Both converters use **continuous double-buffered DMA**. The PCM5102A ties SCK
> to GND and derives its internal clocks from BCK with its on-board PLL, so it
> needs an **uninterrupted** bit clock: if BCK stalls, the PLL loses lock and
> the codec mutes. Each DMA channel ([DAC.cpp](DAC.cpp), [ADC.cpp](ADC.cpp))
> therefore ping-pongs between two buffers, re-armed from its completion
> interrupt (DMA_IRQ_1 for the DAC, DMA_IRQ_0 for the ADC) with no CPU gap, so
> the TX FIFO never empties and BCK/LRCK free-run. `process()` blocks on a flag
> set by the ISR rather than on the DMA itself, which paces the loop to the
> frame rate while the converters keep streaming. A one-shot, blocking DMA
> stalls BCK between buffers and is heard as silence.
