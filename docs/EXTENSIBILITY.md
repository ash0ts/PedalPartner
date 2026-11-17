# An extensible practice companion

Status: architecture proposal, 17 November 2026. Reviewed against R1.1 commit `b97ae66db5bb4e309c0f24ae5083732490a7c6ce`. The interfaces and later features described below are not implemented unless explicitly identified as existing.

## Product boundary

VEX should help its owner start, sustain and reflect on guitar practice through a responsive original character. R1 remains a USB-powered listener on a buffered dry branch. Keep the first physical build small while preserving a path to richer exercises, adaptive coaching and replacement hardware.

Buffering, inline audio output, mute switching and drop-tuning are outside the current build. They require a separately reviewed electrical and mechanical revision. They are not prerequisites for an extensible practice companion.

The useful long-term asset is the practice model, measured observations, session history, original content and test suite. The processor board is one implementation of those assets. Do not promise a future processor will be pin-compatible, fit the same enclosure, or automatically solve recognition accuracy or audio latency.

## What the current code actually provides

| Existing foundation | Limitation to address when extending |
|---|---|
| `core.h` has platform-independent practice rules, pitch-to-note math, button handling and progress record helpers. | Goals are fixed at 5/10/20 minutes. This is a compact core, not a general exercise or policy framework. |
| `ui.h` is shared between hardware and desktop simulation. | Layout and artwork target the current 240 x 240 round display; other screens require layout adaptation. |
| `.ino` owns audio capture, GPIO, screen transport, USB serial and EEPROM access. | These responsibilities still share one sketch; extract small adapters as a concrete feature requires them. |
| Two CRC-protected progress records with sequence numbers and a version field. | Version 1 stores aggregate/current-session state, not a history. It accepts only version 1; a migration mechanism is not implemented. Native struct bytes are not a portable interchange format. |
| USB serial accepts `c`, `s`, `p`, `r` debug commands and prints telemetry. | There is no framed/versioned application protocol, request acknowledgement, duplicate protection, capability negotiation or companion app. |
| Simulator tests actual rules and rendering with injected observations. | It does not validate the audio detector, physical screen timing, EEPROM power interruption, or real hardware resource headroom. |

Target firmware compilation and all physical acceptance remain pending. This document does not upgrade their status.

## Software boundaries to preserve

Use a few explicit modules/functions rather than a plugin system or runtime-loaded code on the microcontroller. Existing behavior must keep working while one boundary is introduced at a time.

| Boundary | Responsibility | Portable representation |
|---|---|---|
| Hardware adapters | Capture audio observations, read controls, draw pixels, provide a monotonic clock, persist bytes and transport messages. | No PJRC, display-driver or GPIO types in practice rules. |
| Observation layer | Describe what the sensors actually measured. | Age/time, RMS, peak, clipped/fresh flags, calibration/configuration identity, and pitch/confidence only when available. Unknown is distinct from zero. |
| Practice core | Own session state, activity accounting, pause rules and reward eligibility. | Typed actions/events plus deterministic state transitions; hardware timing does not define game rules. |
| Exercise definitions | Describe supported goals and completion rules. | Stable exercise IDs, definition version and required measurement capabilities. Start with compiled definitions; add downloadable data only when justified. |
| Coaching policy | Choose a useful suggestion from the supported exercises and character responses. | Observations/session summaries/user preferences in; a bounded proposal with a reason out. It cannot directly grant XP or change measured history. |
| Presentation/content | Render a small view model with original artwork and concise prompts. | State and content IDs rather than device pin numbers or arbitrary generated drawing commands. |
| Companion transport | Exchange summaries, preferences and validated proposals with an optional computer app. | Versioned messages; no requirement to be connected for normal practice. |

Continue using the same practice core in the simulator. Use recorded input sequences to compare behavior before and after changes. Pitch/technique analysis needs real audio evaluation in addition to the injected-observation tests.

## What "agentic" should mean here

A useful loop is: observe practice -> consider the owner's goal and recent outcomes -> propose a supported next exercise -> let the owner accept -> observe the next session -> adjust the following suggestion.

Example: the owner reports limited time today; recent sessions show that a five-minute goal is usually completed. VEX offers that goal and explains why. It does not infer poor technique, injury, fatigue or emotional state from loudness or session length. Any self-reported difficulty remains labeled as user feedback.

### On the pedal

- Immediate pet reactions, calibrated activity detection, the active session, progress saving and a simple deterministic suggestion policy work offline.
- Proposals can select among supported goals, offer a break or choose a bounded presentation cue. Current rules still require pausing before a goal change; physical controls retain priority.
- Agent output cannot manufacture practice time, start a session unexpectedly, change calibration during playing, reset progress, flash firmware or execute code. Those operations are outside the coaching action interface.
- Audio processing never waits for planning, networking, screen rendering, logging, allocation or storage writes. If a policy cannot finish within its budget, retain the existing session and local behavior.

### Optional Mac companion, later

- A local USB bridge can sync compact session summaries, collect the owner's goals/self-reports and show a longer reflection or practice plan.
- An LLM-based planner or other agent can run on the computer, using a local or explicitly configured remote model. R1 makes no claim of running a general language model on Teensy.
- The planner uses a small application API, such as `get_session_summaries`, `get_supported_exercises` and `propose_next_exercise`. The device validates every resulting command. An MCP wrapper, if wanted, belongs around the host application API; the pedal does not need an MCP runtime.
- No raw-audio upload is needed for these flows. Any later recording/export feature gets its own storage, consent and bandwidth design. An unavailable computer/model must not interrupt a session.
- Retain the evidence IDs, policy/model version, proposed action, concise reason, acceptance/rejection and outcome. This supports debugging and evaluation without storing hidden model reasoning.

This is an extension path, not a requirement to build a desktop agent before enjoying the pedal. Start with hand-written policies and demonstrate a user benefit before adding a model.

## Data and connection decisions

1. **Version the interfaces independently.** Track firmware, hardware revision, message schema, save schema and exercise definition versions separately. A handshake advertises actual capabilities; do not infer them from board name. Current activity sensing must not advertise onset accuracy, chord recognition or technique scoring.
2. **Use USB first.** Keep the accessible Micro-B data connection for programming, debugging and later host sync. A future transport can carry the same messages. No new Wi-Fi/Bluetooth radio, external connector or SD card is required by this proposal. A phone connection is not assumed to be available.
3. **Bound every request.** Before exposing the host API, specify framing, maximum message size, timeouts, request IDs, replies and rate limits. Keep debug logs distinct from machine messages. Reject unsupported versions/actions and invalid ranges without changing active-session state. Parse outside the audio callback.
4. **Make retries harmless.** Session summaries need persistent IDs and deduplicated imports. Mutating proposals need acknowledgement and retry handling. Reconnecting must not create a second reward or silently replay a stale command. Specify behavior across restarts before enabling mutations.
5. **Add a small history deliberately.** Start with duration, active time, exercise/version, completion, source/calibration metadata and any explicit user feedback. A bounded, wear-aware session-summary store is future work; choose record count and retention only after storage accounting. Do not continuously log audio measurements to EEPROM. Export longer history to the host.
6. **Distinguish time sources.** Session timing uses a monotonic clock with wraparound handling. R1 has no configured, backed-up calendar-time system. Host-provided dates need explicit provenance; unknown dates stay unknown. A built-in RTC peripheral alone does not establish reliable daily streaks.
7. **Protect progress across upgrades.** Introduce explicit fixed-width byte serialization and migration tests before changing the existing record layout. Preserve the old valid record until migration is verified; unknown future formats must not be silently reset. Make backup/restore and interrupted migration tests part of the first save-format change. Full migration/backup tooling does not exist today.

## Hardware choices that support this direction

Keep Teensy 4.1 + Audio Adapter D/D2 for initial bring-up. PJRC documents a 600 MHz Cortex-M7, 1 MB RAM, approximately 8 MB program flash, USB device communication and audio interfaces. This is a reasonable development platform for the present sensing/UI/rules workload, not evidence of measured spare capacity [1].

- Keep the removable electronics tray, serviceable fasteners and accessible programming connection. At fit review, verify that the intended board/header combination is actually removable and that unplugging it does not damage other wiring. The current CAD does not certify socketability of every board.
- Document each signal, voltage, connector and pin assignment. Preserve the existing allocations. Treat unused MCU pins as candidates requiring review, not as a promised expansion connector; several buses and select pins are already shared/reserved.
- Prefer replaceable display/control panels and carrier revisions where the existing construction permits them. Do not enlarge R1 to reserve space for unspecified future electronics. V2 may require a new tray, faceplate or complete enclosure.
- Measure supply current, heat, audio noise, memory use, worst-case processing time, display stalls and save behavior under load. The 240 x 240 RGB565 framebuffer alone is 115,200 bytes. At the configured 16 MHz SPI rate, sending a full frame has a theoretical minimum wire time of 57.6 ms, before overhead; this is a calculation, not a bench measurement. The current sketch sends a full frame every 200 ms. Measure the impact on observation handling before adding animation or analysis workloads.
- PJRC specifies 4,284 bytes of emulated EEPROM; writes can stall execution from flash [1,3]. Schedule and validate saves accordingly. Neither spare program flash nor optional memory pads are automatically an implemented history/asset store.

USB sync and richer policies are primarily software extensions. Wireless connectivity, reliable offline calendar time, a different display, extra controls, changed power input, inline audio and new connectors each require a hardware review. The existing fabrication packet remains an R1.1 listening-companion packet.

## Implementation sequence and acceptance

| Step | Deliverable | Acceptance |
|---|---|---|
| R1: prove the buddy | Existing target builds, screen/controls, calibrated listener, one useful quest and saved progress. | Complete the existing PLAN.md milestones and physical acceptance log. No agent dependency. |
| First extension: make facts reusable | Small adapter boundaries, typed observations/actions and exercise definitions; preserve existing behavior. | Same recorded input produces the same activity/rewards in simulator and core; invalid/stale/clipped samples stay excluded. Compile all target stages after firmware changes. |
| History and USB sync | Versioned summaries/save migration, capability handshake and read-only export first. | Reboot/migration/corrupt-record fallback; bounded storage; disconnect and duplicate-import tests; no calendar claims without trustworthy dates. |
| Local adaptive buddy | A deterministic policy suggests a supported goal using explicit preferences and recent outcomes. | Owner can decline; active goals do not change mid-session; suggestions cannot alter earned progress. Evaluate usefulness with actual sessions. |
| Optional host agent | A Mac bridge and constrained planning API, then an optional model-backed policy. | Unsupported/malformed/replayed requests rejected; unplugging preserves session operation; suggestions compared with the simpler policy using the same histories. |
| V2 hardware | A measured alternative implementation with preserved data/content contracts. | Portability tests pass; progress imports; new hardware passes electrical, optical, timing and mechanical checks. |

Do not build all these steps before the first physical prototype. The next implementation remains target-build verification and bench bring-up. Introduce each boundary alongside the first feature that needs it; avoid a speculative rewrite.

## When a second hardware version is justified

Choose V2 against a real requirement: smaller assembled footprint, better screen readability, longer validated history, useful local inference, lower noise/power, simpler manufacturing, or component availability. Benchmark candidate hardware on those requirements rather than processor clock alone. A modern MCU and a Linux-capable board have different startup, power, maintenance and audio-timing tradeoffs.

A smaller chip does not necessarily make a smaller pedal: screen, switch spacing, plugs and mechanical loading also set dimensions. A faster chip does not establish trustworthy technique feedback. Match the selected analysis to a labeled audio dataset and instrument conditions before describing it as coaching evidence.

Preserve user progress through an exported schema, original artwork/exercise definitions and the portable core. Expect to replace drivers and possibly the electronics carrier. Consider a custom PCB only after the circuit and desired interactions are validated. Maintain the ability to run the basic buddy without a server or account.

## Sources and review scope

- [1] PJRC, Teensy 4.1 specifications and memory: https://www.pjrc.com/store/teensy41.html
- [2] PJRC, USB serial communication: https://www.pjrc.com/teensy/td_serial.html
- [3] PJRC, EEPROM library and available capacity: https://www.pjrc.com/teensy/td_libs_EEPROM.html
- [4] PJRC, Audio Adapter inputs/outputs and Teensy compatibility: https://www.pjrc.com/store/teensy3_audio.html

The module boundaries, agent action limits, sequence and V2 criteria are project recommendations based on a source review, not vendor-provided capabilities. No firmware, pin assignment, BOM or CAD geometry is changed by this proposal.
