# Commands and patterns

## How a frame is drawn

`loop()` runs continuously. Every pass it services OTA (`ArduinoOTA.handle()`) and checks for a UDP
packet (`udpHandler()`). Every **100 ms** it draws one frame:

1. Look at the last command received (`rxBuf`, initially `"rainbow"`) and draw that pattern.
2. Push both data pins (`strip.show()`, `strip2.show()`).

The on-board LED blinks once a second as a heartbeat.

## UDP commands

Send a UDP packet containing just the command to port **5005**. The board replies `OK: <text>` to anything
it receives, whether or not it's a valid command. The command stays in effect until another one arrives –
including an invalid one, which replaces it (see [known issues](#known-issues)).

| Command | What you see |
|---|---|
| `rainbow` | Colour wheel along the strips, scrolling one step per frame (full cycle ~25.6 s). **Power-up default.** |
| `redGrnChase` | Repeating 30-pixel block – 15 pixels fading up and down in red, then 15 in green – moving 1 pixel per frame (10 pixels/s) from the far end of strip 2 towards the far end of strip 1b |
| `redGrnChase2` | 48-pixel red and green humps that fade up to 96/255 and back, alternating along all 210 pixels and scrolling 1 pixel per frame (10 pixels/s) from the far end of strip 1b towards the far end of strip 2 |
| `redgrn` | Static alternating red and green pixels, each at 100/255 |
| `purple` | Solid purple (255, 0, 255) |
| `warm` | Solid warm white (255, 55, 15) |
| `dim` | Solid white at (1, 1, 1) – just visible |
| `off` | All pixels off |
| `cchk` | Solid deep pink (90, 5, 10) |
| `cchkRRRGGGBBB` | Solid colour from three 3-digit decimal values, e.g. `cchk255100000` = orange, `cchk000000255` = blue |
| *anything else* | Nothing is redrawn – the strips hold their current frame |

Matching is exact and case-sensitive (`cchk` only checks the first 4 characters). A trailing newline –
e.g. from `echo redgrn | nc -u …` – makes the command fail to match, so the strips freeze; use `echo -n`,
the Python example in the README, or the Windows tool
[`udp_send`](https://github.com/davidorser/leds_winC_UDP_CLIprog), which never adds a newline:

```bash
udp_send 192.168.1.50 5005 cchk255100000
```

## Pixel mapping

All drawing goes through `setPixel_decodedStripLoc(loc, …)`, which turns a logical position 0–209 into a
physical strip and index. There are two overloads: one takes separate R, G, B values and one takes a packed
`uint32_t` colour and unpacks it.

| Logical `loc` | Strip object | Physical index |
|---|---|---|
| 0–69 | `strip2` (GPIO 16) | `69 − loc` (strip 2, reversed) |
| 70–139 | `strip` (GPIO 15) | `loc − 70` (strip 1a) |
| 140–209 | `strip` (GPIO 15) | `loc − 70` (strip 1b, chained) |

Out-of-range positions are silently ignored. `DECODE_NUM_PIXELS` is **209 – the last index, not the
count** – so loops that cover every pixel must use `<=`.

## Patterns in detail

### `rainbow` – `rainbowNF()`

Sets pixel `i` to `Wheel((i + j) & 255)`, where `j` increases by 1 each frame. `Wheel()` is the standard
Adafruit colour wheel: red → green → blue → red over 0–255, with R+G+B always 255. 210 pixels cover 210
of the 256 wheel steps, so almost the whole spectrum is visible at once.

### `redGrnChase` – `redGrnChaseNF()`

Draws a 30-pixel block every 30 pixels along the strip, offset by `j`, which increases by 1 each frame.
Within a block the brightness profile is 100 divided by 16, 12, 8, 6, 4, 3, 2, **1**, 2, 3, 4, 6, 8, 12, 16
– a soft peak – first in red, then in green. Exactly seven blocks fill the 210 pixels, so the pattern
wraps from pixel 209 back to 0 without a seam.

### `redGrnChase2` – `redGrnChaseSinNF()`

A triangle-wave chase over the whole strip. For each pixel, `phase = pos + time`:

- `4 * cheapSin(phase, 24)` gives the brightness – it rises from 0 to 96 and back to 0 every 48 pixels.
- `(phase / 48) % 2` picks the colour – even humps are red, odd humps are green.

Each hump begins and ends on a dark pixel, so red and green never blend. `time` increases by 1 each frame
and wraps every 96 frames (one red + one green hump), so the motion is seamless. To change the hump size,
edit `halfWidth`.

### `cchk` – custom colour

Characters 5–13 of the message are read as three 3-digit numbers (red, green, blue). With nothing after
`cchk` it uses (90, 5, 10).

## Extra pixel (`strip3`)

A one-pixel strip on GPIO 18 is set to (64, 64, 64) once in `setup()` and never touched again. The code
also writes pixels 1 and 2, which don't exist on a 1-pixel strip and are ignored by the library.

## Known issues

1. **Unknown commands freeze the animation.** `udpHandler()` copies every message into `rxBuf`, valid or
   not. When nothing matches, no pattern is drawn, so the strips hold their last frame – a typo stops
   `rainbow` or `redGrnChase` mid-motion. The board still replies `OK:`, so the sender can't tell.
2. **`cchk` input isn't validated.** It assumes exactly nine digits follow. Shorter messages read the
   string terminator and beyond, giving an unpredictable colour; numbers above 255 wrap (e.g. 300 → 44).
3. **Full brightness.** `setBrightness(255)` is used, so `cchk255255255` asks for ~135 W – 90 % of the
   150 W supply – and the most voltage drop on strip 1b.
4. **Leftover code.** `rainbow(wait)` (blocking, strip 1 only) and `loopFinishTime` are unused; comments
   still say WS2812B, though `NEO_KHZ800` is the right setting for these WS2811 strips.

### Fixed

- **`off` command added**, and `warm` is now its own command instead of the catch-all for unknown text.
- **`redGrnChase2` no longer flashes** – `cheapSin()` returns `2*max - x` on the falling half (it used to
  go negative and wrap to bright values such as 252).
- **`redGrnChase2` spans all three strips** – it used to draw only pixels 0–95; it now runs alternating
  red and green humps along all 210 pixels.
- **Pixel 209 is animated** – `rainbowNF()` loops with `<=` and `redGrnChaseNF()` uses
  `DECODE_NUM_PIXELS + 1`, which also removed a one-pixel seam in the chase.
- **Serial Monitor spam removed** – the per-loop `Serial.println(strcmp(rxBuf, "hi"))` is commented out.
- Compared with the older v003 sketch: there's a power-up pattern (`rainbow`), timing uses
  `unsigned long` so the animation no longer stops after ~24.8 days, and the pulse-colour bug is gone.

## Ideas for later

- Validate commands in `udpHandler()`: keep the current pattern on unknown text and reply `ERR:`.
- Validate `cchk` input and clamp values to 0–255.
- Remember the last command in flash so it survives a power cut.
- Add a time-of-day schedule using NTP (`configTime()` in the arduino-pico core).
- Cap brightness with `setBrightness()` to reduce power and the voltage-drop tint on strip 1b.
