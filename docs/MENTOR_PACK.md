# Lean mentor review pack

Run `make pack-mentor` from the repository root. Python's standard library creates
`dist/VEX_R1.1_Mentor_Review.zip` from current checked-in outputs. No dependencies,
compilation, CAD rebuild or image generation are required. The command fails if
required files are missing, or the hero's assembly/image hashes are stale.

Unzip once and open **VEX_R1.1_Mentor_Review/START_HERE.md**. Keep the folders
together. The guide PDF is the main reading route; the STEP is the CAD review route.
The pack is for mentor review and fit planning, not a complete developer checkout
or permission to fabricate before measurement.

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

The ZIP omits flat and subassembly duplicates, vendor source CAD, historical/concept
images, firmware/toolchains, Blender intermediates and private local files. Full
editable sources and software remain in the GitHub repository. The two PDFs remain
the reviewed R1.1 hardware guides; this presentation change does not add future
software features to the hardware instructions.

For planning, give the mentor these exact intended parts: **Teensy 4.1, Audio
Adapter Rev D/D2, Adafruit 6178, two SparkFun COM-11992 buttons, P-H605-BLK
footswitch and Switchcraft 111X jack**. Those identities let them check dimensions
and mounting requirements before choosing raw stock. Bring the actual parts for
fit decisions; simplified connector/switch models are not measurements.

The packer uses a fixed file allowlist, fails on symlinks/external paths, records
every included payload's size and SHA-256, uses fixed ZIP timestamps and writes
atomically. Identical inputs produce identical archives. Outputs belong in
ignored `dist/`; share the ZIP directly, without printing or encoding it in chat.
