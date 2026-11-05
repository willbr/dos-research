# 3dfx demos

Each demo lives in its own subfolder.

See the [feature checklist and demo roadmap](FEATURES.md) for capabilities,
limitations, and the next experiments to build.
See [project notes](notes.md) for proposed sectorforth and sectorlisp ports
with interactive Glide graphics.

The nave/graveyard demos share **U** FPS walking, **I** noclip flying,
**N** drag-based manual look and **F** automatic tour. FPS modes use held
**WASD**, mouse look and **Shift** for speed; flying adds **Q/E** for
down/up. See [camera controls](fog/README.md) for collisions and feature keys.

- [cube](cube/README.md): rotating rainbow cube using DOS Glide, with a live
  FPS counter and 60 Hz frame pacing. Launch with `cube/run.cmd` or
  `./cube/run.ps1`.
- [helmet](helmet/README.md): textured C diving helmet inspired by the user's
  Winchester photograph, with silhouette-edge anti-aliasing, inertial mouse
  controls, and a Tab profiler with FPS/frame-time graphs.
  Launch with `helmet/run.cmd` or `./helmet/run.ps1`.
- [envmap](envmap/README.md): the diving helmet with tinted environment
  reflections on copper, brass and steel. **E** toggles them, **+/-** adjusts
  strength. Launch with `envmap/run.cmd` or `./envmap/run.ps1`.
- [textures](textures/README.md): Winchester Cathedral exterior with smooth
  orbit controls, an automatic drone flight, perspective/affine texture
  comparison and the Tab profiler. Launch with `textures/run.cmd` or
  `./textures/run.ps1`.
- [filtering](filtering/README.md): isolated nearest/bilinear checkerboard and
  stone comparisons, with split/single views, magnification/minification and
  moving oblique surfaces. **C** split, **B** filter, **V** mag/min, **Space** pause.
  Launch with `filtering/run.cmd` or `./filtering/run.ps1`.
- [self-lit-city](self-lit-city/README.md): buildings with windows and neon signs
  that stay bright while CPU lighting darkens the walls, using the native alpha
  lighting selector in one pass. **E** mask, **C** split, **M** mask preview.
  Launch with `self-lit-city/run.cmd` or `./self-lit-city/run.ps1`.
- [mipmaps](mipmaps/README.md): cathedral mipmapping comparison, colored mip
  levels, LOD bias and dithered transitions. Uses the software Voodoo backend
  to honor uploaded levels; see its measured host FPS and backend notes.
  Launch with `mipmaps/run.cmd` or `./mipmaps/run.ps1`.
- [blending](blending/README.md): candlelit nave with stained glass, additive
  flames/halos and colored floor patterns, translucent mist, and a sorting
  comparison. Launch with `blending/run.cmd` or `./blending/run.ps1`.
- [depth](depth/README.md): intersecting panels and precision fixtures comparing
  native Z/W buffers, depth comparisons and writes. **H** test, **W** writes,
  **D** order, **P** precision, **N** fitted Z range.
  Launch with `depth/run.cmd` or `./depth/run.ps1`.
- [combiners](combiners/README.md): one textured orb in replace, modulate, add
  and interpolate modes, with labeled RGB/alpha equations and pixel controls.
  **1-4** mode, **C** gallery, **A** alpha equation, **B** source-alpha blending.
  Launch with `combiners/run.cmd` or `./combiners/run.ps1`.
- [specular-paint](specular-paint/README.md): moving highlights on curved metal,
  masked away from chipped blue paint and lettering by the native alpha selector
  in one pass. **E** paint mask, **G** specular, **C** split, **N** gloss.
  Launch with `specular-paint/run.cmd` or `./specular-paint/run.ps1`.
- [texture-compression](texture-compression/README.md): TexUS-generated RGB565,
  indexed P8 and YIQ/NCC image gallery with memory/error comparisons and resident
  NCC-slot controls. **E** errors, **Z** zoom, **V** NCC slots, **T** image.
  Uses software Voodoo for correct NCC switching; OpenGL diagnostics are included.
  Launch with `texture-compression/run.cmd` or `./texture-compression/run.ps1`.
- [fog](fog/README.md): continuous journey from the nave through its doorway
  into a graveyard with headstones and trees; native linear, exponential and
  exponential-squared distance fog. Launch with `fog/run.cmd` or `./fog/run.ps1`.
- [lightmap](lightmap/README.md): the same journey with independently mapped
  light textures, candle pools, stained-glass floor patterns and moonlit
  graveyard shadows. **B** compares lightmaps with base textures alone.
  Launch with `lightmap/run.cmd` or `./lightmap/run.ps1`.
- [project](project/README.md): moving projected lantern over the lightmapped
  nave and graveyard. **P** toggles it, **O** holds it in the world, and **X**
  shows the projection grid. Launch with `project/run.cmd` or `./project/run.ps1`.
- [shadows](shadows/README.md): moving lantern ground shadows from pillars,
  headstones and crosses, preserving baked lighting. **S** toggles shadows.
  Launch with `shadows/run.cmd` or `./shadows/run.ps1`.
- [reflect](reflect/README.md): a still-water graveyard puddle with mirrored
  geometry, native depth occlusion and alpha water tint. **R** compares the
  reflection with plain water. Launch with `reflect/run.cmd` or `./reflect/run.ps1`.
- [detail](detail/README.md): nearby stone grain over the lightmapped nave and
  graveyard, with fog-correct compositing and distance fade. Opens on a paused
  close-up with a labeled split; **D** toggles detail and **V** toggles the split.
  **+/-** changes strength and **J/K** tiling. Launch with `detail/run.cmd`
  or `./detail/run.ps1`.
- [foliage](foliage/README.md): alpha-tested tree canopies, grass and ivy,
  traveling gusts, and loose leaves that flutter, settle and lift again.
  **,/.** adjusts the leaf count from zero to 128; **M** sends a gust.
  Launch with `foliage/run.cmd` or `./foliage/run.ps1`.
- [water](water/README.md): two scrolling ripple layers over the graveyard
  reflection, responding to gusts; loose leaves float, bob, drift and strand
  on the bank. **V** compares still/rippled water, **+/-** changes strength.
  Launch with `water/run.cmd` or `./water/run.ps1`.
- [rain](rain/README.md): wind-driven rain over the graveyard puddle, with
  expanding impact rings and brief splashes. **J/K** adjusts rain from off to
  heavy; **M** sends a gust. Launch with `rain/run.cmd` or `./rain/run.ps1`.
- [sectorforth](sectorforth/README.md): 32-bit assembly Forth port with Glide
  words and an RGB triangle defined in Forth. Launch with `sectorforth/run.cmd`
  or `./sectorforth/run.ps1`.
- [sectorlisp](sectorlisp/README.md): DOS port of sectorlisp's C reference
  evaluator with native Glide functions and an RGB triangle defined in Lisp.
  Launch with `sectorlisp/run.cmd` or `./sectorlisp/run.ps1`.

## Manuals

- [Glide 2.4 API Reference](Glide_Reference_Manual_2.4_199707.pdf) (137 pages).
- [Glide 2.4 Programming Guide](Glide_Programming_Guide_2.4_199707.pdf) (172 pages).

Original 3dfx manuals, downloaded from the
[Bitsavers mirror](https://bitsavers.trailing-edge.com/components/3dfx/).

## Tools

The complete Glide SDK and DOS runtime files are vendored in
[`vendor/`](vendor/README.md) and included in Git for reuse across demos.
Open Watcom is downloaded separately into `cube/tools/watcom` by
`cube/setup.ps1`. Build scripts copy the vendored runtimes beside each demo.
