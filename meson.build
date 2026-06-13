project('sndfx', ['c', 'cpp'],
    version: '0.1.0',
    default_options: [
        'cpp_std=c++23',
        'c_std=c11',
        'warning_level=2',
        'werror=true',
    ],
    meson_version: '>=1.0.0',
)

# ETL dependency
etl_dep = dependency('etl',
    fallback: ['etl', 'etl_dep'],
)

# Common sources
common_inc = include_directories('common')
common_src = files('common/main.cpp',
'common/ADC.cpp',
'common/Compressor.cpp',
'common/DAC.cpp',
'common/Operation.cpp')

# Platform selection based on cross-compilation
if meson.is_cross_build()
    subdir('platforms/rp2350')
else
    subdir('platforms/linux')
endif
