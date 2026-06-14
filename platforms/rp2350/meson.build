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

rp2350_inc = include_directories('.')

# Pico SDK sources needed by our code
pico_sdk_src = files(
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_gpio' / 'gpio.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_pio' / 'pio.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_dma' / 'dma.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_irq' / 'irq.c',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_platform_panic' / 'panic.c',
    pico_sdk_path / 'src' / 'common' / 'hardware_claim' / 'claim.c',
)

sndfx_rp2350 = executable('sndfx',
    common_src,
    pico_sdk_src,
    'ADC.cpp',
    'DAC.cpp',
    'Platform.cpp',
    i2s_input_pio_h,
    i2s_output_pio_h,
    include_directories: [common_inc, rp2350_inc, pico_inc],
    dependencies: [etl_dep],
    cpp_args: ['-include', meson.project_source_root() / 'common' / 'etl_profile.h'],
    link_args: ['-Wl,--unresolved-symbols=ignore-in-object-files'],
)
