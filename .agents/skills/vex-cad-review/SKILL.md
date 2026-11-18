---
name: vex-cad-review
description: Review or modify PedalPartner VEX CAD, STEP grouping, mechanical fit, CAD-based hero imagery or mentor review packaging. Use for enclosure, mounts, fabrication exports, render provenance or mentor handoff.
---

# VEX CAD review

Work from the repository root. Read `AGENTS.md`, `docs/HANDOFF.md` and `docs/CAD_REVIEW.md`. Classify the requested work before running builds.

## Presentation or packaging only

- Read `docs/MARKETING.md` for hero work. Run `make hero` to tessellate the existing grouped STEP and render the real assembly. Preserve hardware geometry, proportions and the 32.4 mm display. Use existing shared UI pixels. Change lighting, materials, camera or layout only; never generate imaginary product imagery.
- Inspect both JPEGs and `assets/marketing/provenance.json`. Confirm the assembly hash and all 72 components. Keep large render intermediates under ignored `build/hero/`; leave assembly-guide renders unchanged.
- Read `docs/MENTOR_PACK.md` for the lean ZIP. Run `make pack-mentor` and verify its manifest/index. Do not regenerate CAD, firmware or guide PDFs merely to package existing outputs.
- Run `make test` once after source/doc edits. Reuse recorded mechanical checks when geometry is unchanged. Publish only the changed files using `docs/PUBLISHING.md`; stop any method that needs repeated payload chunks.

## Geometry or purchased-interface changes

1. Run `make measure-check` and read `docs/PRINTING.md`. The readout joins `hardware/data/measurements.csv` with `MEASUREMENTS` in `project_data.py`: each row names the `build_cad.py` constant it stands for, the tolerance and the parts it gates, and the table at the end says which STL/DXF files are print-now, ready or waiting. Edit geometry in answer to a flagged row, not ahead of one; never fill unknowns with inferred values. If a new purchased interface needs a measurement, add a `MEASUREMENTS` tuple (and the CSV row) with the parts it gates so `check_project` keeps them consistent.
2. Edit `hardware/cad/build_cad.py`; update `assembly_tree.py` if ownership changes. Keep each physical part in one service group, with wiring reserves separate. Do not rescale purchased parts to fix interference.
3. Run `make cad cad-check` with pinned CAD dependencies, then `make measure-check` again (update the CSV's assumption column when the constant changed; the checker compares them); inspect both reports and exported hierarchy. Check screw engagement, insertion/removal and cable/service-loop space. Distinguish simplified envelopes from manufacturer geometry.
4. Update affected BOM/wiring, guide illustrations and documents. Run `make render`, `make docs` and `make hero` only for the assets affected by the change. Inspect relevant images/PDF pages and run `make test`.
5. Report exact files, commands/results, assumptions and the next physical fit check; update the handoff. An overlap-free STEP does not certify tolerance, strength or safe foot loading. STEP has no native Fusion feature timeline.
