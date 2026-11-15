# Commands and patterns

## How a frame is drawn

`loop()` runs continuously. Every pass it services OTA (`ArduinoOTA.handle()`) and checks for a UDP
packet (`udpHandler()`). Every **100 ms** it draws one frame:

1. Look at the last command received (`rxBuf`, initially `"rainbow"`) and draw that pattern.
2. Push both data pins (`strip.show()`, `strip2.show()`).

The on-board LED blinks once a second as a heartbeat.

## UDP commands

Send a UDP packet containing just the command to port **5005**. The board replies `OK: <text>` to anything
it receives, whether or not it's a valid command. The command stays in effect until another one arrives.

| Command | What you see |
|---|---|
| `rainbow` | Colour wheel along the strips, scrolling one step per frame (full cycle ~25.6 s). **Power-up default.** |
| `redGrnChase` | Repeating 30-pixel block – 15 pixels fading up and down in red, then 15 in green – moving 1 pixel per frame (10 pixels/s) from the far end of strip 2 towards the far end of strip 1b |
| `redGrnChase2` | Experimental sine-style red/green chase – unfinished, see [known issues](#known-issues) |
| `redgrn` | Static alternating red and green pixels, each at 100/255 |
| `purple` | Solid purple (255, 0, 255) |
| `dim` | Solid white at (1, 1, 1) – just visible |
| `cchk` | Solid deep pink (90, 5, 10) |
| `cchkRRRGGGBBB` | Solid colour from three 3-digit decimal values, e.g. `cchk255100000` = orange, `cchk000000255` = blue |
| *anything else* (including `off`) | Solid warm white (255, 55, 15) |

Matching is exact and case-sensitive (`cchk` only checks the first 4 characters). A trailing newline –
e.g. from `echo redgrn | nc -u …` – makes the command fail to match and you get warm white; use `echo -n`
or the Python example in the README.

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
– a soft peak – first in red, then in green. Seven blocks fit in 210 pixels.

### `redGrnChase2` – `redGrnChaseSinNF()`

Intended as a triangle-wave chase: red on pixels 0–47, a 6-pixel gap, green on 54–95, using `cheapSin()`
as a cheap triangle wave with peak 24 × 4 = 96. Not finished – see below.

### `cchk` – custom colour

Characters 5–13 of the message are read as three 3-digit numbers (red, green, blue). With nothing after
`cchk` it uses (90, 5, 10).

## Extra pixel (`strip3`)

A one-pixel strip on GPIO 18 is set to (64, 64, 64) once in `setup()` and never touched again. The code
also writes pixels 1 and 2, which don't exist on a 1-pixel strip and are ignored by the library.

## Known issues

Found while documenting the sketch. None have been fixed yet.

1. **No way to turn the lights off.** Any unrecognised text – including `off` – falls into the final
   `else` and paints full warm white (~57 W). `dim` is the lowest setting.
2. **`redGrnChase2` is broken.**
   - `cheapSin()` returns `max − x` on the falling half instead of `2·max − x`, so the value goes
     negative. Negative values wrap to large ones when stored in the 8-bit colour, so you get bright
     flashes (e.g. −4 becomes 252) instead of a fade.
   - Only pixels 0–47 and 54–95 are drawn. Pixels 48–53 and 96–209 keep whatever the previous command left.
3. **Pixel 209 is skipped by the animations.** `rainbowNF()` loops `i < DECODE_NUM_PIXELS` and
   `redGrnChaseNF()` uses `len = DECODE_NUM_PIXELS` with `% len`, so both treat the strip as 209 pixels.
   The last pixel (far end of strip 1b) keeps the previous command's colour, and the chase has a one-pixel
   seam where its last block wraps onto the first.
4. **`cchk` input isn't validated.** It assumes exactly nine digits follow. Shorter messages read the
   string terminator and beyond, giving an unpredictable colour; numbers above 255 wrap (e.g. 300 → 44).
5. **Serial Monitor spam.** `Serial.println(strcmp(rxBuf, "hi"))` runs on every pass through `loop()`,
   thousands of times a second, burying the useful startup and UDP messages.
6. **Full brightness.** `setBrightness(255)` is used, so `cchk255255255` asks for ~135 W – 90 % of the
   150 W supply – and the most voltage drop on strip 1b.
7. **Leftover code.** `rainbow(wait)` (blocking, strip 1 only) and `loopFinishTime` are unused; comments
   still say WS2812B, though `NEO_KHZ800` is the right setting for these WS2811 strips.

Fixed compared with the older v003 sketch: there's now a power-up pattern (`rainbow`), timing uses
`unsigned long` so the animation no longer freezes after ~24.8 days, and the pulse-colour bug is gone
(v004 has no pulse).

## Ideas for later

- Add an `off` command and make unknown text do nothing instead of turning on warm white.
- Fix `cheapSin()` (`return 2*max - x;`) and draw the full strip in `redGrnChase2`.
- Use `DECODE_NUM_PIXELS + 1` (or rename it to the last index) in `rainbowNF()` and `redGrnChaseNF()`.
- Validate `cchk` input and clamp values to 0–255.
- Remember the last command in flash so it survives a power cut.
- Add a time-of-day schedule using NTP (`configTime()` in the arduino-pico core).
- Cap brightness with `setBrightness()` to reduce power and the voltage-drop tint on strip 1b.
