# Fixed-function colour and alpha combiners

C89 / Glide 2.43 lab for the `combiners` entry in [FEATURES.md](../FEATURES.md).
Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`.
Opens with four copies of one rotating textured orb, labeled with their RGB equations.
Geometry, texture coordinates and CPU lighting are prepared once and shared.

| Key | Action |
| --- | --- |
| 1-4 | Select replace/modulate/add/interpolate in single view |
| C | Four-mode gallery / single view |
| A | Cycle the five output-alpha equations |
| B | Enable/disable source-alpha framebuffer blending |
| + / - | Constant alpha CA, from zero to 255 |
| J / K | Less / more CPU lighting L |
| U / I | Less / more vertex alpha VA |
| N | Constant RGB tint: blue, copper or green |
| T | Switch the two original procedural RGBA textures |
| Space | Pause/resume object and light motion |
| R | Reset all controls |
| Tab | Frame profiler with FPS and frame-time graphs |
| Esc | Exit |

## Equations

The following equations use normalized 0..1 channels. T is texture RGB,
L is CPU-supplied vertex RGB, C is constant RGB, TA is texture alpha,
CA is constant alpha and VA is vertex alpha. Addition clamps to 0..1.

| RGB mode | Equation | What to look for |
| --- | --- | --- |
| Replace | T | Original texture colors, independent of lighting |
| Modulate | T * L | Lighting darkens and tints the texture |
| Add | clamp(T + L) | Brightening, with saturated highlights |
| Interpolate | CA * T + (1-CA) * C | CA moves between constant tint and texture |

| Alpha mode | Equation |
| --- | --- |
| Opaque | 1, supplied through vertex alpha |
| Replace | TA |
| Modulate | TA * CA |
| Add | clamp(TA + CA) |
| Interpolate | TA * VA + (1-TA) * CA |

B is deliberately separate from A. With blending off, generated alpha does
not make the object transparent. With blending on, the framebuffer receives
sourceRGB * sourceAlpha + backgroundRGB * (1-sourceAlpha). The patterned
backdrop makes those changes visible. VA controls non-opaque modes; opaque
mode forces vertex alpha to one. In particular, replace/add/modulate may
ignore a control that is absent from their displayed equation.

## Native implementation

`grColorCombine` configures texture replacement, local-RGB multiplication,
texture-plus-local addition, and `f*(other-local)+local` interpolation.
`grAlphaCombine` independently generates source alpha. The manual specifies
that its local/other alpha sources are also read by the colour combiner's
alpha factors. Every alpha mode therefore keeps **Alocal = constant alpha**,
so RGB interpolation consistently uses CA even as the output-alpha equation
changes. The constant RGB tint and constant alpha share `grConstantColorValue`.

See the [Glide Reference](../Glide_Reference_Manual_2.4_199707.pdf), printed
pp. 13-15 (`grAlphaCombine`) and 25-27 (`grColorCombine`). These are predefined
fixed-function operations, not shader programs or multipass approximations.

The textures are ARGB4444. The orb uses a small latitude/longitude mesh with
front-facing cells only, one textured draw per visible cell per view, Z testing
and opaque depth writes. Blending demonstrates the front shell over a backdrop;
it does not render a physically transparent solid with a second back shell.
Mipmap filtering and fog are disabled. CPU vertex lighting is a simple diffuse
input to the equations; the additive mode is not a complete specular model.
Backdrop writes are explicitly disabled before disabling depth testing, so
one gallery object's depth cannot contaminate the next panel.

## Validation

`./verify.ps1` runs native OpenGL and software Voodoo checks. Both pass:

- 12 RGB probes against calculated replacement, multiplication, saturated
  addition and interpolation values, including zero/half/full CA.
- 45 alpha probes through ordinary source-alpha blending, including zero/full
  texture alpha, CA endpoints, saturation and interpolated vertex alpha.
- Five shared-source checks: RGB interpolation remains identical across all
  output-alpha modes when framebuffer blending is disabled.
- Four gallery panels match freshly cleared, same-viewport reference draws
  exactly, preventing stale backdrop depth from silently clipping objects.
- Identical geometry counts across RGB modes, visible alpha blending,
  restored state, repeatable paused frames and moving-scene differences.

The 62 numeric probes allow small 16-bit output/fixed-point rounding errors.
Native screenshots and runtime logs are ignored; software checks do not
overwrite native captures. The roadmap remains unchecked pending visual review.

`./bench.ps1` measures 600 moving frames across gallery/single views, RGB and
alpha equations, blending, constants, tints and both textures, with the profiler
shown for the second half. DOSBox-X 2026.01.02 on this machine measures
**59.85 Windows-clock FPS**, maximum guest frame **17.232 ms**, **zero frames
over 18 ms**, and at most **164 object-cell draws** across the four views.

Target: native emulated Voodoo Graphics, 640x480 at 60 Hz, 2 MB framebuffer
and one 2 MB TMU, `voodoo_maxmem=false`, `glide=false`. Texture allocation is
**206,856 bytes**, including test, backdrop, font and profiler data. FPS counts
completed refresh-paced frames against RTC seconds. All assets are generated
in C; no user photographs or external textures are saved.
