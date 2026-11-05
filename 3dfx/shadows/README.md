# Winchester lantern ground shadows

The nave and graveyard tour with moving hard shadows from the projected
lantern. Pillars, headstones and crosses cast onto the nave floor, grass and
raised path. Baked candle pools, stained-glass patterns and moonlit lighting
remain underneath.

Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`.
Native C89 / DOS / Glide 2.43, standard Voodoo 1: 2 MB framebuffer, one
2 MB TMU, `voodoo_maxmem=false`, 640x480 at 60 Hz, fixed 100000 cycles/ms.
The shared compiler and Glide runtime setup is unchanged.

| Key | Action |
| --- | --- |
| S | Ground shadows on/off |
| P | Projected lantern on/off |
| O | Hold lantern in world / follow camera |
| X | Warm beam / diagnostic projection grid |
| B / H | Baked lightmaps / distance fog |
| F | Automatic journey / manual look |
| U / I / N | FPS walk / FPS fly / drag-based manual look |
| WASD / Shift | Held movement / faster movement in FPS modes |
| Q / E | Descend / rise in FPS fly |
| Y | Shadows while S is used for backward movement |
| Space | Pause journey and lantern sweep |
| Left drag / arrows | Smooth manual look |
| Wheel / right drag | Local dolly |
| Tab | Frame profiler with FPS, frame time and SHADOW row |
| Esc | Exit |

## Ground mask

FPS modes use button-free mouse look. Walking has basic collisions; flying is
noclip. See the [shared camera controls](../fog/README.md) for details.

The CPU clips convex caster volumes below the point light, projects them
onto each receiver plane, builds their convex silhouettes, and clips those
silhouettes to the ground regions and camera viewport. The pillar shaft uses
a direct clipped-ring path. Conservative light bounds and beam planes reject
irrelevant casters; clipping skips planes that contain every vertex.

After the opaque scene and lightmaps, a depth-only pass stamps the silhouettes
using `ZERO, ONE` color blending and a small depth bias. Scene RGB stays intact.
The mask uses a 24-unit bias, ahead of the lantern's eight-unit clipping
tolerance, so the stable light pass still respects moving ground shadows.
The additive lantern pass then fails its depth test inside the mask. Overlapping
shadows form a union rather than accumulating darkness; foreground geometry
retains its depth. The next frame clears the mask with the normal depth clear.
This uses neither a stencil buffer nor a shadow texture and adds no TMU memory:
total texture allocation remains 1,220,608 bytes.

This demo implements hard planar shadows for selected pillar and grave geometry.
It does not cast onto walls, roofs, other objects or uneven terrain. The mask
applies to the moving lantern; baked lighting has its own static shadows.

## Verification

`./verify.ps1` checks native OpenGL scene captures and OpenGL/software Voodoo
pixel controls. Four additional controls check color preservation, lantern
rejection, overlapping masks and foreground depth. Scene comparisons check
both rooms against the unshadowed lantern and baked base; switching the lantern
off must make the shadow toggle pixel-identical. Projection checks cover a
moving light, raised receiver and caster crossing the light's horizon.

`./soak.ps1 -Stress` measures the busiest final six seconds; `-FullTour` measures
all 6,000 frames of the 100-second moving-camera tour, with the profiler visible
for its second half. Windows wall-clock throughput and guest frame times are
reported separately. The profiler rasterizes and uploads in small slices to
avoid redraw bursts consuming the shadow pass's frame budget.

Measured on the configured DOSBox-X OpenGL backend: the complete tour ran at
**59.96 Windows FPS**, with a **17.210 ms** maximum guest frame and **zero
frames over 18 ms**. The Windows interval includes the two startup warmup
frames; the start-marker disk write precedes those frames so the measured
guest frames begin synchronized to retrace. These are emulator measurements,
not a claim about original-card throughput.
