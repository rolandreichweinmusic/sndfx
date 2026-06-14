Linux host specific implementation, for testing purposes.

## Real-time audio setup

For low-latency audio the program (see `Platform.cpp`) tries to:

- lock its memory with `mlockall` so page faults cannot stall the audio loop, and
- run the audio thread under the `SCHED_FIFO` real-time scheduling policy.

Both require privileges. Without them the program still runs, but prints a
warning and falls back to normal priority. Grant the privileges in one of the
following ways.

### Recommended: allow the `audio` group (persistent, unprivileged run)

Add your user to the `audio` group and raise its limits, then log back in:

```sh
sudo usermod -aG audio "$USER"
```

Create `/etc/security/limits.d/99-realtime.conf`:

```
@audio   -   rtprio     95
@audio   -   memlock    unlimited
```

Then run normally:

```sh
./build-linux/platforms/linux/sndfx
```

On many distributions installing the `rtkit` package (or a JACK/PipeWire audio
setup) already provides an equivalent configuration.

### Quick check: run with elevated privileges

```sh
sudo ./build-linux/platforms/linux/sndfx
```

This is fine for a one-off latency test but not recommended as a habit.

### Verify it worked

If real-time scheduling is active, no `Platform:` warnings are printed at
startup. You can also confirm the scheduling policy (`FF` = SCHED_FIFO) while the
program runs:

```sh
ps -eLo comm,cls,rtprio | grep sndfx
```