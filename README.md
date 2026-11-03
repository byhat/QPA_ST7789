# QPA_ST7789

Two components in one repository:

- **`src/`** — a Qt 6 QPA platform plugin (`qpast7789`) that renders Qt
  applications directly on an ST7789 SPI display (135x240, RGB565);
- **`lib/`** — a small dependency-free display driver library that can be
  used on its own, without Qt, to init the panel, fill it and blit
  rectangles.

Built for a 1.14" ST7789 135x240 LCD on **Orange Pi Zero 3** (H618,
Ubuntu 24.04, Qt 6.4); the structure and behavior mirror the QPA_ST7735
project. The library talks to the hardware through plain Linux
calls (`spidev` ioctls); the DC/CS/RST lines are driven through
**WiringPi/WiringOP**. No OpenGL.

## Repository layout

```
QPA_ST7789/
├── lib/                    display driver library (no Qt dependency)
│   ├── hal/                SpidevBus (spidev), WiringGpioLine (WiringPi/WiringOP)
│   ├── display/            St7789Driver: init, fill, blit
│   └── example/            st7789-example: draws colored squares at 32 MHz
├── scripts/setup-permissions.sh  udev rules + input group (no root needed)
├── src/                    QPA plugin "qpast7789"
│   ├── Plugin/             plugin entry point + metadata.json
│   ├── Platform/           Integration, Screen, Window, Cursor, BackingStore
│   ├── Input/              MouseDevice, KeyboardDevice: evdev input
│   └── Config/             JSON / environment configuration
├── config/qpa_st7789.json  default configuration for Orange Pi Zero 3
└── qml-app/                QML demo (clock, text input, scroll view, button)
```

## Wiring (Orange Pi Zero 3, 26-pin header)

| Display pin | Board pin | Signal             | SoC pin |
|-------------|-----------|--------------------|---------|
| VCC         | 1 or 17   | 3.3V               | -       |
| GND         | 6/9/14/20/25 | GND              | -       |
| SCL (SCK)   | 23        | SPI1 SCLK (PH6)    | - (spidev) |
| SDA (MOSI)  | 19        | SPI1 MOSI (PH7)    | - (spidev) |
| CS          | 26        | chip-select        | PC10    |
| DC (A0)     | 22        | data/command       | PC7     |
| RESET       | 18        | reset              | PC14    |

The "Board pin" column is what goes into the config: GPIO lines are
addressed by physical header pin numbers.

GPIO lines are driven through WiringPi/WiringOP (see the next
section). SPI must be enabled with a spidev overlay so that
`/dev/spidev1.1` exists (Orange Pi: `overlays=spi1-cs1-spidev` in
`/boot/orangepiEnv.txt`).

## GPIO lines: WiringPi / WiringOP

GPIO is driven through WiringPi (Raspberry Pi) or WiringOP (Orange Pi)
with **physical header pin numbers** (`wiringPiSetupPhys()`) — the
number of the pin you wired the display to:

    gpio readall | grep PC10
    |      |  16 |     PC10 |   OUT  | 0 |  26 | ...   <- physical pin 26

Do not use the GPIO-column numbers: the gpio-mode table of at least one
WiringOP build (Zero 3) is broken and silently drives wrong pins,
which shows up as colorful garbage on the panel.

Install the library build for your board (WiringOP repositories are
per-SoC; on Raspberry Pi use a WiringPi fork with Pi 5 support), then
`make install` — the plugin looks for `wiringPi.h` and `-lwiringPi`
in `/usr/local` and `/usr`.

Line numbers live in the JSON config (`gpio.cs/dc/rst`) and can be
overridden with `QT_SPI_GPIO_CS/DC/RST`, so moving the plugin to
another board is a config edit, not a recompile.

Note: WiringPi/WiringOP writes GPIO registers directly — on Orange Pi
boards that goes through `/dev/mem`, so applications there run with
`sudo`. On Raspberry Pi the `gpiomem` device allows running without
root (the permissions script opens it).

## Requirements

For the display library alone (`lib/`) nothing beyond a C++17 compiler
and CMake is needed.

On the target board, for the plugin and the demo:

    sudo apt install build-essential cmake \
        qt6-base-dev qt6-base-private-dev qt6-base-dev-tools

plus WiringPi/WiringOP built and installed for your board
(`wiringPi.h` and `libwiringPi.so` must be present).

For the QML demo additionally:

    sudo apt install qt6-declarative-dev \
        qml6-module-qtquick qml6-module-qtquick-window \
        qml6-module-qtquick-controls qml6-module-qtquick-templates

## Build

Build everything (library + example + plugin + QML demo) on the board:

    cmake -B build -S .
    cmake --build build -j$(nproc)

Outputs:

- `build/lib/example/st7789-example` — standalone library demo
- `build/src/libqpast7789.so` — the QPA plugin

### Building only the display library

The library has no Qt dependency and builds on its own — useful for
plain C++ projects that just want to draw on the panel:

    cmake -B build-lib -S lib
    cmake --build build-lib -j$(nproc)

Output: `build-lib/example/st7789-example` (colored squares demo).

## Install the plugin

For distro-packaged Qt the plugin must go into the system platforms
directory; `--prefix /usr` is required (the default `/usr/local` prefix
is not scanned by Qt):

    sudo cmake --install build --prefix /usr

This installs:

- `/usr/lib/<arch>/qt6/plugins/platforms/libqpast7789.so`
- `/usr/share/qpa_st7789/examples/qpa_st7789.json` — sample config

Alternatively skip installation and point Qt at the build directory:

    export QT_QPA_PLATFORM_PLUGIN_PATH=$PWD/build/src

## Running applications

After installing the plugin run once as root:

    sudo sh scripts/setup-permissions.sh

It installs a udev rule that opens `/dev/spidev*` and `/dev/gpiomem`
and adds your user to the `input` group for the mouse and keyboard.

GPIO itself goes through WiringPi/WiringOP: on boards with `gpiomem`
(Raspberry Pi) applications then run without root; on Orange Pi the
library needs `/dev/mem`, so run the application with `sudo`:

    sudo env QT_QPA_PLATFORM=qpast7789 ./your-app

Widgets and raster windows work out of the box. Qt Quick (QML) needs
the software renderer because the plugin provides no OpenGL context:

    sudo env QT_QPA_PLATFORM=qpast7789 QT_QUICK_BACKEND=software ./your-qml-app

### QML demo

    g++ -std=c++17 -O2 qml-app/main.cpp -o /tmp/qml-demo \
        $(pkg-config --cflags --libs Qt6Quick Qt6Qml Qt6Gui)
    sudo install -D qml-app/Main.qml /usr/local/share/qpast7789/Main.qml
    sudo env QT_QPA_PLATFORM=qpast7789 QT_QUICK_BACKEND=software /tmp/qml-demo

## Using the display library directly

Without any Qt, link against the `st7789` static library (or just add
the six files of `lib/hal` and `lib/display` to your project):

```cpp
#include "SpidevBus.hpp"
#include "SysfsGpioLine.hpp"
#include "St7789Driver.hpp"

SpidevBus bus("/dev/spidev1.1");
WiringGpioLine chipSelect(26);   // PC10, физический пин
WiringGpioLine dataCommand(22);  // PC7
WiringGpioLine reset(18);        // PC14

St7789Driver::Config config{135, 240, 1'000'000, 32'000'000, 40, 52, true};
St7789Driver driver(bus, chipSelect, dataCommand, reset, config);
driver.initialize();
driver.fill(0xF800);  // red

// partial update: pixels are RGB565 in native uint16_t order,
// stride is the row pitch of the source buffer in bytes
std::vector<std::uint16_t> patch(40 * 40, 0x07E0);
driver.blit(10, 10, 40, 40, patch.data(), 40 * sizeof(std::uint16_t));
```

`lib/example/main.cpp` is a complete runnable demo: it clears the panel
and draws colored squares one by one in different screen regions.

## Configuration

The plugin looks for a JSON file in this order (first existing wins):

1. path from `$QPA_ST7789_CONFIG`
2. `./qpa_st7789.json` in the working directory
3. `/etc/qpa_st7789.json` and `/usr/local/etc/qpa_st7789.json`
4. `/usr/share/qpa_st7789/qpa_st7789.json` — installed by
   `cmake --install`, so a board-specific config needs no recompiling
5. built-in defaults (Orange Pi Zero 3, 32 MHz data speed)

```json
{
    "spi":   { "device": "/dev/spidev1.1", "speed": 32000000 },
    "gpio":  { "cs": 26, "dc": 22, "rst": 18 },
    "panel": {
        "width": 135, "height": 240,
        "initSpeedHz": 1000000,
        "offsetX": 40, "offsetY": 52,
        "invert": true
    },
    "fonts": { "dir": "/usr/share/fonts/truetype/dejavu" },
    "mouse": { "enabled": true, "showCursor": true, "device": "auto" },
    "keyboard": { "enabled": true, "device": "auto" }
}

The `panel` keys are what makes an ST7789 different from an ST7735:

| Key       | Default | Effect                                                    |
|-----------|---------|-----------------------------------------------------------|
| `offsetX` | `40`    | RAM window column offset: the 135x240 visible area sits inside the controller's 240x320 RAM; some modules need 52 or 53 |
| `offsetY` | `52`    | RAM window row offset; some modules need 40 or 53         |
| `invert`  | `true`  | INVON during init: 1.14" panels show inverted colors without it |

If the picture is shifted by a few pixels or wraps around, tune
`offsetX`/`offsetY` for your particular module.
```

Environment variables override JSON (handy for quick experiments):

| Variable          | Effect                                    |
|-------------------|-------------------------------------------|
| `QT_SPI_DEV`      | spidev device path                        |
| `QT_SPI_SPEED`    | data transfer speed, Hz                   |
| `QT_SPI_GPIO_CS`  | chip-select GPIO number                   |
| `QT_SPI_GPIO_DC`  | data/command GPIO number                  |
| `QT_SPI_GPIO_RST` | reset GPIO number                         |
| `QT_MOUSE_DEV`    | mouse evdev device, e.g. `/dev/input/event3` |

Notes on speeds: single commands (init sequence) are always sent at
1 MHz — the panel does not accept them at higher rates on this setup.
Pixel data runs at `spi.speed`; 32 MHz is stable on short wires, drop
it if you see artifacts.

## Mouse input

A USB mouse works out of the box: the plugin reads it directly from
evdev (`/dev/input/event*`), delivers left/right/middle clicks and the
wheel to the application like any other QPA plugin, and draws a
software cursor on top of each frame (the panel has no hardware
overlay). The cursor tip is clamped to the screen bounds, exactly like
on a desktop. Reading `/dev/input` requires root — the same `sudo` the
display already needs.

The mouse device is autodetected (first evdev node with "mouse" in its
name); pin it explicitly via `QT_MOUSE_DEV` or the JSON `mouse.device`.
Without a mouse the plugin keeps working, and the cursor stays hidden
until the first mouse event. Only `ArrowCursor` and `BlankCursor`
shapes are honored — there is no cursor theme on the panel, everything
else renders as the arrow.

Mouse options in the JSON config:

```json
"mouse": { "enabled": true, "showCursor": true, "device": "auto" }
```

| Key          | Default | Effect                                              |
|--------------|---------|-----------------------------------------------------|
| `enabled`    | `true`  | `false` disables the mouse completely: no device is opened, no events are delivered |
| `showCursor` | `true`  | `false` hides the software cursor; clicks and the wheel keep working |
| `device`     | `"auto"`| explicit evdev node, e.g. `/dev/input/event3`       |

Cursor movement is decoupled from input processing: mouse events are
delivered to the application immediately, while panel updates with the
new cursor position are coalesced to at most ~30 per second.

## Keyboard input

A USB keyboard works the same way: the plugin reads it from evdev and
delivers key presses/releases (with Shift/Ctrl/Alt/Meta modifiers,
Caps Lock and autorepeat) via `QWindowSystemInterface::handleKeyEvent`,
exactly like other QPA plugins. The US layout is generated internally —
printable keys produce text, navigation and function keys send their
Qt key codes. The device is autodetected (name with "keyboard", or a
node that has KEY_Q/KEY_SPACE and no BTN_LEFT); options in the JSON
config:

```json
"keyboard": { "enabled": true, "device": "auto" }
```

## Troubleshooting

- **White screen**: check wiring (CS/DC/RST), check `ls /dev/spidev*`.
- **Garbage on the panel**: the DC/CS/RST numbers resolve to the
  wrong physical pins for this board — recheck them against the
  **GPIO column** of `gpio readall`; on 1.14" panels also tune the
  `panel.offsetX/offsetY` values.
- **Only the first frame renders**: the DC level must be re-asserted
  before every scan line (the library does this); re-check the DC wire.
  If the SPI controller suspends between rare updates, its state is
  visible in
  `cat /sys/class/spi_master/spi1/device/power/runtime_status`.
- **No Qt logs on the console**: add `QT_FORCE_STDERR_LOGGING=1`.
- **Plugin not found**: `QT_QPA_PLATFORM_PLUGIN_PATH` must point to the
  directory that contains `libqpast7789.so` directly.
- **QML shows "This plugin does not support createPlatformOpenGLContext"**:
  `QT_QUICK_BACKEND=software` is missing.
- **Mouse does not move / no clicks**: run as root (`/dev/input/event*`
  are root-only), check the device picked in the log
  (`qpast7789: mouse: ...`), try `QT_MOUSE_DEV=/dev/input/eventN`.

## Limitations

- Raster rendering only, no OpenGL, no vsync.
- Mouse and keyboard only: no touch input.
- Keyboard text is US-layout only.
- Every window is stretched to the full 135x240 screen.
- Each frame is sent whole (about 11 ms at 32 MHz); partial dirty
  regions are not used because incremental updates leave artifacts.

## Hardware notes baked into the library

These behaviors were verified on the target and are part of `lib/`:

- CS is pulled low once per session and held; the kernel chip-select is
  not used (the CS pin of the display is a regular GPIO line).
- The DC level is re-asserted before every scan line; without this the
  panel drops the stream after the first frame.
- GPIO lines are not unexported on destruction: unexporting from one
  process breaks open lines of another (ENODEV).
