# Ratatoskr GUI

Qt6/QML frontend for [ratatoskrd](https://github.com/olekkvale/ratatoskr) -- open source hardware control for Linux.

## Features

- **Volume** -- headset volume, MixAmp, sidetone, mic input (PipeWire/PulseAudio)
- **Stream Routing** -- 5-channel stream mixer (stream, mic out, game, bluetooth, voice) with per-channel mute
- **EQ Editor** -- 10-band parametric EQ with gain (-12 to +12 dB) and Q factor (0.031-8.0)
  - Built-in presets: Standard, Gaming, Media (headphone) + Standard, Broadcast, Competition (mic)
  - Community presets: Music+Media, Tarkov Footsteps, Gaming+Media + Stream, Clear
  - Custom presets: save, load, rename, delete, reorder -- persisted in JSON
  - Noise Gate (Home/Night/Tournament) in mic mode
- **Settings** -- device info, Bluetooth status, notifications, suspend timer, LED brightness

## Requirements

- [ratatoskrd](https://github.com/olekkvale/ratatoskr) running on system D-Bus
- Qt6: `qt6-base`, `qt6-declarative`, `qt6-svg`
- `libpulse` (PipeWire/PulseAudio compatibility)
- `cmake` (3.16+), C++17 compiler

## Building

```bash
# Arch Linux
sudo pacman -S qt6-base qt6-declarative qt6-svg libpulse cmake

mkdir build && cd build
cmake ..
make
```

## Running

```bash
# Start daemon first (requires root)
sudo ratatoskrd

# Then start GUI (as regular user)
./build/ratatoskr-gui
```

## License

GPL-3.0. See [LICENSE](LICENSE).
