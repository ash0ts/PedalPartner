# VEX / Pocket Rival — product brief for a design studio

This is the document to read if you have been asked to take VEX from a working hobbyist prototype towards a manufacturable product. Everything in this repository was made by one person as a first electronics build; it is a proposal with stated evidence, not a validated design. `make pack-studio` packages this brief with the files it refers to.

## 1. What the product is

A small guitar practice companion that sits on a pedalboard. A pixel-art bat on a round 32.4 mm screen reacts while you play: it earns participation XP from detected playing, runs short timed practice quests (5/10/20 minutes), awards one fragment per completed session, and offers an experimental single-note tuner. It listens on a buffered dry branch of the player's signal and never touches the audible guitar path.

Who it is for: beginning and returning guitarists who need a reason to pick the instrument up for ten minutes, and who respond to a character rather than a chart. It is not a tutor, a metronome or an effects pedal.

What it is not, by design: no audio output, bypass or muting; no 9 V pedal power; no technique scoring, calendar streaks, death, debt or punishment. See `docs/DECISIONS.md` for the reasoning behind each exclusion.

## 2. What is fixed

These define the product. Changing them means a different product, so treat them as the contract.

| Fixed | Why | Where |
|---|---|---|
| The character: original pixel bat, its animations and screen layouts | This is the personality people respond to; it is the product | `art/` (PNG + JSON), `assets/motion/`, `assets/screens/`, `docs/ART.md` |
| Charcoal / crimson / muted gold palette, clipped round bezel, thin gold insert, small crimson cheek | The "rival device" identity; renders in `assets/marketing/` | `art/theme.json`, `hardware/cad/build_cad.py` |
| Real 240 × 240 round display, about 32.4 mm active | The character is drawn for this resolution; a larger screen is a V2 decision with its own art | `firmware/VEX_Pocket_Rival/config.h`, BOM E03 |
| Three controls: MODE, QUEST, footswitch; one hold time (1.2 s) | Simple, foot-operable, no menus | `docs/BUILD_GUIDE.md` controls page |
| Input: buffered line-level dry signal only, through the documented attenuator | Protects the player's rig and the codec; no passive-pickup loading claims | `hardware/data/wiring.csv`, guide "Build the audio attenuator" |
| Power: USB 5 V only; all logic 3.3 V | Keeps the first product out of pedalboard power-supply and isolation problems | `AGENTS.md` invariants |
| Behaviour rules: boot paused and uncalibrated; silence, stale samples, clipping and tuner mode earn no time; one fragment per completed session | Trust in the XP number | `firmware/VEX_Pocket_Rival/core.h`, `app.h`, scenario tests |
| Saved progress survives power loss (two CRC-protected records) | Nobody loses their bat | `core.h`, `tests/scenarios/` |

## 3. What is open

These are the parts the prototype solves the hobbyist way and that a studio is expected to redesign.

| Open | Prototype answer | Questions for you |
|---|---|---|
| Enclosure manufacture | 3D-printed PETG base and lid, flat polycarbonate window, 2 mm aluminium foot plate on four columns | Material, process, cost at 100 / 1 000 units; foot-load path with a rating; sealing and finish that keeps the palette |
| Electronics integration | Teensy 4.1 + PJRC Audio Adapter + Adafruit 6178 as purchased modules, hand-wired on a perfboard | Custom PCB with the same pin ownership and codec (or a justified substitute), connectors, EMI/ground strategy, test points |
| Enclosure size | 86 × 146 × 45 mm, driven by the stacked modules | Smaller is welcome if the 32.4 mm screen, foot clearance and jack stay |
| Window and viewing | Flat clear disc; readability at pedalboard distance is unmeasured | Dome, anti-glare, bezel depth |
| Footswitch feel | P-H605-BLK soft switch on a printed plate | Whatever survives a product test cycle |
| Compliance | None | Whatever the target market requires for a USB-powered accessory |
| Cost | Parts allowance $140–260 as modules (`docs/SHOPPING.md`) | BOM and tooling targets at volume |

## 4. What we are asking for

Choose the scope that fits; each row stands alone.

1. **Industrial design review:** form, finish, readability and foot ergonomics against the renders and the fixed palette.
2. **Mechanical DFM:** an enclosure that can be made at volume, with a stated force rating for the foot path, derived from the grouped STEP and `hardware/generated/parameters.json`.
3. **Electrical productisation:** a schematic and PCB that preserve the pin ownership in `config.h`, the codec and display identities, USB-only power and the attenuator topology, so the existing firmware runs without change.
4. **Compliance and test plan** for the target market.

Deliverables we can take back: STEP or native CAD, schematic and board files, a DFM report, a cost model and a test plan. We will keep firmware, art, simulator and tests in this repository and update the build guide from your results.

## 5. What is here and how to read it

Paths are repository paths. The studio ZIP (`make pack-studio`) carries the subset its `START_HERE.md` lists; everything else, including firmware, art sources and the simulator, is at https://github.com/ash0ts/PedalPartner.

| Need | Open |
|---|---|
| Appearance | `assets/marketing/vex-hero.jpg`, `vex-social.jpg`; provenance in `assets/marketing/provenance.json` (renders of the real CAD, no generative imagery) |
| Character in motion and every screen state | `assets/motion/vex-motion.mp4`, `assets/motion/storyboard.png`, `assets/screens/contact_sheet.png` |
| Geometry | `hardware/generated/VEX_Pocket_Rival_Assembly.step` (five service groups, 72 solids, mm); `parts.json` says which solids are original, vendor or community reference; `parameters.json` lists driving dimensions; `docs/CAD_REVIEW.md` maps the files |
| Electrical contract | `hardware/data/BOM.csv`, `wiring.csv`, `fasteners.csv`; `firmware/VEX_Pocket_Rival/config.h`; attenuator and stack pages of the guide |
| Behaviour | Run it without hardware: `make test && make sim` opens the firmware as WebAssembly in a browser (`docs/LOCAL_DEVELOPMENT.md`) |
| Why | `docs/DECISIONS.md`; hardware revisions already considered: `docs/EXTENSIBILITY.md` |
| Current evidence and limits | `docs/HANDOFF.md`; the bench plan that will produce the first numbers: `docs/BRINGUP.md` |
| Licence and provenance | MIT for original work (`LICENSE`); vendor and community CAD and the font carry their own terms (`docs/THIRD_PARTY.md`, `hardware/vendor/PROVENANCE.md`) |

Files for agents (`AGENTS.md`, `.agents/skills/`, `docs/PLAN.md`) describe how this repository is maintained; you can ignore them.

## 6. What is deliberately not here

- No schematic file or PCB. The wiring is a table of module pins; the attenuator is three parts on a perfboard.
- No measured data. `hardware/data/measurements.csv` and `acceptance_log.csv` are blank until the first bench session (`docs/BRINGUP.md`). Nothing has run on a board yet; firmware compiles for the target and runs identically in the simulator.
- No force rating, drop, thermal or EMC result. The printed enclosure is hand-operation only until a load test.
- No certification of any kind. A valid STEP and passing tests are not a fit, strength or electrical certificate.
- No native CAD timeline. Geometry is generated by `hardware/cad/build_cad.py` (build123d); the STEP has named solids only.
- No user research beyond the maker's own practice.

## 7. Known risks

| Risk | Evidence so far | What settles it |
|---|---|---|
| 32.4 mm screen is too small at pedalboard distance | Chosen for a documented driver; readability unmeasured | Gate 1 in `docs/BRINGUP.md`: the real screen at the real angle before any enclosure part is printed |
| Foot load through a printed shell | Aluminium plate on four columns; no rating | Load test without electronics; your DFM |
| SPI redraw cannot hold the 100 ms animation cadence without tearing | Host prediction 2–60 KB per step with dirty bands; instrumented (`flush_us`, `tx_bytes`) but unmeasured | Gate 2 in `docs/BRINGUP.md`; a DMA driver is the fallback behind an adapter |
| The player's rig has no always-live buffered dry feed | Acceptance row 12 | A buffered splitter is the documented answer; a product may need to bundle one |
| Detecting "playing" from level alone credits noise | Calibration, gate, stale/clip exclusion, tested on synthetic tones and a reference WAV | Real guitars in acceptance rows 07, 08, 13 |

## 8. Working together

Issues and pull requests on GitHub are the record. Measured numbers go in the two CSVs with the measurer's initials in `checked_by`; never fill a measured field with an inferred value. Keep anything you redesign behind the same behaviour tests (`make test`) and, if you change the panel or codec, behind the adapters the firmware already has (`Panel` in the sketch, `analyzer.h`).
