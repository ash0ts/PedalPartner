# Guitar drills (`ACT_DRILL`)

VEX / Pocket Rival provides five single-note guitar drills for an intermediate player on top of the shared practice state machine, block-energy onset detector (`analyzer.h`) and note tracker (`notes.h`).

## Rules and boundaries

- **Participation XP stays separate.** Playing during any drill earns practice time and quest fragments under the same gate, 40 ms smoothing and 500 ms activity hold as the Pet screen; drill scores never grant or withhold XP.
- **Personal bests live in their own EEPROM block.** `Skills` v1 occupies two 64-byte CRC-32 slots at EEPROM `128` and `192` (`SKILLS_SLOT_A` / `SKILLS_SLOT_B` in `core.h`), keeping `Record` v1 (`0` / `64`, 48 B) and `LinkState` (76 B) unchanged.
- **No audio output.** Tempo drills flash `PIN_LED` from a 1 kHz `IntervalTimer` (`75 ms` on beat 1, `32 ms` on beats 2–4) so SPI display flushes cannot jitter the visual beat, and self-paced modes work alongside an external metronome.
- **Controls keep one meaning everywhere.**
  - `MODE` tap: next screen (`Pet → Tuner → Drill`). Hold (`1.2 s`): save `Record` and `Skills`.
  - `QUEST` tap while paused: next choice on this screen (quest goal on `Pet`, drill preset on `Drill`). Hold (`1.2 s`): 3 s noise-floor calibration.
  - `FOOT` tap: start or pause. Hold (`1.2 s`): new session.

---

## What each drill measures — and cannot measure

| Drill & presets | Pitch profile | What it measures | What it cannot measure | Saved personal best (`Skills`) |
|---|---|---|---|---|
| **Timing trainer (`PULSE`)**<br>`pulse-qtr`, `pulse-8th`, `pulse-free` (`drill_pulse.h`) | `PITCH_OFF` (0 YIN work) | Picked attack times (`Onset` ring, 2.9 ms block resolution) against a 4-bar quarter/eighth grid anchored on your first note, or against your own median inter-onset spacing (`pulse-free`). Reports early/late bias (`RUSHING` / `LATE` / `LOCKED`) and timing spread in ms; climbs `+4 BPM` after a clean 4-bar round (`≤ 30 ms`) and drops `-4 BPM` after two consecutive misses, starting `8 BPM` below your saved best. | Hammer-on-only / pull-off-only passages (no energy jump — pick every note), swung subdivisions, or polyphonic chord strums whose string-to-string rake spans >15 ms. | Best clean tempo (`BPM`, higher is better) |
| **Fretboard note finder (`HUNT`)**<br>`hunt-any`, `hunt-str` (`drill_hunt.h`) | `PITCH_TUNER` (`2048` win / `2048` hop, `55–1400 Hz`) | Reaction time from prompt to settled monophonic pitch across 10-note seeded rounds (`hunt-any` matches pitch class in any octave; `hunt-str` checks the exact MIDI pitch of frets `0–11` on the named string, plus fret-12 octave on an open prompt). Repeats slow pitch classes more often. | Which physical string/fret produced an enharmonic unison when two strings share the same MIDI pitch (`hunt-str` verifies exact octave + pitch class, not pickup timbre), or chords. | Best average find time (`centiseconds`, e.g. `52` = `0.52 s`, lower is better) |
| **Bend & vibrato (`BEND`)**<br>`bend-half`, `bend-whole` (`drill_bend.h`) | `PITCH_FAST` (`1024` win / `512` hop = `11.6 ms`, `140–1400 Hz`) | Upward pitch excursion from a settled fretted base note (`≥ 140 Hz`, D3 and up) to `+100 c` or `+200 c`, locking cents error once the bend apex holds steady (`~58 ms`). On unbent held notes with regular periodic wobble, reports live vibrato rate (`Hz`) and peak-to-peak width (`cents`) beside a typical `5–7 Hz`, `25–70 c` range. | Pre-bends struck already bent (no unbent base note to reference), release-only bends, bends starting below D3 (`146.8 Hz`), or double-stop unison bends. | Best 8-bend median `|error|` (`0.1 c` units, min `1`, lower is better) |
| **Scale run checker (`RUN`)**<br>`run-penta`, `run-major`, `run-minor` (`drill_run.h`) | `PITCH_RUN` (`2048` win / `1024` hop = `23.2 ms`, `55–1400 Hz`; auto-falls back to `2048` hop if bench `yin_us > 14,000`) | Sets the key root from your first pitched note in each 16-note phrase, flashes 12 chromatic degree dots gold (in scale) or red (outside, counted), catches ≥1-semitone hammer-on steps via `NoteTracker`, and scores picked-note spacing evenness (`≤ 30 ms` spread). | Modal harmonic context when you start on a non-root scale degree (always start the 16-note phrase on the intended root), ultra-fast shredding (`> ~160 BPM` sixteenths where notes are shorter than a 46 ms YIN window), or two-note chords. | Best clean eighth-note tempo (`BPM` with `0` outside notes and `≤ 30 ms` spread, higher is better) |
| **Picking dynamics (`TOUCH`)**<br>`touch-even`, `touch-acc3`, `touch-acc4` (`drill_touch.h`) | `PITCH_OFF` (0 YIN work) | Per-note 36 ms attack crest level in dB against your running median across 12-note rounds. `touch-even` scores overall dB spread; `touch-acc3` and `touch-acc4` verify every 3rd or 4th note is `≥ +4.0 dB` above the unaccented median with no misplaced accents. | Tone colour / pick angle independent of peak amplitude, palm-mute warmth at equal peak level, or compressor-flattened signals (place VEX on a dry, uncompressed buffered branch). | Best clean dB spread (`0.01 dB` units, min `1`, lower is better) |

---

## Recipe: adding a new drill

Shared groundwork keeps a new drill confined to one header, one table row and one scenario:

1. **Skill slot (`core.h`)**: pick one of the three reserved `SkillId` entries (`SKILL_RSV5..SKILL_RSV7`) in `Skills` v1 so the 64-byte EEPROM layout stays version 1 without migration.
2. **Drill header (`firmware/VEX_Pocket_Rival/drill_<name>.h`)**: write a plain zero-heap struct with `configure(...)`, `start(now_ms, event_seq)`, `update(const NoteTracker&, now_ms, SkillStat&, bool& newBest)` and `fillView(DrillView&, title, SkillStat, running, now_ms)`. Reuse one of the five `DrillVizKind` layouts in `drill_view.h` (`VIZ_TIMING`, `VIZ_PROMPT`, `VIZ_BEND`, `VIZ_SCALE`, `VIZ_BARS`) so `ui.h` needs no new drawing branch unless a genuinely new chart is warranted.
3. **Preset row (`firmware/VEX_Pocket_Rival/drills.h`)**: add the enum tag to `DrillFamily`, one line per preset in `DRILL_PRESETS[]` (hyphenated slug such as `"my-drill"` so scenario `expect activity=my-drill` matches verbatim), and wire the `switch` cases in `DrillHost`. The WebAssembly workbench exports `DRILL_PRESETS[]` directly (`vex_preset_count` / `vex_preset_slug` / `vex_preset_title`), so preset pills appear automatically in `make sim`.
4. **Workbench performance (`simulator/web/app.js`)**: add a branch in `playDrillPerformance(kind)` using the shared `notes` / `bend` / `vib` protocol commands so **Steady take**, **Rushing take**, **Uneven take** and **Wrong notes** demonstrate the drill in one click.
5. **Tests & goldens**: add `tests/scenarios/drill_<name>.txt` (`GOLDEN_GROUPS` in `tools/web.py` groups it under **Guitar drills**), generate and visually inspect golden PNGs (`python3 tests/simulator_test.py --update-golden`), add a labelled case in `test_audio/drills/` if audio detection changed, and run `make test`, `make test CXX=g++-16` and `make firmware-all`.

---

## Fifteen-minute guitar clip set (`Record clip` in `make sim`)

Until real pickup recordings are added, onset and pitch thresholds are tuned against `test_audio/guitar_notes_left_only.wav` and eight synthetic `test_audio/drills/*.json` clips (`make eval`). With a guitar and USB audio interface connected, open `make sim`, select **Mac / interface input**, type each take name into the **Record clip** box, click **Record clip**, play the take dry (no compressor/overdrive), click **Stop & save WAV**, drop the downloaded `.wav` into `test_audio/drills/` with a matching `.json` label file (`"kind": "real"`), and run `make eval`:

1. `real_01_quiet_floor` — 5 s hands muted on strings at normal guitar volume (`expected: []`, verifies 0 false onsets).
2. `real_02_quarter_92bpm` — 16 picked quarter notes on low `E2` and `A2` with a 92 BPM click (`expected`: 16 notes).
3. `real_03_eighth_110bpm` — 16 alternate-picked eighth notes on `D3` and `G3` at 110 BPM (`expected`: 16 notes).
4. `real_04_fretboard_octaves` — single notes `F#2`, `F#3`, `F#4`, `C3`, `G3`, `Bb3`, `D4`, `A4` (`expected`: exact 8-note MIDI sequence).
5. `real_05_bends_half` — four half-step bends (`+100 c`) on the G and B strings, holding each apex ~0.4 s (`expected`: 4 notes).
6. `real_06_bends_whole` — four whole-step bends (`+200 c`) on G3 (`7th fret`) and B3 (`8th fret`), holding each apex ~0.4 s (`expected`: 4 notes).
7. `real_07_vibrato_e4` — two 1.5 s held notes on `E4` (`B string 5th fret`) with steady finger vibrato (`expected`: 2 notes, no extra onset splits).
8. `real_08_run_a_minor_penta` — 16-note ascending/descending A minor pentatonic picked run (`A2 C3 D3 E3 G3 A3 ...`).
9. `real_09_legato_hammers` — 8-note phrase mixing picked attacks and fretted hammer-ons/pull-offs (`A2 -> C3`, `D3 -> E3`, `G3 -> A3`).
10. `real_10_touch_acc3` — 12 picked notes on `A2`/`D3` accenting every 3rd pick firmly and playing the two inner picks softly (`expected`: 12 onsets).
