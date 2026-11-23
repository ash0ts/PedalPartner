# Validation evidence

`software_validation.json` records initial R1.1 starter checks. Re-run `make test` for the current source (core unit tests, 14 simulator scenarios + 45 goldens in native and WebAssembly when `em++` + `node` are present, `make eval` on `test_audio/drills/`, art, pack, link, bench-report, measurement-gate and project consistency checks). Target compilation writes `build/target_build_report.json` after `make firmware-all` (and runs in the manual Firmware workflow). CAD reports live in `hardware/generated/checks/`. No physical result is inferred from these files.
