# Review packs: hobbyist / mentor and design studio

Two ZIPs are built from the same checked-in outputs by Python's standard library.
No dependencies, compilation, CAD rebuild or image generation are required. Each
command fails if a required file is missing or the hero's assembly/image hashes are stale.

| Reader | Command | Output | Adds |
|---|---|---|---|
| Hobbyist alone, or hobbyist with a mentor | `make pack-mentor` | `dist/VEX_R1.1_Mentor_Review.zip` | The review set below |
| Design studio or product team | `make pack-studio` | `dist/VEX_R1.1_Studio_Pack.zip` | The same set plus [the studio brief](STUDIO_BRIEF.md), [CAD review map](CAD_REVIEW.md), [decisions](DECISIONS.md), [extensibility](EXTENSIBILITY.md), [bring-up plan](BRINGUP.md), [handoff](HANDOFF.md), [editable guide](BUILD_GUIDE.md), [local dev](LOCAL_DEVELOPMENT.md), [milestones](PLAN.md), [sources](SOURCES.md), `parameters.json`, `config.h`, the motion video and storyboard, and the screen contact sheet |

Unzip once and open **START_HERE.md** inside the top folder. Keep the folders
together. For a builder the guide PDF is the main reading route and the STEP the CAD
review route; for a studio the brief comes first, then the renders, motion and STEP.
Neither pack is a complete developer checkout or permission to fabricate before
measurement. Solo builders should also read [SOLO_BUILD.md](SOLO_BUILD.md), which is
in the repository rather than the ZIP.

| Included | Purpose |
|---|---|
| Illustrated guide and 1:1 template PDFs | Assembly, wiring, bench sequence and footprints |
| Current shopping list and all data CSVs | Exact component identities, wiring, fasteners and blank measurement/acceptance sheets |
| One grouped STEP, assembly/part manifests and recorded checks | Review all 72 parts in five service groups |
| Separate wiring reserve STEP | Cable and service-space review |
| Ten selected print-candidate STLs, printing instructions and two DXF profiles | Fit coupon and fabrication references after measured approval; cut the window, metal plate and perfboard from specified stock |
| CAD hero/share images with provenance | Show the proposed finished appearance |
| License and third-party provenance/notices | Retain attribution and unresolved source-license limits |
| START_HERE.md and SHA-256 manifest | Navigation and exact included-byte identity |

Both ZIPs omit flat and subassembly duplicates, vendor source CAD, historical/concept
images, firmware/toolchains, Blender intermediates and private local files. Full
editable sources and software remain in the GitHub repository. The two PDFs remain
the reviewed R1.1 hardware guides; this presentation change does not add future
software features to the hardware instructions.

For planning, give whoever reviews the fit these exact intended parts: **Teensy 4.1, Audio
Adapter Rev D/D2, Adafruit 6178, two SparkFun COM-11992 buttons, P-H605-BLK
footswitch and Switchcraft 111X jack**. Those identities let them check dimensions
and mounting requirements before choosing raw stock. Bring the actual parts for
fit decisions; simplified connector/switch models are not measurements.

The packer uses a fixed file allowlist, fails on symlinks/external paths, records
every included payload's size and SHA-256, uses fixed ZIP timestamps and writes
atomically. Identical inputs produce identical archives. Outputs belong in
ignored `dist/`; share the ZIP directly, without printing or encoding it in chat.
