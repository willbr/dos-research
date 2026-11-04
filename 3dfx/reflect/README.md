# Winchester still-water reflections

A graveyard puddle reflects nearby headstones, crosses, trees and the west
doorway. The existing nave/graveyard journey, baked lightmaps, fog, projected
lantern and moving ground shadows carry over.

Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`.
The demo uses native C89 / DOS / Glide 2.43 and the standard Voodoo 1 settings:
2 MB framebuffer, one 2 MB TMU, `voodoo_maxmem=false`, 640x480 at 60 Hz,
fixed 100000 CPU cycles/ms. No new texture allocation is needed; the complete
scene still uses 1,220,608 bytes of TMU memory.

| Key | Action |
| --- | --- |
| R | Reflected scene / still water with no reflected objects |
| S | Moving lantern ground shadows |
| P / O / X | Lantern toggle / hold in world / projection grid |
| B / H | Baked lightmaps / distance fog |
| F | Automatic journey / manual look |
| Space | Pause journey and lantern sweep |
| Left drag / arrows | Smooth manual look |
| Wheel / right drag | Local dolly |
| Tab | Profiler, including REFLECT and FPS/frame-time graphs |
| Esc | Exit |

The puddle lies left of the path, centered near `(-5.8, -28)` in scene X/Z.
The flight looks down toward it on both the outward and returning journey;
camera positions and doorway clearance are unchanged. Interactive launches
start at the first puddle view, then continue around the full loop. Benchmarks
still begin at the original nave starting point and cover the entire route.

## Reflection pass

The CPU mirrors geometry about the horizontal water plane at Y = -0.020.
It selects a bounded, coarse reflection scene from the existing mesh, culls
against the reflected camera, and clips polygons against the view frustum,
water plane and the eight planes through the camera and puddle edges.
Back-to-front polygon ordering resolves overlapping reflected surfaces.
The main scene's room apertures reject puddles hidden behind the nave walls.

Texture interpolation and native distance fog use the mirrored geometry's
camera reciprocal depth. Raster Z instead follows the actual water plane at
each screen vertex. The normal scene depth buffer therefore hides reflections
behind foreground objects. Reflection draws never write depth and require no
depth clear, stencil buffer, offscreen target or texture readback. This is a
planar geometry reflection, not a render-to-texture demo.

A source-alpha water tint composites over the reflected scene. The existing
projected lantern and ground-shadow passes run afterward, illuminating and
shadowing the puddle surface. Reflected objects use their base material and
native fog; the reflection does not replay baked lightmaps, transparent candle
sprites or the dynamic lantern lighting on those objects.

The reflection scene uses coarse grave meshes and selected nearby trees and
facade polygons. Painter ordering is an approximation for this bounded scene;
it is not a general solution for intersecting reflected geometry. There are no
recursive reflections, ripples or shader effects.

## Checks

`./verify.ps1` runs the full OpenGL scene checks and 35 pixel controls on both
OpenGL and software Voodoo. Added controls cover foreground rejection, ground
coverage, preserved depth and source-alpha water tint. Captures from both sides
of the puddle must show reflected objects with zero changed pixels outside its
outline; a hidden nave view must remain unchanged when R is toggled.

`water.conf` runs only those reflection controls and captures (`/water`).
`./soak.ps1 -Stress` measures the busy final six seconds; `-FullTour` measures
the entire 6,000-frame, 100-second tour, with the profiler visible for its second
half. Windows wall-clock throughput and guest frame times are reported
separately; the Windows interval includes the two startup warmup frames.

The final complete tour measured **59.95 Windows FPS**, a **17.214 ms** maximum guest
frame and **zero frames over 18 ms**, including the new downward puddle views.
These results describe the configured DOSBox-X OpenGL backend, rather than
original-card throughput.
