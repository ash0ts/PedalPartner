---
name: vex-cad-review
description: Review or modify PedalPartner VEX CAD, STEP grouping, printed parts and mechanical fit. Use for enclosure, bezel, board mounts, connectors, fabrication exports or mentor CAD review.
---

# VEX CAD review

Work from the repository root. Read `AGENTS.md`, `docs/HANDOFF.md` and `docs/CAD_REVIEW.md`. Read `docs/PRINTING.md` for fabrication work; read the relevant rows of `hardware/data/project_data.py` and `hardware/data/measurements.csv` for purchased interfaces.

1. Identify the intended change and which mating parts it affects. Preserve the actual display size and USB/buffered-input architecture unless the user requests a new revision.
2. Edit `hardware/cad/build_cad.py`. Update `assembly_tree.py` if ownership changes. Keep each physical part in exactly one service group and keep wiring reserves separate.
3. Run `make cad cad-check` with the pinned CAD requirements. Inspect both reports under `hardware/generated/checks/` and the exported hierarchy, not only the render.
4. Inspect sections, screw engagement, component insertion/removal and cable/service-loop space. Distinguish simplified envelopes from manufacturer data. Record assumptions; never fabricate measured values.
5. Update BOM/wiring if affected. Run `make render` with Blender and `make docs`; visually inspect the current assembly and relevant PDF pages. Run `make test` for cross-file consistency.
6. Report exact files changed, geometry results, what still needs measurement and the next physical fit check. Update the handoff.

For appearance-only work, prefer practical finish/insert changes. Render materials live separately in `hardware/cad/render_cad.py`; keep them consistent with CAD colors and the finish specification. STEP is a neutral solid assembly, not a native Fusion feature timeline. Do not rescale purchased parts to fix an interference. An overlap-free assembly is not proof of tolerance, strength or safe foot loading.
