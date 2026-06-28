#include <etl/infinite_loop.h>
#include <etl/print.h>
#include <etl/error_handler.h>
#include <etl/chrono.h>

#include "hardware/gpio.h"
#include "hardware/timer.h"
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

extern "C"
{

// Character sink for etl::print / etl::println on the RP2350.
//
// ETL funnels every formatted-output character through this hook. We route it
// to the board's default UART (uart0, GPIO0 = TX) at 115200 8N1. Those are the
// Seeed XIAO RP2350 defaults (PICO_DEFAULT_UART*) and sit on a pin left free by
// the I2S buses (the ADC owns GPIO2-5, the DAC GPIO26-28). The UART is brought
// up on the first call rather than in the Platform constructor so the very
// first log line is emitted correctly no matter when it occurs -- including
// from the error handler before, or independently of, board construction.
void etl_putchar(int c)
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

// ETL routes its three chrono clocks through these hooks, each of which must
// return the current count in that clock's configured duration -- nanoseconds
// on this target (the default). The RP2350's only timebase is the hardware
// timer: a free-running 64-bit microsecond counter that starts at zero on
// power-up and, for all practical purposes, never wraps. It is monotonic, so it
// backs the steady and high-resolution clocks directly; with no RTC configured
// for wall-clock time, the system clock necessarily shares the same since-boot
// origin. The microsecond count is scaled to nanoseconds (x1000); the
// underlying hardware resolution remains 1 microsecond.

etl::chrono::high_resolution_clock::rep etl_get_high_resolution_clock()
{
    return static_cast<etl::chrono::high_resolution_clock::rep>(time_us_64()) * 1000;
}

etl::chrono::system_clock::rep etl_get_system_clock()
{
    return static_cast<etl::chrono::system_clock::rep>(time_us_64()) * 1000;
}

etl::chrono::steady_clock::rep etl_get_steady_clock()
{
    return static_cast<etl::chrono::steady_clock::rep>(time_us_64()) * 1000;
}

}
