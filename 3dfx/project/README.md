# Winchester projected lantern

A moving, off-center lantern projects a warm spot across the nave's stone,
pillars and floor, then across the graveyard's headstones. Existing lightmaps,
stained glass, candle sprites, distance fog and the camera journey carry over.
The light draws on opaque geometry and follows its surfaces.

Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`. Standard Voodoo 1
settings are unchanged: one 2 MB TMU, 2 MB framebuffer, `voodoo_maxmem=false`,
640x480 at 60 Hz, fixed 100000 CPU cycles/ms. This remains native C / DOS /
Glide, using the shared vendored runtime and compiler setup.

| Key | Action |
| --- | --- |
| P | Projected lantern on/off |
| O | Hold the lantern's position and direction in the world / follow camera |
| X | Warm spot / colored projection grid |
| B | Baked lightmaps on/off |
| H | Distance fog on/off |
| F | Journey / manual look |
| U / I / N | FPS walk / FPS fly / drag-based manual look |
| WASD / Shift | Held movement / faster movement in FPS modes |
| Space / Ctrl | Rise / descend in FPS fly |
| Space | Pause journey and lantern sweep |
| Left drag / arrows | Smooth manual look |
| Wheel / right drag | Local dolly |
| 1 / 2 / 3, [ / ], Q | Fog curve, density and color |
| Tab | Profiler, including a separate **PROJECT** row |
| Esc | Exit |

FPS modes use button-free look, collision-aware walking or noclip flight;
see the [shared camera controls](../fog/README.md).

Use **O** with the journey running to see the camera move independently of
the fixed projector. **X** makes perspective and parallax easy to inspect;
the diagnostic grid disables the warm spot's vertex range fade. The default
lantern is offset 0.45 scene units sideways and 0.16 upward from the camera,
with a smooth horizontal and vertical sweep. Its projection frustum extends
from 0.12 to 11 scene units.

## Independent projection

The CPU constructs an independent projector basis and homogeneous S/T/Q
coordinates. At each screen vertex, with camera reciprocal depth `w=1/z`:

- FBI `oow` remains `1/z` for camera-based fog.
- TMU0 `oow` becomes `projectorDepth/z`.
- TMU0 `sow` and `tow` contain projector homogeneous S/T divided by camera z.
- Native interpolation and texture division produce the projector's UVs.

`GR_STWHINT_W_DIFF_TMU0` tells Glide these reciprocal coordinates differ.
The hint returns to zero afterward for the normal scene and HUD. Texture
coordinates span 0–255 even though the two projector textures are 128x128.
See the Programming Guide, printed pp. 79–84 and 92.

Visible camera-clipped polygons are reused for the added pass. Conservative
world bounds and camera-depth rejection avoid work outside the light volume;
six homogeneous clip planes handle the actual cone bounds, near plane and
range limit. This prevents projection behind the lantern and texture-edge
smearing. The pass tests depth with an eight-unit bias to tolerate native depth
rounding after clipping, and never writes depth. Camera-occluded surfaces
reject the light. Shadowing remains the scene's baked lightmap shadows.

The earlier one-unit bias allowed clipped triangles to fail the depth test
intermittently, making patches of the torch flash. A fixed-camera animated
beam regression compares that path with the corrected bias; shallow foreground
depth controls verify that the light still respects occluders. The shared
correction also applies to shadows, reflections and masonry-detail demos.

## Additive light and fog

The spot is additive projected lighting. If `B` is the textured, lightmapped
surface, `L` the projected light and `f` the camera fog factor, the result is
`f * fogColor + (1-f) * (B + L)`, subject to framebuffer clamping/dithering.

The existing scene supplies the fogged base. The extra pass uses
`GR_FOG_WITH_TABLE | GR_FOG_ADD2` and `ONE, ONE` blending, adding `(1-f)*L`
without adding the fog color a second time. A soft radial texture and
vertex-interpolated range fade shape the warm spot. The projection grid uses
constant intensity to isolate coordinate mapping in reference comparisons.

Two RGB565 128x128 cookies use 64 KiB. A 32 KiB alpha atlas caches the HUD;
only changed rows upload when FPS or a mode changes. Total TMU allocation is
**1,220,608 bytes**, including the original textures and profiler buffers.
The profiler rasterizes its background across two frames to keep its own
refresh work within the frame budget. PROJECT measures the added draw/clip
pass; receiver selection and polygon capture are included in OPAQUE.

## Validation

`./verify.ps1` runs OpenGL scene checks and software Voodoo pixel controls.
The 27 controls cover existing fog/lightmap behavior plus independent
projector Q at two scales, deliberately incorrect camera-W texture mapping,
fog indexed by camera W while the texture uses projector Q, full-fog
invariance and foreground occlusion. RGB565 rounding permits 12/255 per
channel. Analytical checks compare projector coordinates across four camera
poses and check the held-world transform.

Scene captures compare the lantern off/on indoors and outdoors, plus the
projection grid. The camera clearance and full/culled framebuffer checks
carry over. Constant/iterated-color comparisons use the original tolerance;
projective grid quad/triangle comparisons allow 660 scene pixels over a
16/255 channel difference for native fixed-point edge/gradient rounding
(0.3% of the viewport). The independent-coordinate controls establish the
mapping separately from those raster edge differences.

Run `./soak.ps1` for a 1,440-frame Windows-clock benchmark, or add `-FullTour`
for all 6,000 frames of the 100-second journey at 1/60-second increments.
`-Stress` densely samples the final six seconds over 600 frames, concentrating
on the doorway return that originally exceeded the frame budget.
Half the route shows the profiler. Benchmarks fail on any guest frame over
18 ms or Windows throughput below 59.5 FPS. Startup texture generation and
initial renderer warm-up are excluded. Generated captures and logs are ignored.

Final full-tour measurement on this machine: **59.98 Windows FPS** over
6,000 frames / 100.029 seconds, **60.04 guest FPS**, a **17.212 ms** maximum
guest frame and **zero frames over 18 ms**. The dense doorway stress test
also passes at 60.03 Windows FPS with zero frames over 18 ms.
