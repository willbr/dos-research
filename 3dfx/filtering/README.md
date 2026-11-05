# Nearest and bilinear filtering

Small C89 / Glide 2.43 comparison lab for the `filtering` entry in
[FEATURES.md](../FEATURES.md). Run `./run.ps1` or `run.cmd`; rebuild with
`./build.ps1`. Opens with an animated, magnified side-by-side comparison.

The left and right panels use identical texture data, geometry, perspective
weights and UV motion. Only `grTexFilterMode` changes. The top panels show a
one-texel black/white checkerboard; the bottom panels magnify the existing
procedural cathedral stone texture. Swapping sides checks for viewport bias.

| Key | Action |
| --- | --- |
| C | Side-by-side / single view |
| B | Toggle nearest / bilinear in single view; leaves split view |
| S | Swap the two sides |
| V | Magnify (32 texels) / minify (1024 texels) |
| + / - | More / fewer sampled texels, from 8 to 2048 |
| O | Front-facing / oblique perspective surface |
| Space | Pause / resume slow UV motion |
| Arrows | Pan the sampled region one texel |
| R | Reset the comparison |
| Tab | Profiler with FPS and frame-time graphs |
| Esc | Exit |

Nearest selects a single texel and preserves hard pixel boundaries. Bilinear
weights four neighboring texels, smoothing magnified edges. Minification mode
shows the remaining shimmer with each filter. This lab deliberately disables
mipmapping: bilinear filtering alone does not solve distant texture aliasing.
The separate [mipmap demo](../mipmaps/README.md) covers that mechanism.

The independent minification and magnification arguments are documented in
the [Glide Reference](../Glide_Reference_Manual_2.4_199707.pdf), printed p. 96;
the [Programming Guide](../Glide_Programming_Guide_2.4_199707.pdf), printed
pp. 83-84, describes nearest and four-texel bilinear interpolation.

## Validation

`./verify.ps1` runs the native OpenGL and software Voodoo configurations.
The strict `/test` path checks:

- A 2x2 red/green/blue/white fixture: nearest returns one original texel;
  bilinear at its midpoint returns approximately gray (128,128,128).
- Independent filter arguments: changing only minification affects the
  minified fixture; changing only magnification affects the magnified one.
- Identical scene pixels after swapping nearest/bilinear between viewports.
- Repeatable paused frames, visible UV motion and magnified/minified/oblique
  native framebuffer captures.

DOSBox-X 2026.01.02 OpenGL passes every check. Software Voodoo passes the
color, ordinary nearest/bilinear, swap and motion checks, but its single-level
minification fixture responds to the magnification argument. `/softwarecheck`
reports that exact observed pattern as **LIMIT**; unexpected failures still
fail verification. `/test` remains strict on either backend. This is an
emulator observation, not a claim about original hardware. Native screenshots
are preserved; software diagnostics do not overwrite them. Captures/logs are
ignored. Leave the roadmap item unchecked until its visual comparison is reviewed.

`./bench.ps1` measures 600 moving frames across eight cases: split/single,
nearest/bilinear, magnified/minified and oblique, with the profiler shown for
the second half. On this machine: **59.86 Windows-clock FPS**, maximum guest
frame **17.202 ms**, **zero frames over 18 ms**. The on-screen FPS counter
counts completed refresh-paced frames against RTC seconds.

Target: 640x480 at 60 Hz, native emulated Voodoo Graphics, 2 MB framebuffer
and one 2 MB TMU, `voodoo_maxmem=false`, `glide=false`. Detected configuration
and allocation are logged. Total texture allocation is **401,416 bytes**.
The launch configs mount this folder as C: and the existing procedural stone
asset directory as D:, avoiding a duplicate texture copy and DOS long-name issues.
