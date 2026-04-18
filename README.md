# Ratatoskr GUI

Qt6/QML frontend for [ratatoskrd](https://github.com/olekkvale/ratatoskr) -- open source hardware control for Linux.

## Supported devices

- **Logitech Astro A50 Gen 5** -- full control surface: Volume, Stream Routing, EQ Editor, Settings

### Planned (pending driver support in ratatoskrd)

- **Keychron Q6 HE** (USB wired + 2.4 GHz dongle)

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

## Installation

```bash
# Recommended: match daemon prefix for consistent paths
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

Files installed (with `-DCMAKE_INSTALL_PREFIX=/usr`):
- `/usr/bin/ratatoskr-gui`
- `/usr/share/applications/ratatoskr-gui.desktop`
- `/usr/share/icons/hicolor/scalable/apps/ratatoskr-gui.svg`

Default prefix is `/usr/local` if `-DCMAKE_INSTALL_PREFIX` is omitted.

## Uninstallation

```bash
sudo cmake --build build --target uninstall
```

## Running

The daemon must run on the system D-Bus before starting the GUI.

```bash
# Primary: systemd-managed daemon (installed by ratatoskrd package)
sudo systemctl start ratatoskrd

# Alternative (development): run daemon manually
sudo ratatoskrd

# Start GUI (as regular user)
ratatoskr-gui
```

## License

GPL-3.0. See [LICENSE](LICENSE).
