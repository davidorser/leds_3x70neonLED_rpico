# Hardware

## Components

| Part | Key specs | Role |
|---|---|---|
| **Raspberry Pi Pico W** | RP2040 + CYW43439 Wi‑Fi, 3.3 V logic | Controller |
| **3 × ALITOVE addressable neon LED strip** (variant *Neon-5M-420LED-12V*, white FPCB, IP65) | 12 V DC, WS2811, 5 m, 420 LEDs per strip driven by **70 WS2811 ICs** (6 LEDs per IC), ~45 W per strip at full white per the listing | Lights – 210 addressable pixels in total |
| **GINRANS LED power supply** | 100–240 V AC in, **12 V DC, 12.5 A (150 W)** out, constant voltage, IP67 | Powers the strips and the buck converter |
| **PlusRoc DC‑DC buck converter** | 8–35 V in, **5 V, 3 A (15 W)** out, IP68, micro-USB output lead, 20 AWG input leads | Powers the Pico W from the 12 V supply |

Because each WS2811 drives a 6-LED segment, one "pixel" is a ~7 cm section of the neon strip. Patterns
are coarse compared with a 5 V pixel strand – plan animations in terms of 70 pixels per strip.

## Physical layout

The strips form **one long line with the controller in the middle** (described in the sketch as "one long
strip with box in middle"):

```
 logical: 0 ─────────── 69 │        │ 70 ─────────── 139   140 ─────────── 209
          [   Strip 2    ] ◄─[Pico W]─► [   Strip 1a    ] ─► [   Strip 1b    ]
 physical: 69 ◄──────── 0     GPIO16    0 ──────────── 69    70 ─────────── 139
                              GPIO15        (strip 1b is chained after 1a)
```

| Physical strip | Data pin | Index on that pin | Colour order | Logical pixels |
|---|---|---|---|---|
| Strip 2 | GPIO 16 (`PIN`, `strip2`) | 0–69 | GBR | 0–69 (**reversed**: logical 0 = far end, 69 = next to the Pico) |
| Strip 1a | GPIO 15 (`PIN2`, `strip`) | 0–69 | BRG | 70–139 |
| Strip 1b (chained after 1a) | GPIO 15 | 70–139 | BRG | 140–209 |

Reversing strip 2 makes the logical pixels run continuously from one far end to the other, so animations
travel straight through the controller without a jump.

The older v003 sketch arranged the same strips as a closed loop ("loop around table"); its mapping is
still in a comment in `setPixel_decodedStripLoc()`.

### Extra pixel on GPIO 18

The sketch also creates `strip3`: **one WS2811 pixel on GPIO 18** (physical pin 24), BRG order, 400 kHz
mode. It's set to dim white (64, 64, 64) once at boot and never changed. Its purpose isn't documented –
possibly a status or test pixel. *(To confirm.)*

## Wiring

```
 AC mains ─► GINRANS 12 V / 12.5 A
                 │ +12 V                                   │ GND
                 ├──────────────► Strip 1a +12 V           ├──► Strip 1a GND
                 ├──────────────► Strip 2  +12 V           ├──► Strip 2  GND
                 │                                         │
                 └─► PlusRoc buck IN+          IN– ◄───────┤
                       5 V micro-USB ─► Pico W USB port    │
                                                           │
   ┌───────────────────────┐                               │
   │     Raspberry Pi      │                               │
   │        Pico W         │                               │
   │  GPIO15 (pin 20) ─────┼──────────────► Strip 1a DIN   │
   │  GPIO16 (pin 21) ─────┼──────────────► Strip 2  DIN   │
   │  GPIO18 (pin 24) ─────┼──────────────► extra pixel DIN (to confirm)
   │  GND (pin 18/23) ─────┼───────────────────────────────┘
   └───────────────────────┘
```

Notes:

- **12 V must never reach a Pico W pin.** VSYS accepts at most ~5.5 V. The buck converter's micro-USB
  lead is the only power input to the Pico W.
- **Common ground is required** between the Pico, both strip runs and the supply. Run a ground wire
  alongside each data wire even though the buck converter's input and output grounds are likely shared.
- **Data level:** the Pico drives data at 3.3 V. WS2811 inputs often accept this over a short lead, but
  it's marginal. If you see flicker or wrong colours, add a 3.3 V → 5 V buffer such as a 74AHCT125
  powered from the Pico's **VBUS** pin (pin 40, 5 V from the buck converter).
- The sketch header recommends a **300–500 Ω resistor** in series with each DIN and a **1000 µF
  capacitor** across the strip power leads, and connecting GND first if you ever wire a live circuit.
- **Fusing:** the supply can deliver 12.5 A, far more than the strips' thin feed wires should carry.
  An inline fuse of about 5 A on each strip run's +12 V lead is cheap protection.
- **Programming:** the Pico W has only one USB port. For a USB upload or Serial Monitor, unplug the buck
  converter and plug in your computer; after the first flash use OTA instead.

## Power budget

Estimates assume each strip draws ~45 W at full white (from the product listing) and that power scales
with the sum of the R, G and B values. `setBrightness(255)` is used, so these are the real levels.

| Command | RGB per pixel | Fraction of full white | Est. total, 3 strips | % of 150 W supply |
|---|---|---:|---:|---:|
| `cchk255255255` (worst case) | 255, 255, 255 | 100 % | ~135 W | 90 % |
| `purple` | 255, 0, 255 | 67 % | ~90 W | 60 % |
| warm white (default / unknown text) | 255, 55, 15 | 42 % | ~57 W | 38 % |
| `rainbow` | always sums to 255 | 33 % | ~45 W | 30 % |
| `redgrn` | 100 on one channel | 13 % | ~18 W | 12 % |
| `redGrnChase` | ≤ 100 on one channel, tapered | < 13 % | < 18 W | < 12 % |
| `dim` | 1, 1, 1 | < 1 % | ~0.5 W | < 1 % |

The Pico W and buck converter add only about 1–2 W. The listing warns that white turns yellowish at the
far end of a 5 m strip from voltage drop and suggests feeding power at both ends; strip 1b, at the end of
a 10 m chain, is the most likely place to see it.

## To confirm on the real build

- [ ] Is a level shifter, series resistor or bulk capacitor fitted?
- [ ] Is 12 V fed only at the controller, or also injected at the far ends (especially strip 1b)?
- [ ] Any fuses?
- [ ] What is the single pixel on GPIO 18, and is it still connected?
- [ ] Why strip 2 is GBR while strips 1a/1b are BRG (different production batches?).
