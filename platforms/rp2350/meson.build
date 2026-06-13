pico_sdk_path = get_option('pico_sdk_path')

pico_inc = include_directories(
    pico_sdk_path / 'src' / 'common' / 'pico_base_headers' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_platform_compiler' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_platform_sections' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'pico_platform_panic' / 'include',
    pico_sdk_path / 'src' / 'rp2350' / 'pico_platform' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_base' / 'include',
    pico_sdk_path / 'src' / 'common' / 'pico_stdlib_headers' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_gpio' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_uart' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_spi' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_i2c' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_pwm' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_adc' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_timer' / 'include',
    pico_sdk_path / 'src' / 'rp2_common' / 'hardware_clocks' / 'include',
    is_system: true,
)

sndfx_rp2350 = executable('sndfx',
    common_src,
    include_directories: [common_inc, pico_inc],
    dependencies: [etl_dep],
    cpp_args: ['-include', meson.project_source_root() / 'common' / 'etl_profile.h'],
)
