linux_inc = include_directories('.')
alsa_dep = dependency('alsa')
sndfx_linux = executable('sndfx',
    common_src,
    'AudioDevice.cpp',
    'ADC.cpp',
    'DAC.cpp',
    include_directories: [common_inc, linux_inc],
    dependencies: [etl_dep, alsa_dep],
)
