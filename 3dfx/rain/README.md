# Winchester wind and rain

C89 / Glide 2.43 weather demo extending [water](../water/README.md).
Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`.
Starts beside the puddle in FPS walk with medium rain.

| Key | Action |
| --- | --- |
| J / K | Less / more rain: off, light (32 drops), medium (64), heavy (96) |
| M / Z | Send a wind gust / toggle wind |
| V / + / - | Still/rippled water / ripple strength |
| , / . | Fewer / more leaves, up to 128 |
| R / P | Reflection / lantern |
| U / I / N | FPS walk / FPS fly / manual drag view |
| WASD / mouse / Shift | Move / direct look / faster movement |
| Q / E | Descend / ascend in flight |
| F / Tab / Esc | Automatic tour / profiler and history graphs / exit |

Manual view pauses the simulation; Space resumes it. FPS walk/fly stay animated.
The previous water demo remains available independently.

## Rendering and simulation

Rain falls in world space, with gusts pushing it along the same +X/-Z direction
as leaves and water flow. The bounded 96-drop pool follows the camera outdoors;
a third covers the small puddle area so its interaction stays visible. Ground
and approximate grave support heights stop drops, while exposed water hits
allocate from a fixed 16-impact ring pool. Closely spaced contacts coalesce
to bound sprite work. No particles spawn inside the nave.

Each impact briefly throws up two small splashes and expands a fading ring.
One original procedural 32x32 ALPHA8 ring sprite is uploaded once and shared
by every impact. Ring polygons clip against the puddle and the view frustum;
foreground geometry and planted cutouts occlude weather through the Z buffer.
Weather uses alpha blending with depth writes disabled. One-pixel streaks and
small splash quads share an untextured state; rings share one texture state.
Distant streaks, rings and splashes fade to reduce drawing work. No per-frame
texture uploads, framebuffer readback, extra TMU or expanded Voodoo memory.

Rain uses a fixed 60 Hz simulation step with a bounded accumulator and its own
deterministic random stream. Intensity controls the number of active drops and
adds modest surface roughness; gusts strengthen water flow and rain slant.
Switching rain off hides weather immediately and lets stored impacts expire.
The existing water texture, planar reflections, wind and floating leaves remain.

This is a small weather effect, with approximate ground/grave contacts rather
than general mesh collision. Tree canopies do not intercept drops. Water remains
a flat plane: the ring sprites do not deform or refract reflected geometry.

## Validation

`./verify.ps1` checks native OpenGL and software Voodoo color/depth controls.
Native checks cover intensity limits, gust direction, pause, dry-ground impact
rejection, bounded impact lifetimes, visible weather and repeatable paused frames.
Native captures and runtime logs are ignored.

`./soak.ps1 -Stress` runs 600 moving close-puddle frames with heavy rain and
128 leaves, showing the profiler for the second half. On this machine with
DOSBox-X 2026.01.02: **59.80 Windows FPS**, maximum guest frame **17.198 ms**,
**zero frames over 18 ms**. The 600-frame `-Doorway` and `-ReturnPath` checks
also pass: **59.72 / 59.74 Windows FPS**, maxima **17.201 / 17.202 ms**, with
zero frames over 18 ms. The original water demo's native/software checks pass.
`-FullTour` is available for the complete 6,000-frame route. Weather drawing is
included in the profiler's Blending row; simulation is included in Backdrop.

Target: native emulated Voodoo1, 640x480 at 60 Hz, 2 MB framebuffer and one
2 MB TMU, `voodoo_maxmem=false`, `glide=false`. Total TMU use is **1,633,280 bytes**,
including the extra **1,024-byte** ring sprite. The performance claim covers the
tested paths with default alpha-tested foliage and optional stone detail off.
