Sndfx
=====

Sound Effects Processor

## Build

### Linux (native, for testing)

The host code uses C++23, which needs GCC 14+ or Clang 18+. Select
one via a meson native file (the distro-default GCC 13 will not compile it):

```sh
meson setup build-linux --native-file cross/linux-gcc14.ini
meson compile -C build-linux
```

To use Clang 18 instead, swap in `cross/linux-clang18.ini`.

### RP2350 (cross-compiled)

```sh
meson setup build-rp2350 --cross-file cross/rp2350.ini
meson compile -C build-rp2350
```

## Run

### Linux

```sh
./build-linux/platforms/linux/sndfx
```

### RP2350 (flash via picotool)

Hold BOOTSEL and connect the board, then:

```sh
picotool load build-rp2350/platforms/rp2350/sndfx
picotool reboot
```

## Clean

```sh
meson setup build-linux --wipe
meson setup build-rp2350 --wipe
```
