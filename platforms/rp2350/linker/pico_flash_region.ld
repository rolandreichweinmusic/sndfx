/* Flash memory region for the Seeed XIAO RP2350 (4 MB external QSPI flash).
 *
 * The RP2350 maps execute-in-place (XIP) flash at 0x10000000. This file is
 * INCLUDEd by memmap_default.ld and is the project-local replacement for the
 * file the Pico SDK normally generates from pico_flash_region.template.ld via
 * CMake (where PICO_FLASH_SIZE_BYTES is substituted in). We are not using the
 * SDK's CMake build, so the size is fixed here to match the board.
 */
FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 4M
