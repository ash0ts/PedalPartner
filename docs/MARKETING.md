# CAD-based presentation images

The README hero and share image are conventional Blender renders of the **actual
R1.1 grouped STEP assembly**. No generative image model is involved. The enclosure,
controls, bezel, window and real 32.4 mm display keep their CAD dimensions and
locations. Studio lighting, proposed finishes, existing decal labels and graphic
layout are presentation choices. These are not photographs of a built product.

| Asset | Use |
|---|---|
| [vex-hero.jpg](../assets/marketing/vex-hero.jpg) | README and wide presentation; 1920 × 1080 |
| [vex-social.jpg](../assets/marketing/vex-social.jpg) | Sharing; 1200 × 630 |
| [provenance.json](../assets/marketing/provenance.json) | Source STEP/screen hashes, component count, renderer, scripts and output hashes |

## Reproduce

Use the existing pinned CAD environment, Blender 4.3+ and Pillow, then run:

```sh
make hero > build-hero.log 2>&1
```

This command imports the existing STEP and tessellates its 72 components; it does
not run the parametric CAD build, collision sweep, guide generator or firmware
compiler. Bounds must match the recorded CAD report. It then loads those exact
meshes in Blender, renders with Cycles on eight CPU threads, and composes type
with the repository's DejaVu fonts. Noise is sampled directly; OpenImageDenoise
is not required. Expect small pixel differences between Blender/platform versions.

`hardware/cad/export_render_meshes.py` writes ignored meshes and source hashes.
`hardware/cad/render_hero.py` checks those hashes and writes the studio scene,
transparent product PNG and render record to ignored `build/hero/`.
`tools/compose_hero.py` writes the two JPEGs and provenance. Existing assembly-guide
illustrations and CAD exports are untouched. Keep only the finished small JPEGs,
provenance and scripts in Git; the scene and full-size intermediate stay local.

The screen texture is `assets/screen.png`, produced by the shared C++ UI through
`tools/render_screen.py`. Reuse it unless the UI changes. If it does, first build
the host simulator and regenerate the texture, then render again. A styled image
does not validate screen glare, standing readability, materials or foot strength.

For layout-only changes, reuse `build/hero/product.png` and run
`python3 tools/compose_hero.py`. After changing CAD, regenerate/validate the CAD
first, then run `make hero` to refresh the image's geometry and provenance.
