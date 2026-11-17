# Design decisions — R1.1

| Decision | Reason / boundary |
|---|---|
| Keep the real 1.28-inch screen | A documented Adafruit board and GC9A01A driver support a first build. Concept-art screen proportions are aspirational. If standing readability fails, make a deliberate new hardware revision. |
| Transfer the rival vibe through details | Charcoal octagonal bezel, thin muted-gold rim, crimson cheek/base band/buttons, shallow panel marks and original pixel bat preserve personality without a complex shell. |
| Flat 40 × 1 mm window | An accessible, replaceable cover. A fishbowl lens remains a later optical experiment requiring glare/distortion tests and its own retention design. |
| Listen on a buffered dry branch | Keeps prototype software out of the audible signal path. The passive attenuator is not a high-impedance guitar preamp. No audio output or mute function is implemented. |
| USB 5 V only | Avoids adding a 9 V protection/regulation design to the first build. USB ground noise must still be tested with the actual rig. |
| Participation XP, one quest fragment per session | Clear and testable motivation. Audio activity is not evidence of good timing, intonation or technique. No calendar clock means no true daily streaks. |
| Five STEP service groups | Makes review and removal of the lid/carrier practical. Hardware belongs to its mating assembly; wiring reserves are separate. |
| Python construction plus neutral solids | Reproducible source and broad CAD interoperability. STEP has no native feature history. |
| Shared C++ logic and renderer | Host simulation exercises the actual rules/pixels. It injects signal measurements rather than pretending to emulate SGTL5000/DSP. |
| Two CRC-protected save records | Recovers the last valid slot after a damaged write. Hardware power interruption and EEPROM wear still need validation; changing format requires migration. |
| Lean AGENTS.md plus two task skills | Always-load context stays small; detailed procedures and evidence are read when relevant. No session-history importer is required. |

Rejected for R1.1: large fictional screen, external ears/armor, custom dome, RGB spectacle, audio bypass, mains/battery/9 V conversion, broad game systems and speculative automatic technique scores. These are scope decisions, not permanent bans.
