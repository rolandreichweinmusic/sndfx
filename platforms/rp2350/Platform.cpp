#include <etl/infinite_loop.h>
#include <etl/print.h>
#include <etl/error_handler.h>

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "Platform.h"

Platform::Platform() {
    // Nothing to do. With no operating system or scheduler, the audio loop runs
    // uninterrupted on the core at full priority, so real-time behaviour is
    // inherent. Any future board-level initialisation belongs here.
}

void Platform::error_handler(const etl::exception& e) {
    etl::println("Error: {}", e.what());
    etl::infinite_loop();
}

// Character sink for etl::print / etl::println on the RP2350.
//
// ETL funnels every formatted-output character through this hook. We route it
// to the board's default UART (uart0, GPIO0 = TX) at 115200 8N1. Those are the
// Seeed XIAO RP2350 defaults (PICO_DEFAULT_UART*) and sit on a pin left free by
// the I2S buses (the ADC owns GPIO2-5, the DAC GPIO26-28). The UART is brought
// up on the first call rather than in the Platform constructor so the very
// first log line is emitted correctly no matter when it occurs -- including
// from the error handler before, or independently of, board construction.
extern "C" void etl_putchar(int c)
{
    static bool initialised = false;
    if (!initialised) {
        gpio_set_function(PICO_DEFAULT_UART_TX_PIN,
                          UART_FUNCSEL_NUM(uart_default, PICO_DEFAULT_UART_TX_PIN));
        uart_init(uart_default, PICO_DEFAULT_UART_BAUD_RATE);
        initialised = true;
    }

    // Expand newlines to CR+LF so lines break correctly on a serial console.
    if (c == '\n') {
        uart_putc_raw(uart_default, '\r');
    }
    uart_putc_raw(uart_default, static_cast<char>(c));
}
