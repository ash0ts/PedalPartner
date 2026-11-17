# Third-party material and attribution

The existing root MIT license covers original PedalPartner code, documentation, VEX pixel art and enclosure construction. It does not replace licenses or permissions for imported material.

- **Adafruit display PCB/CAD:** attribution and source hashes in [PROVENANCE.md](../hardware/vendor/PROVENANCE.md); PCB license in [ADAFRUIT_LICENSE.txt](../hardware/vendor/ADAFRUIT_LICENSE.txt). Retain Adafruit attribution and applicable source terms when redistributing derived board geometry.
- **Teensy and Audio Adapter community STEP references:** carried forward from the user's supplied CAD pack, with original repository URLs/hashes in PROVENANCE.md. No blanket MIT relicensing is asserted. A license file was not identified at either upstream repository root during preparation; downstream redistribution terms remain to be resolved. These are fit references, not manufacturer-certified models.
- **DejaVu fonts:** [font license](../assets/fonts/LICENSE.txt). The generated tiny bitmap font is derived from the bundled DejaVu Sans font; retain this notice/license with it.
- **Libraries:** Arduino/Teensy and Adafruit dependencies are installed by the build, not vendored as project code. Their upstream terms apply.
- **Appearance studies:** original generated concept images in `docs/reference/`, retained to explain design intent. They are not manufacturing drawings. VEX artwork is original; this is not an official Tamagotchi or Mega Man product.

Where a neutral assembly includes imported purchased-component geometry, those subcomponents retain the provenance above. Custom enclosure solids remain separately exportable for review and fabrication.
