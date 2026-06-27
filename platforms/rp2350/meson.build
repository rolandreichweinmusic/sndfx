pico_sdk_path = run_command('sh', '-c', 'echo ${PICO_SDK_PATH:-}', check: true).stdout().strip()
if pico_sdk_path != ''
    message('Using PICO_SDK_PATH: ' + pico_sdk_path)
else
    error('PICO_SDK_PATH environment variable is not defined')
endif

pioasm_src = files(
    pico_sdk_path / 'tools' / 'pioasm' / 'main.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'pio_assembler.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'pio_disassembler.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'gen' / 'lexer.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'gen' / 'parser.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'c_sdk_output.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'python_output.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'hex_output.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'json_output.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'ada_output.cpp',
    pico_sdk_path / 'tools' / 'pioasm' / 'go_output.cpp',
)

pioasm = executable('pioasm',
    pioasm_src,
    include_directories: include_directories(
        pico_sdk_path / 'tools' / 'pioasm',
        pico_sdk_path / 'tools' / 'pioasm' / 'gen',
        'pioasm_cfg',
    ),
    override_options: ['werror=false'],
    native: true,
)

pico_inc = include_directories(
    'pico_config',
    pico_sdk_path / 'src' / 'common' / 'pico_base_headers' / 'include',
    pico_sdk_path / 'src' / 'common' / 'hardware_claim' / 'include',
    pico_sdk_path / 'src' / 'common' / 'pico_sync' / 'include',
    pico_sdk_path / 'src' / 'common' / 'pico_time' / 'include',
    pico_sdk_path / 'src' / 'common' / 'pico_util' / 'include',
    pico_sdk_path / 'src' / 'common' / 'pico_binary_info' / 'include',
    pico_sdk_path / 'src' / 'common' / 'pico_stdlib_headers' / 'include',
    pico_sdk_path / 'src' / 'rp2350' / 'pico_platform' / 'include',
    pico_sdk_path / 'src' / 'rp2350' / 'hardware_structs' / 'include',
    pico_sdk_path / 'src' / 'rp2350' / 'hardware_regs' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_platform_compiler' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_platform_common' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_platform_sections' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_platform_panic' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_runtime' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_runtime_init' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_base' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_irq' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_sync' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_sync_spin_lock' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_resets' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_clocks' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_pll' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_ticks' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_xosc' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_gpio' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_pio' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_dma' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_timer' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_uart' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_spi' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_i2c' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_pwm' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_adc' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_vreg' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_boot_lock' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_runtime' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_runtime_init' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_clib_interface' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_atomic' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_bootrom' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'boot_bootrom_headers' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_rcp' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_flash' / 'include',
    pico_sdk_path / 'src' / 'common' / 'boot_picobin_headers' / 'include',
    pico_sdk_path / 'src' / 'common' / 'boot_picoboot_headers' / 'include',
    pico_sdk_path / 'src' / 'boards' / 'include',
    is_system: true,
)

# Generate PIO headers
i2s_input_pio_h = custom_target('i2s_input_pio_h',
    input: 'i2s_input.pio',
    output: 'i2s_input.pio.h',
    command: [pioasm, '-o', 'c-sdk', '@INPUT@', '@OUTPUT@'],
)

i2s_output_pio_h = custom_target('i2s_output_pio_h',
    input: 'i2s_output.pio',
    output: 'i2s_output.pio.h',
    command: [pioasm, '-o', 'c-sdk', '@INPUT@', '@OUTPUT@'],
)

# PCM1808 I2S clock (BCK/LRCK) generator: the RP2350 is the I2S clock master for
# the input path with the PCM1808 in slave mode.
i2s_clkgen_pio_h = custom_target('i2s_clkgen_pio_h',
    input: 'i2s_clkgen.pio',
    output: 'i2s_clkgen.pio.h',
    command: [pioasm, '-o', 'c-sdk', '@INPUT@', '@OUTPUT@'],
)

rp2350_inc = include_directories('.')

# Pico SDK sources needed by our code
pico_sdk_src = files(
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_gpio' / 'gpio.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_pio' / 'pio.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_dma' / 'dma.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_irq' / 'irq.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_uart' / 'uart.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_platform_panic' / 'panic.c',
    pico_sdk_path / 'src' / 'common' / 'hardware_claim' / 'claim.c',
)

# Startup and runtime sources from the Pico SDK.
#
# crt0.S supplies the cold-boot entry path the RP2350 bootrom requires: the
# vector table (.vectors), the reset handler (.reset), and the embedded
# IMAGE_DEF block (.embedded_block) that marks the binary as a bootable Arm
# image. Without it the bootrom refuses to launch the firmware.
#
# The runtime sources bring the chip up from the bootrom's minimal state to a
# usable system clock. runtime_init() (newlib variant, since we link against
# nosys.specs) runs the registered initialisers, including runtime_init_clocks,
# which configures XOSC + PLLs and brings clk_sys to 150 MHz -- the frequency
# the I2S clock dividers in DAC.cpp/ADC.cpp assume.
pico_startup_src = files(
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_crt0' / 'crt0.S',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_irq' / 'irq_handler_chain.S',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_runtime' / 'runtime.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_runtime_init' / 'runtime_init.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_runtime_init' / 'runtime_init_clocks.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_runtime_init' / 'runtime_init_stack_guard.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_clib_interface' / 'newlib_interface.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_bootrom' / 'bootrom.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_clocks' / 'clocks.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_pll' / 'pll.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_xosc' / 'xosc.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_ticks' / 'ticks.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_timer' / 'timer.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_vreg' / 'vreg.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_sync' / 'sync.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_sync_spin_lock' / 'sync_spin_lock.c',
)

# The Pico SDK is third-party code that does not build cleanly under this
# project's strict warning settings (warning_level=2, werror=true), e.g. it has
# unused-parameter warnings in the newlib syscall stubs. Compile it into a
# separate static library with relaxed warnings so our own sources can stay
# strict. The PIO headers are generated and consumed by our ADC/DAC sources, so
# they remain with the executable below.
pico_support = static_library('pico_support',
    pico_sdk_src,
    pico_startup_src,
    include_directories: [rp2350_inc, pico_inc],
    pic: false,
    override_options: ['warning_level=1', 'werror=false', 'c_std=gnu11'],
)

# Linker script that places the binary in XIP flash at 0x10000000 with the
# correct RP2350 memory map. memmap_default.ld INCLUDEs pico_flash_region.ld,
# both of which live in the linker/ directory below (added to the linker search
# path with -L so the INCLUDE resolves).
linker_dir = meson.current_source_dir() / 'linker'
linker_script = linker_dir / 'memmap_default.ld'

sndfx_rp2350 = executable('sndfx',
    common_src,
    'ADC.cpp',
    'DAC.cpp',
    'Platform.cpp',
    i2s_input_pio_h,
    i2s_output_pio_h,
    i2s_clkgen_pio_h,
    include_directories: [common_inc, rp2350_inc, pico_inc],
    dependencies: [etl_dep],
    link_whole: pico_support,
    cpp_args: ['-include', meson.project_source_root() / 'common' / 'etl_profile.h'],
    link_args: [
        '-T' + linker_script,
        '-L' + linker_dir,
        '-Wl,--gc-sections',
        # RAM-resident code (.time_critical, copied .text) shares the SRAM
        # .data load segment, which is necessarily writable and executable; the
        # RP2350 memory map relies on this, so silence ld's RWX-segment warning.
        '-Wl,--no-warn-rwx-segments',
    ],
    link_depends: [linker_script, linker_dir / 'pico_flash_region.ld'],
)
