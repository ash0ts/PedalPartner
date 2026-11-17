# Art pipeline — sprites, theme, fonts and motion

VEX's character, colours and fonts are data in `art/`. An artist edits PNG and JSON files in any
pixel editor; `tools/make_sprites.py` validates them and writes the C++ headers the firmware, the
native simulator and the browser workbench all compile. Nobody has to touch `ui.h` to change how
VEX looks or moves.

![PNG strips in art/ become generated headers, the renderer draws on a 100 ms cadence, and the Teensy sends only the rows that changed](figures/art-pipeline.svg)

The figure's editable source is `docs/figures/art-pipeline/` (spec + three state SVGs). Byte
counts in it were measured on the host simulator; SPI timing on the bench has not been measured.

## Files

| Path | What it is | Contract |
|---|---|---|
| `art/theme.json` | Named colours (`#RRGGBB`) and the seven UI roles (`bg, ink, gold, red, dim, plum, fade`) | Max 16 colours, all distinct in RGB. Role names map to `UI_*` in `theme.h`; colour names to `COLOR_*`. |
| `art/vex/manifest.json` | Ordered animations: `{"name", "frames", "frame_ms", "loop"}` | Order is the firmware enum (`ANIM_IDLE`, …). `frames` 1–16; `frame_ms` a multiple of `DISPLAY_MS` (100). `loop:false` plays once from its trigger. |
| `art/vex/<name>.png` | One strip per animation: `32 × frames` wide, 32 high, RGBA | Every opaque pixel must be exactly a theme colour; alpha is 0 (transparent) or 255. Up to 15 sprite colours in total. |
| `art/fonts/tiny.png` | 95 glyphs, 5 × 7 at a 6 px pitch, characters 32–126 | White = ink. Used for captions. Rasterised from the bundled DejaVu Sans Mono. |
| `art/fonts/display.png` | 95 glyphs in 8 × 12 cells, 8 px pitch | White = ink. The importer trims each glyph and emits a width table, so the font is proportional. Used for titles, note names and banners. Rasterised from the bundled DejaVu Sans Bold at 9 px. |

Generated into `firmware/VEX_Pocket_Rival/` and committed: `theme.h`, `sprites.h` (palette +
one byte per pixel, 1 KB per frame), `anim.h` (table of first frame, count, period, loop),
`tinyfont.h`, `displayfont.h`. `assets/vex_sheet.png` is a contact sheet of every frame.

## Workflow

```sh
python3 tools/make_sprites.py            # import art/ → headers (standard library only)
make art-check                           # verify committed headers match art/ without writing
make sim                                 # workbench; saving a PNG re-imports, rebuilds and reloads
python3 tests/simulator_test.py --update-golden   # after an intentional look change; review frames
python3 tools/render_gallery.py          # every screen state into assets/screens/
```

`make test` runs `tests/art_test.py`, which fails when a committed header is stale and checks that
the importer rejects off-palette pixels, partial alpha, wrong strip sizes, bad timing and missing
roles. The error names the file, frame and pixel, for example
`art error: strum.png frame 2 pixel (9,14): #FF00FF is not a theme colour`.

Goldens in `tests/golden/` are pixel-exact frames rendered by the native simulator. Any visible
change makes them fail on purpose; regenerate, look at the contact sheet, commit the new frames.
The WebAssembly build is compared against the same goldens, so the browser cannot drift from the
device renderer.

If the pet card frame used for the CAD texture changes (`tools/render_screen.py`), the hero
render's provenance goes stale and `make hero` must run (see [MARKETING.md](MARKETING.md)).

## How motion works

- `config.h` sets `DISPLAY_MS=100`. `drawScreen` quantises `View::now` to that cadence before
  anything is drawn, so the device, the native simulator and the browser pick the same frame for
  the same millisecond. Every `frame_ms` is a multiple of it; the importer refuses others. This is
  what removed the strum jitter: a 250 ms toggle sampled every 200 ms repeated and skipped frames.
- `ui.h::chooseAnim` picks an animation by state priority: level-up > fragment cheer > tuner
  listen > strum while sound is credited > sleep after 30 idle seconds > blink (one-shot every 4 s)
  > idle breathe. Loops index `(now / frame_ms) % frames`; one-shots run from their trigger.
- The quest ring is a continuous 4 px arc; its leading square alternates ink/gold every 500 ms
  while time is being earned. Unfilled fifths keep a dim tick.
- On the Teensy, `flush.h` compares the new frame with a shadow copy of what the panel shows
  (`DMAMEM`, RAM2) and sends only the changed rows as up to eight bands. Host measurements: a
  blink frame 2.4 KB, a strum step 31 KB, a breath 60 KB, versus 115 KB for the whole frame.
  Telemetry prints `tx_bytes`/`tx_max_bytes` each second so the bench can confirm the SPI budget;
  if it cannot keep up, set `DISPLAY_MS` back to 200 and re-import.

## Drawing guidance

- The panel is 32.4 mm across; a 32 × 32 sprite is drawn at 5× (160 px). One sprite pixel is about
  0.68 mm. Silhouette and two-tone contrast read; single-pixel detail does not.
- Keep feet and floor (rows 26–31) planted in idle, blink and strum so the character does not slide.
  A 1 px vertical bob is a breath; 2 px is a hop.
- New colours go in `theme.json` first. The sprite palette has room for 15; the display is RGB565,
  so pick values whose low bits are zero (`#C8A830` round-trips exactly, `#C9A931` does not) if
  you need the hex in the theme and the hex on screen to match.
- Preview at true size: the workbench has a 32.4 mm toggle. If a frame reads at that size on a
  laptop screen, it reads on the pedal.
- Timing: 100 ms per frame for an action, 200 ms for a bounce, 1000 ms for breathing or sleep.
  Anything under 100 ms cannot be shown.

## Contributing art

Original work only. By contributing files under `art/` you license them under the repository
licence; include a line in your pull request naming the author. Bundled fonts are DejaVu
(`assets/fonts/LICENSE.txt`). Do not trace copyrighted characters or ship art generated from
another project's sprites. Keep VEX a bat with charcoal, crimson and muted gold.
