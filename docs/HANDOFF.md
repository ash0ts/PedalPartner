# Current handoff — R1.1

Last prepared: 17 November 2026 (New York). Repository: [ash0ts/PedalPartner](https://github.com/ash0ts/PedalPartner).

## Where we left off

The user wants a feasible first pedal build with a mentor, retaining a rival-device / virtual-pet personality. Earlier concept images were appealing but too futuristic; the first practical CAD lost charm. R1.1 keeps the feasible mechanical/electrical architecture and transfers the color/character through small cosmetic changes. Do not return to a huge imaginary display or flatten the design into a generic box.

This is a **complete starter design package for review**, not a physically commissioned device. The next useful work is target-build verification and measured bench bring-up, followed by fit-coupon changes. Automatic CI covers only desktop tests. Target compilation must be verified locally or with the manual firmware workflow before bench use; it is not a publication prerequisite.

## Evidence

| Check | State / evidence |
|---|---|
| Shared core unit tests | Passed locally; `make test` reproduces |
| Native simulator integration | Passed: calibration, gating, quest, CRC fallback, reboot and rendered frames |
| CAD validity / collisions | Passed: 72 parts, 2,556 pairs, no modeled overlaps; `hardware/generated/checks/cad_validation.json` |
| Grouped STEP round trip | Passed: five groups and all 72 component names/bounds/volumes/solid counts; `step_roundtrip.json` |
| Target Teensy compilation | Not yet verified. Pinned local commands and an optional manual workflow are provided. Local toolchain downloads were blocked by HTTP 403 in the preparation environment. |
| Physical build / uploaded firmware | Not performed |
| Electrical, optical, thermal, foot strength | Not performed; blank acceptance rows intentionally remain blank |

## Extensibility review checkpoint

[EXTENSIBILITY.md](EXTENSIBILITY.md) records a source-reviewed proposal for portable practice rules, typed observations, session history/save migration, USB sync and bounded local/host coaching policies. Core rules and rendering are already shared with the simulator; versioned host commands, session history, adaptive policies, migration tooling and a companion app are not implemented.

This is a documentation-only proposal. Firmware, physical parts and the R1.1 fabrication packet are unchanged. Review covered `core.h`, `ui.h`'s existing role, `.ino`, `config.h`, existing decisions/development guidance and PJRC specifications. Source-to-manifest checks and local Markdown-link checks were used for the proposal; existing software/CAD evidence is reused, with no new firmware or CAD test claim. The next action remains target compilation and measured bench bring-up.

## Recovery and lightweight CI

The interrupted publication checkpoint was recovered intact. The publication-only changes simplify automation: one Linux desktop job, no OS matrix, manual firmware/CAD workflows, bounded runtimes, and short log excerpts for agents. Firmware build failures still retain their report, and the local build command still attempts all four stages. Existing CAD and simulator evidence is retained; no physical or target-build success is inferred from it.

Publication recovery now has an explicit no-payload-in-context rule in `AGENTS.md` and [publishing instructions](PUBLISHING.md). The interrupted encoder must not be resumed by printing or manually relaying file chunks. Reuse recorded validation for unchanged hardware/software.

## Exact next actions

1. For desktop exploration, run `make sim` (host tests already pass). Before hardware bring-up, run `make setup-firmware && make firmware-all` on a connected Mac, or launch the manual firmware workflow once. Fix target compiler failures before bench use.
2. Order the bench electronics from `docs/SHOPPING.md`. Confirm Teensy 4.1, Audio Adapter D/D2 and Adafruit 6178 identities.
3. Meet the mentor with the guide and actual parts. Prove stage 1 and stage 2 before audio wiring. Test display readability at the intended pedalboard position.
4. Measure header gap, audio-board revision/component heights, jack body/lugs/thread, USB plug, button stack and footswitch travel. Enter actual values in `hardware/data/measurements.csv`.
5. Update CAD if needed, regenerate/export/check, then print the fit coupon. Order/cut the metal plate and window only after fit decisions.
6. Wire the attenuator, validate stages 3/4, then enclose and complete `acceptance_log.csv`. Test structural loading with electronics removed before foot operation.

## Critical unknowns

- Header seating is modeled at 9.4 mm; exact socket/pin combinations and access to used pins need bench confirmation.
- Audio board community CAD may differ from purchased D2. The three mount holes and component clearance must be measured.
- Simplified switch/jack bodies do not certify solder-lug or inserted-plug clearance.
- The 32.4 mm screen may be too small for a standing tuner; pet charm can work seated while tuner legibility needs its own test.
- No force rating exists for the printed enclosure. Metal plate/column contact and layer adhesion require a mentor-defined load test.
- Actual pitch stability, ADC gain, USB noise and power-loss behavior have not been characterized.

## Source map and continuation prompt

`AGENTS.md` maps authoritative files. `docs/DECISIONS.md` explains constraints. `docs/PLAN.md` has milestone acceptance. `docs/RESEARCH.md` links the primary sources behind the agent setup and toolchain.

Suggested next-agent prompt:

> Read AGENTS.md and docs/HANDOFF.md. Continue PedalPartner R1.1 without redesigning the product. Start with the requested task and reuse evidence for unchanged work. Keep output short; do not run optional firmware/CAD builds unless the task needs them. Before physical bring-up, verify all four Teensy stages. Preserve the real display size, serviceable CAD groups, USB-only power and buffered dry-input architecture. Report measured facts separately from modeled assumptions. Update the handoff with results and the exact next step.

Repo-local skills are in `.agents/skills/`. Pi can discover that directory after project trust; invoke `/skill:vex-cad-review` or `/skill:vex-firmware-change`. Other agents can read the same SKILL.md directly if their discovery convention differs. Do not import private global session history into this public repository.
