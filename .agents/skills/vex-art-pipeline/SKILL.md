---
name: vex-art-pipeline
description: Change VEX's sprites, animation timing, theme colours or bitmap fonts through the PNG/JSON sources in art/ and the importer, without editing C++. Use when the user asks for new poses, frames, palette tweaks, font changes, motion feel or an artist-facing explanation of how art reaches the pedal.
---

# VEX art pipeline

Read `docs/ART.md` first; it is the contract. Sources live in `art/` and the importer
`tools/make_sprites.py` generates `theme.h`, `sprites.h`, `anim.h`, `tinyfont.h` and
`displayfont.h` in `firmware/VEX_Pocket_Rival/`. Never hand-edit those headers.

## Steps

1. Decide what changes: a frame (`art/vex/<name>.png`), timing, play order or a new animation
   (`art/vex/manifest.json`), colours (`art/theme.json`) or glyphs (`art/fonts/*.png`). A new
   animation also needs a trigger in `ui.h::chooseAnim`; that is the only C++ step and it stays in
   the shared renderer. To play drawn frames in an order such as ping-pong, add
   `"sequence": [0,1,2,1]` instead of drawing duplicates; steps, not drawn frames, are what the
   simulator, scenarios and telemetry report as `frame`.
2. Edit the PNG in any pixel editor or with a short Pillow script in `build/venv`. Keep strips
   `32 × frames` wide, theme colours only, alpha 0/255. For loops that run for minutes (idle,
   listen, sleep) keep feet, head outline and floor planted and change at most a few rows per
   step; celebrations (cheer, levelup, strum) may move the whole bat. `frame_ms` must be a
   multiple of `DISPLAY_MS` in `config.h`.
3. Run `python3 tools/make_sprites.py`. Fix any `art error:` it reports; it names the file,
   frame, pixel and colour. Read the **step cost** table it prints: rows and bytes each step
   sends to the panel (a full frame is 240 rows / 115,200 B). Idle steps must stay under 30 % of
   the rows (`core_tests.cpp` enforces it); treat anything over about 60 rows in a long-running
   loop as a redesign, not a timing tweak.
4. Look at the result: `assets/vex_sheet.png` for frames, `make sim` for motion at the real
   cadence (toggle true size), or `python3 tools/render_gallery.py` for every screen state.
5. `python3 tests/simulator_test.py --update-golden`, then review the changed goldens as a
   contact sheet before committing them. `make test` must pass (includes `tests/art_test.py` and
   the WASM golden comparison). If an animation's step count or order changed, update the
   `frame=` expectations in `tests/scenarios/personality.txt` and run `make motion`, which fails
   when a loop skips or reverses a step and re-encodes `assets/motion/`.
6. If the calibrated-paused pet frame changed, run `python3 tools/render_screen.py` and
   `make hero` so the hero provenance stays valid. Compile the target (`make firmware-all`) when a
   header the sketch includes changed; note flash/RAM in the handoff.

## Boundaries

- Sprite size is fixed at 32 × 32 and the index stays 8-bit; palette max 15 sprite colours.
- Do not add rendering work to the audio callback or allocation to the renderer.
- Original art only, licensed with the repository; no traced or generated copies of other
  characters. Keep charcoal, crimson and muted gold.
- Host renders and the importer's step cost are predictions, not SPI timing. Record `flush_us`
  and `tx_bytes` from the bench (`make monitor` or the workbench Device tab with
  `make sim DEVICE=auto`) before claiming the 100 ms cadence holds on hardware.
