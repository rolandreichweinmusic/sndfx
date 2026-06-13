Sndfx
=====

Sound Effects Processor

## Build

### Linux (native, for testing)

```sh
meson setup build-linux
meson compile -C build-linux
```

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
