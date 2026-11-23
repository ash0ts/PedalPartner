# Primary sources and how they shaped the repository

Checked 16 November 2026. Versions are pinned to make the starter reproducible, not to imply perpetual freshness.

| Source | Applied decision |
|---|---|
| [AGENTS.md](https://agents.md/) | Put commands, invariants and source map in a predictable short root file; keep human onboarding in README. |
| [Agent Skills specification](https://agentskills.io/specification) | Named directories with YAML name/description; short procedures and references read on demand. Three focused repo skills (`vex-cad-review`, `vex-firmware-change`, `vex-art-pipeline`) instead of a large context dump. |
| [Pi official skills documentation](https://github.com/earendil-works/pi/blob/main/packages/coding-agent/docs/skills.md) | Use portable `.agents/skills/` discovery and `/skill:name`; project trust applies. The former badlogic/pi-mono URL redirects to this upstream. |
| [Arduino CLI compile](https://arduino.github.io/arduino-cli/1.6/commands/arduino-cli_compile/) | Explicit compile commands, per-stage build/output directories; upload is not part of the build target. |
| [PJRC package index](https://www.pjrc.com/teensy/package_teensy_index.json) | Pin Teensy core and use the official package source. |
| [PJRC installation](https://www.pjrc.com/teensy/td_download.html) | IDE bring-up route and board-package setup. |
| [Adafruit GC9A01A](https://github.com/adafruit/Adafruit_GC9A01A), [GFX](https://github.com/adafruit/Adafruit-GFX-Library), [BusIO](https://github.com/adafruit/Adafruit_BusIO) | Pin the three target display libraries in one build script. |
| [build123d assembly documentation](https://build123d.readthedocs.io/en/latest/assemblies.html) | Nested labeled compounds plus neutral STEP; verify exported hierarchy by re-import. |

The handoff format is a project choice informed by these sources: current state, evidence, unknowns, exact next actions and a small source map. No source is claimed to guarantee agent performance. The repo is sufficient to resume without a proprietary memory service; the three skills are reusable procedures, not hidden state.

Hardware product/datasheet links are maintained in [SOURCES.md](SOURCES.md) and the generated [shopping list](SHOPPING.md). Manufacturer data and community CAD are distinguished in [provenance](../hardware/vendor/PROVENANCE.md). Never infer measured fit from a product rendering.
