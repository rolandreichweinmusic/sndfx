linux_inc = include_directories('.')

sndfx_linux = executable('sndfx',
    common_src,
    'ADC.cpp',
    'DAC.cpp',
    include_directories: [common_inc, linux_inc],
    dependencies: [etl_dep],
)
