---
name: vex-art-pipeline
description: Change VEX's sprites, animation timing, theme colours or bitmap fonts through the PNG/JSON sources in art/ and the importer, without editing C++. Use when the user asks for new poses, frames, palette tweaks, font changes, motion feel or an artist-facing explanation of how art reaches the pedal.
---

# VEX art pipeline

Read `docs/ART.md` first; it is the contract. Sources live in `art/` and the importer
`tools/make_sprites.py` generates `theme.h`, `sprites.h`, `anim.h`, `tinyfont.h` and
`displayfont.h` in `firmware/VEX_Pocket_Rival/`. Never hand-edit those headers.

## Steps

1. Decide what changes: a frame (`art/vex/<name>.png`), timing or a new animation
   (`art/vex/manifest.json`), colours (`art/theme.json`) or glyphs (`art/fonts/*.png`). A new
   animation also needs a trigger in `ui.h::chooseAnim`; that is the only C++ step and it stays in
   the shared renderer.
2. Edit the PNG in any pixel editor or with a short Pillow script in `build/venv`. Keep strips
   `32 × frames` wide, theme colours only, alpha 0/255, feet rows 26–31 planted. `frame_ms` must be
   a multiple of `DISPLAY_MS` in `config.h`.
3. Run `python3 tools/make_sprites.py`. Fix any `art error:` it reports; it names the file,
   frame, pixel and colour.
4. Look at the result: `assets/vex_sheet.png` for frames, `make sim` for motion at the real
   cadence (toggle true size), or `python3 tools/render_gallery.py` for every screen state.
5. `python3 tests/simulator_test.py --update-golden`, then review the changed goldens as a
   contact sheet before committing them. `make test` must pass (includes `tests/art_test.py` and
   the WASM golden comparison).
6. If the calibrated-paused pet frame changed, run `python3 tools/render_screen.py` and
   `make hero` so the hero provenance stays valid. Compile the target (`make firmware-all`) when a
   header the sketch includes changed; note flash/RAM in the handoff.

## Boundaries

- Sprite size is fixed at 32 × 32 and the index stays 8-bit; palette max 15 sprite colours.
- Do not add rendering work to the audio callback or allocation to the renderer.
- Original art only, licensed with the repository; no traced or generated copies of other
  characters. Keep charcoal, crimson and muted gold.
- Host renders are not evidence of SPI timing. Record `tx_bytes` from the bench before claiming
  the 100 ms cadence holds on hardware.
