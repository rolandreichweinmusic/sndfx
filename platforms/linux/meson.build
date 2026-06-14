linux_inc = include_directories('.')
alsa_dep = dependency('alsa')
threads_dep = dependency('threads')
sndfx_linux = executable('sndfx',
    common_src,
    'AudioDevice.cpp',
    'ADC.cpp',
    'DAC.cpp',
    'Platform.cpp',
    include_directories: [common_inc, linux_inc],
    dependencies: [etl_dep, alsa_dep, threads_dep],
)
