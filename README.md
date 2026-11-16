# 3×70 Neon LED Strips – Raspberry Pi Pico W

Holiday light controller for **three 12 V addressable "neon" LED flex strips** (WS2811, 70 pixels each,
210 pixels total) driven by a **Raspberry Pi Pico W**. The strips run as one long line with the controller
in the middle. You pick the pattern by sending a short **UDP text message** over Wi‑Fi – rainbow, red/green
chase, solid colours, or any RGB colour you type in.

The Pico W can be **re-flashed over the air (OTA)** from the Arduino IDE once the first USB upload is done.

> History: written in Nov 2025 as `20251113_RPICO_LightsXmas_RedGrn_WifiUDP_v004.ino`, a successor to the
> Oct 2025 `20251018_RPICO_AniFrame_v003.ino`; packaged as this repository in Nov 2026.

---

## Features

- **Rainbow at power-up** – a colour wheel scrolls along all 210 pixels until another command arrives.
- **Red/green chase** – soft red and green bands, 15 pixels each, travel along the strips.
- **Solid colours** – red/green alternating, purple, warm white, a very dim night-light level, and off.
- **Any colour** – `cchkRRRGGGBBB` (e.g. `cchk255100000`) sets every pixel to that RGB value.
- **UDP control** – send the command word to port 5005; the board replies `OK: <message>`. See
  [docs/COMMANDS.md](docs/COMMANDS.md).
- **One logical strip** – the three physical strips (on two data pins, with different colour orders) are
  addressed as a single 0–209 pixel run.
- **OTA updates** – upload new firmware over Wi‑Fi using the Arduino IDE network port.
- **Heartbeat** – the Pico W's on-board LED blinks once per second while the sketch is running.

## Hardware

| Item | Notes |
|---|---|
| [Raspberry Pi Pico W](https://www.raspberrypi.com/products/raspberry-pi-pico/) | Board `rp2040:rp2040:rpipicow` |
| 3 × [ALITOVE addressable neon LED strip](https://www.amazon.com/dp/B0CDQ2LDQH) | 12 V, WS2811, 5 m, 420 LEDs / 70 pixels per strip (each pixel = 6 LEDs) – variant *Neon-5M-420LED-12V* |
| [GINRANS 12 V LED power supply](https://www.amazon.com/dp/B0B6W2RMNF) | 150 W, 12.5 A, IP67, mains plug |
| [PlusRoc 12 V → 5 V buck converter](https://www.amazon.com/dp/B09DGFR24W) | 5 V 3 A, micro-USB lead – powers the Pico W through its USB port (style *Micro USB Connector*) |

### Wiring summary

| Pico W pin | Connects to |
|---|---|
| GPIO 15 (physical pin 20) | DIN of **strip 1a** (strip 1b is chained after it) |
| GPIO 16 (physical pin 21) | DIN of **strip 2** |
| GPIO 18 (physical pin 24) | A single extra WS2811 pixel, set dim white at boot (purpose to confirm) |
| GND (e.g. physical pin 18 or 23) | GND of both strip runs and of the 12 V supply |
| Micro-USB port | 5 V from the buck converter |

The 12 V supply feeds the strips directly and the buck converter in parallel. **Never connect 12 V to any
Pico W pin.** Diagram, power budget and open questions are in [docs/HARDWARE.md](docs/HARDWARE.md).

## Software requirements

- [Arduino IDE 2.x](https://www.arduino.cc/en/software)
- **Raspberry Pi Pico/RP2040/RP2350** board core by Earle Philhower
  ([arduino-pico](https://github.com/earlephilhower/arduino-pico)) – tested with 6.3.1
- **Adafruit NeoPixel** library – tested with 1.15.5

Install steps for both are under [Getting started](#getting-started). `WiFi`, `WiFiUdp`, `SimpleMDNS` and
`ArduinoOTA` are bundled with the arduino-pico core.

## Getting started

1. **Clone** the repository. Keep the folder name `leds_3x70neonLED_rpico` – the Arduino IDE requires the
   sketch folder to match the `.ino` file name.
   ```bash
   git clone https://github.com/davidorser/leds_3x70neonLED_rpico.git
   ```
2. **Add the Philhower board package URL**: in the Arduino IDE open **File → Preferences** (on macOS,
   **Arduino IDE → Settings**). In **Additional boards manager URLs** add the URL below, then click **OK**.
   If the field already has URLs, put this one on its own line (click the icon beside the field to open
   the list editor).
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. **Install the Pico board core**: open **Tools → Board → Boards Manager…**, search for `pico`, find
   **Raspberry Pi Pico/RP2040/RP2350** *by Earle F. Philhower, III*, and click **Install**.
   > Don't confuse it with **Arduino Mbed OS RP2040 Boards** by Arduino, which has no Pico W Wi‑Fi support
   > and won't compile this sketch.
4. **Install the Adafruit NeoPixel library**: open **Tools → Manage Libraries…**, search for
   `Adafruit NeoPixel`, and click **Install**.
5. **Create your credentials file**: copy `secrets.h.example` to `secrets.h` in the same folder and fill
   in your Wi‑Fi SSID/password, an OTA hostname and an OTA password. `secrets.h` is git-ignored.
6. **Open** `leds_3x70neonLED_rpico.ino` in the Arduino IDE.
7. Select **Tools → Board → Raspberry Pi Pico/RP2040/RP2350 → Raspberry Pi Pico W**.
8. **First upload over USB**: unplug the buck converter from the Pico W and connect it to your computer
   instead (the Pico W has only one USB port). Hold BOOTSEL while plugging in if no COM port appears, then
   click *Upload*.
9. Open the **Serial Monitor** to see the assigned IP address and `UDP listening on port 5005`.
10. Plug the buck converter back in; from now on use OTA uploads.

Prefer the command line? The same setup, build and upload steps using `arduino-cli` are in
[docs/ARDUINO_CLI.md](docs/ARDUINO_CLI.md).

### Over-the-air (OTA) updates

After the first USB flash, the board advertises itself on the network as `OTA_HOSTNAME` (default
`djoLightsTriStrip`) on port 2040. In the Arduino IDE choose it under **Tools → Port → Network ports**,
then *Upload*; enter `OTA_PASSWORD` when prompted. Your PC and the Pico W must be on the same subnet (mDNS
is used for discovery).

> If the Pico W can't connect to Wi‑Fi at boot it prints `Connection Failed! Rebooting...` and restarts
> after 5 s, so it keeps retrying until the network is available. **The strips stay dark until Wi‑Fi
> connects.**

### Changing the pattern

Send a command as a UDP packet to port 5005. Python example:

```python
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(2)
s.sendto(b"redGrnChase", ("192.168.1.50", 5005))   # use the IP printed on the Serial Monitor
print(s.recvfrom(1024)[0].decode())                 # -> OK: redGrnChase
```

| Command | Result |
|---|---|
| `rainbow` | Scrolling rainbow (the power-up default) |
| `redGrnChase` | Travelling red and green bands |
| `redgrn` | Alternating red/green pixels, static |
| `purple` | Solid purple |
| `warm` | Solid warm white |
| `dim` | Barely-on white night light |
| `off` | All pixels off |
| `cchkRRRGGGBBB` | Solid colour from three 3-digit values, e.g. `cchk000000255` for blue |
| *anything else* | Nothing is redrawn – the strips freeze on their current frame |

Commands are case-sensitive (no trailing newline). The board replies `OK:` even to commands it doesn't
recognise. Details are in [docs/COMMANDS.md](docs/COMMANDS.md).

## Configuration

| Setting | Where | Default | Meaning |
|---|---|---|---|
| `PIN2` | `.ino` | `15` | Data pin for strips 1a + 1b (`strip`) |
| `PIN` | `.ino` | `16` | Data pin for strip 2 (`strip2`) |
| `NUMPIXELS` | `.ino` | `70` | Pixels per physical strip |
| `DECODE_NUM_PIXELS` | `.ino` | `209` | Index of the last logical pixel (210 pixels, 0–209) |
| Colour order | `.ino` | `NEO_BRG` (strips 1a/1b), `NEO_GBR` (strip 2) | Change if colours appear swapped |
| `rxBuf` initial value | `.ino` | `"rainbow"` | Pattern shown at power-up |
| Frame period | `loop()` | `100` ms | Time between animation frames |
| `LOCAL_PORT` | `.ino` | `5005` | UDP listen port |
| `WIFI_SSID`, `WIFI_PASSWORD`, `OTA_HOSTNAME`, `OTA_PASSWORD` | `secrets.h` | – | Network credentials |

## Repository layout

```
leds_3x70neonLED_rpico/
├── leds_3x70neonLED_rpico.ino   # the sketch
├── secrets.h.example            # template for Wi-Fi / OTA credentials
├── secrets.h                    # your copy (git-ignored, not in the repo)
├── docs/
│   ├── HARDWARE.md              # parts, wiring, power budget
│   ├── COMMANDS.md              # UDP commands, pixel mapping, patterns
│   └── ARDUINO_CLI.md           # building and uploading with arduino-cli
└── README.md
```

## Known limitations

These are documented, not yet fixed. Details are in [docs/COMMANDS.md](docs/COMMANDS.md#known-issues).

- **Unknown commands freeze the animation** – a typo stops `rainbow` or `redGrnChase` on its current
  frame, and the board still replies `OK:`.
- **`redGrnChase2` is unfinished** – it only draws pixels 0–95; the rest keep the previous pattern.
- **`cchk` isn't checked** – fewer than 9 digits gives a random colour; values over 255 wrap around.
- **Full brightness** – `setBrightness(255)`; `purple` is ~90 W and `cchk255255255` ~135 W, 90 % of the
  supply (see [docs/HARDWARE.md](docs/HARDWARE.md#power-budget)).

## Author

David Orser
