# Z and W depth buffers

C89 / Glide 2.43 control lab for the `depth` entry in [FEATURES.md](../FEATURES.md).
Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`.
Opens with rotating intersecting panels, Z on the left and W on the right.

| Key | Action |
| --- | --- |
| C | Split Z/W comparison / single view |
| B | Select Z/W in single view; leaves split mode |
| H | Depth testing on/off |
| W | Depth writes on/off |
| G | Cycle AUTO and all eight comparison functions |
| D | Reverse submission order |
| P | Intersections / precision fixture |
| N | Broad / fitted Z mapping |
| + / - | Double / halve precision distance, from 16 to 32768 |
| Space | Pause / resume panel motion |
| R | Reset all controls |
| Tab | Profiler with FPS and frame-time graphs |
| Esc | Exit |

With testing and writes enabled, reversing order preserves the visible
intersections apart from ties at exactly equal encoded depths. Disabling
testing makes the last panel win. Disabling writes still performs comparisons
against existing depth, but the panels no longer establish their own occlusion.

AUTO uses GREATER for our Z mapping and LESS for W. G selects the literal
Glide comparison instead; selecting the opposite inequality can retain farther
surfaces. NEVER rejects everything. EQUAL also produces a blank normal scene
because no panel matches the initial clear depth. The pixel controls test
EQUAL and every other function against a previously drawn reference surface.
The clear value follows LESS/LEQUAL versus GREATER/GEQUAL; for the other
functions it uses the mode's normal far value.

## Precision fixture

Press P. Orange is the nearer inset; cyan is the farther panel. Each row shows
the same pair in both submission orders. At distance 4096, the four separations
are approximately 0.410, 2.048, 4.096 and 40.960 world units.

With the broad Z range of 1..65535, all four pairs tie in the tested emulators:
far-first leaves cyan in the middle. W preserves orange in all four cases.
Press N to fit the Z range to 95%-105% of the selected distance: Z then
resolves all four pairs too. This illustrates depth encoding and mapping,
not a claim that W is universally better or that emulator quantization exactly
matches original hardware. Depth-code ties also depend on the chosen comparison.

## Native implementation

`grDepthBufferMode` selects 16-bit fixed-point Z or the native floating W
representation. Both modes reuse the same auxiliary buffer; the split clears
and renders each viewport independently. No second depth allocation is needed.

Z receives `GrVertex.ooz = 65535 * (1/d - 1/far) / (1/near - 1/far)`;
the larger value is nearer. W receives `GrVertex.oow = 1/d`; the hardware
recovers its W representation for depth comparisons, where smaller is nearer.
Both quantities are supplied for the same geometry. `grDepthBufferFunction`
and `grDepthMask` control comparisons and writes independently. Every viewport
clear temporarily enables depth writes, including after the W key disables them,
so previous frames cannot contaminate the comparison. Depth bias remains zero.

See the [Programming Guide](../Glide_Programming_Guide_2.4_199707.pdf), printed
pp. 59-60, and the [API Reference](../Glide_Reference_Manual_2.4_199707.pdf),
printed pp. 32-35. The scene uses opaque vertex colors to keep texture and
alpha behavior out of the depth controls.

## Validation

`./verify.ps1` runs native OpenGL and software Voodoo checks:

- 48 color probes: both buffer modes, all eight comparisons, and incoming
  nearer/equal/farther depths against an existing reference surface.
- Depth testing disabled, writes independently off/on, and a fresh clear
  after depth writes were disabled.
- Reversed-order intersections with testing off/on, repeatable fixed frames,
  broad/fitted precision captures and fitted-Z separation checks.

Both backends pass on DOSBox-X 2026.01.02. Depth-off changes about 30,000
scene pixels when order reverses. Depth-on changes zero in OpenGL and seven
in software, with up to 64 allowed for ownership at depth ties along the
intersection. Precision results are logged as observations. Native captures
and runtime logs are ignored; software checks do not overwrite native images.

`./bench.ps1` measures 600 moving frames across Z/W, split/single,
test/write/comparison settings and both precision mappings, with the profiler
shown for the second half: **59.84 Windows-clock FPS**, maximum guest frame
**17.203 ms**, **zero frames over 18 ms** on this machine.

Target: native emulated Voodoo Graphics, 640x480 at 60 Hz, 2 MB framebuffer
and one 2 MB TMU, `voodoo_maxmem=false`, `glide=false`. Texture use is
**139,264 bytes**, for the font and profiler only. FPS counts completed
refresh-paced frames against RTC seconds. Visual review remains pending.
