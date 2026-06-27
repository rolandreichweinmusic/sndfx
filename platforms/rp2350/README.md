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

The RP2350 generates all three input clocks -- SCKI, BCK and LRCK -- from a
**single PIO state machine** on its own PIO block (`pio2`; see
[i2s_clkgen.pio](i2s_clkgen.pio) and [ADC.cpp](ADC.cpp)), so no external
oscillator is needed. Using one state machine is essential: the PCM1808 has no
PLL, and in slave mode its decimation filter requires SCKI to be an exact,
phase-coherent 256x multiple of LRCK. One state machine emits a fixed 512-cycle
frame in which SCKI toggles every cycle, BCK every 4, and LRCK every 256, so the
256 : 64 : 1 ratio is exact by construction. (An earlier design generated SCKI
and BCK/LRCK from two independent state machines with separate fractional
dividers; their ratio drifted and the captured audio was heavily distorted. The
clock program is a full 512 instructions, which is why it runs on its own PIO
rather than sharing `pio0` with the 9-instruction input sampler.)

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

> Note: the audio pipeline targets **fs ≈ 48 kHz** with 32-bit I2S frames
> (32 bits left + 32 bits right), so BCK = 64 fs and SCKI = 256 fs. A single
> `pio2` state machine generates SCKI, BCK and LRCK together on
> GPIO5/GPIO3/GPIO4, emitting an exact **512-cycle** frame: SCKI toggles every
> cycle (256 fs), BCK every 4 cycles (64 fs) and LRCK every 256 cycles (fs), so
> the 256 : 64 : 1 ratio is exact.
>
> The PCM1808 has no PLL and converts directly on SCKI, so SCKI jitter becomes
> audible distortion. The state machine is therefore clocked with an **integer**
> divider: a PIO *fractional* divider dithers the clock cycle-to-cycle (large
> SCKI jitter), whereas an integer divider is jitter-free. The trade-off is that
> fs is not exactly 48 kHz -- at clk_sys = 150 MHz the integer divider is 6, so
> fs = 150 MHz / (512 × 6) = **48.83 kHz** (a +1.7% pitch offset, irrelevant for
> a live effects passthrough). The DAC ([DAC.cpp](DAC.cpp)) rounds its divider
> the same way to exactly 4× this one, so the ADC and DAC run at the **same** fs
> (clk_sys / 3072) and stay sample-rate locked; otherwise the capture and
> playback rates drift and the buffer hand-off periodically clicks. For an exact
> 48 kHz at the lowest jitter, feed SCKI from a dedicated 12.288 MHz (or
> 24.576 MHz) oscillator instead -- a 12 MHz crystal cannot synthesise an exact
> 12.288 MHz through the PLL, so every on-chip divider is either fractional
> (jitter) or off-frequency (this integer-divider trade-off).

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

## Debug UART

Formatted output from `etl::print` / `etl::println` (for example the ETL error
handler in [Platform.cpp](Platform.cpp)) is routed to a serial console over
**UART0**. ETL funnels every character through the `etl_putchar` hook, which
[Platform.cpp](Platform.cpp) implements by writing to the UART; the peripheral
is brought up lazily on the first character so the first log line is emitted
correctly no matter when it occurs.

### Signal connections

These are the Seeed XIAO RP2350 board defaults (`PICO_DEFAULT_UART*`) and are the
two pins left free by the I2S buses — the ADC owns GPIO2–5 and the DAC GPIO26–28.

| RP2350 GPIO | Signal | Direction (RP2350) | USB-serial adapter |
| ----------- | ------ | ------------------ | ------------------ |
| GPIO0       | TX     | Output             | RX                 |
| GPIO1       | RX     | Input              | TX                 |
| GND         | Ground | —                  | GND                |

The line format is **115200 baud, 8 data bits, no parity, 1 stop bit (8N1)**
(`PICO_DEFAULT_UART_BAUD_RATE`; `uart_init` configures 8N1). Connect a 3.3 V
USB-to-serial adapter and open it at 115200 8N1, e.g. `screen /dev/ttyUSB0
115200` or `minicom -D /dev/ttyUSB0 -b 115200`.

> Note: the firmware only **transmits** — logging is output-only, so in practice
> only GPIO0 (TX) and GND need to be wired. GPIO1 is the board's default UART RX
> pin and is listed for completeness; it is reserved for the UART but not read by
> the current firmware. The RP2350 GPIOs are **3.3 V** logic and are not 5 V
> tolerant, so use a 3.3 V adapter. Newlines (`\n`) are expanded to CR+LF in
> [Platform.cpp](Platform.cpp) so lines break correctly on a serial terminal.

