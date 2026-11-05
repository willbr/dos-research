# Winchester masonry detail textures

Fine stone porosity layered over the existing nave and graveyard textures.
The [shadows demo](../shadows/README.md) supplies baked lightmaps, stained glass,
distance fog, a moving lantern and its ground shadows. This demo adds a nearby
detail layer without changing the scene meshes. It opens paused at a close
stone wall, with base texture on the left and detail grain on the right.
F starts the flight; its first two shots move close to the nave piers.

Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`.
Native C89 / DOS / Glide 2.43, 640x480 at 60 Hz, fixed 100000 cycles/ms,
standard Voodoo 1: 2 MB framebuffer, one 2 MB TMU, `voodoo_maxmem=false`,
Glide passthrough off. Compiler and runtime setup remain shared and vendored.

| Key | Action |
| --- | --- |
| D | Detail layer on/off |
| V | Split comparison / full-screen detail |
| + / - | Strength in exact 10-point steps, clamped to 0..100 |
| J / K | Decrease/increase tiling multiplier, clamped to 1..8 |
| B / H | Baked lightmaps / distance fog |
| P / O / X | Lantern toggle / hold in world / projection grid |
| S | Moving ground shadows |
| F | Automatic journey / manual look |
| U / I / N | FPS walk / FPS fly / drag-based manual look |
| WASD / Shift | Move / faster movement in FPS modes |
| Mouse | Button-free look in FPS modes |
| Q / E | Descend / rise in FPS fly |
| T / Y | Detail / shadows while WASD is in use |
| Space | Pause journey and lantern sweep |
| Left drag / arrows | Smooth manual look |
| Wheel / right drag | Local dolly |
| Tab | Frame profiler, including DETAIL and FPS/frame-time graphs |
| Esc | Exit |

Default strength is 100 and tiling is 2 to make the effect clear in the opening
comparison. The labeled split is clipped on the CPU so the base half stays
unchanged on both native backends. Pause before comparing D or settings.
Zoom uses the mouse wheel/right drag; +/- controls strength in this demo.
The existing 1/2/3, [ / ], Q fog controls remain available.
WASD overrides the D/S letter toggles in FPS modes; use T/Y instead.
Walking has basic collisions and free flight is noclip. F resumes the tour.
See the [shared camera controls](../fog/README.md) for movement behavior.

## Detail and distance fade

A deterministic, smoothly filtered grain pattern is generated in C into one
128x128 ALPHA8 texture. Alpha describes small darker pores, rather than
displacement or normal-map lighting. Stone and trim receive it; paving, grass,
glass, doors, sprites and sky do not. Both the original albedo and the detail
layer stay in place in world space as the camera moves.

Existing perspective-correct base UVs are multiplied by the tiling setting.
At tiling 4, a detail tile covers approximately half a scene metre
on ordinary wall mapping. Vertex alpha fades the contribution using reciprocal
camera depth: full strength at vertices within 2 units, zero at 6, interpolated
across each polygon. Polygons are clipped at
the far fade boundary before drawing. Higher tiling shortens the fade range
to keep smaller features from lingering into the distance. This is a vertex
fade with bilinear filtering, not a new mipmap or anisotropic-filtering demo.

The visible scene's group bounds reject distant geometry before collecting
detail polygons. Already clipped triangles/quads are reused; only nearby
polygons enter the fixed-size cache. Depth tests preserve foreground objects
and detail draws do not write depth.

## Fog and lighting composition

The layer is applied after baked lighting and the projected lantern, before
transparent effects. Let `C = f*F + (1-f)*L` be the existing opaque color,
with fog fraction f, fog color F and lit stone L. The grain supplies alpha A,
including strength and distance fade. The desired result is
`f*F + (1-f)*L*(1-A)`.

One pass multiplies the destination by `1-A`. When fog is enabled, a second
pass adds `f*F*A` using the existing fog table and MULT2 mode. Thus the layer
modulates the stone and all its existing illumination while preserving fog
color. With fog disabled, only the first pass is needed. This correction is
necessary on the configured native OpenGL backend, where the previously tested
PREFOG_COLOR shortcut does not work correctly. There are no shaders or extra
TMUs.

The grain adds **16,384 bytes**, bringing total texture allocation to
**1,236,992 bytes**, within the single 2 MB TMU. The profiler also caches static
dashboard labels in CPU memory; its raster jobs and uploads remain incremental.
Shared quad preparation avoids duplicate vertex marking and skips lightmap
bookkeeping for surfaces without lightmaps.

## Checks

`./verify.ps1` runs the native OpenGL scene tests and 39 pixel controls on both
OpenGL and software Voodoo. Eight new controls cover zero/half/full fog, fog
disabled, foreground rejection, unchanged depth and zero/full detail alpha.

Scene comparisons cover close lightmapped walls, gravestones, distant scenery,
fully fogged stone and fog disabled. Zero strength must match D off exactly.
Group rejection must match the unculled detail collection path exactly.
Distant geometry must receive no detail. Full fog must remain unchanged within
one framebuffer quantization step. `grain.conf` runs those checks alone.
The split view must preserve the base half exactly and match full detail on
the other half within one framebuffer quantization step. `torch.conf` runs
the shared moving-light clipping-depth stability regression.

`./soak.ps1 -Stress` measures the last six seconds of the flight; `-FullTour`
measures all 6,000 frames of its 100-second loop, with the profiler visible for
the second half. Windows wall-clock throughput and guest frame times are
reported separately, including two startup warmup frames in the Windows interval.
The split is disabled for benchmarks so they measure full-screen detail.

The camera-controls build measured **59.95 Windows FPS**, a **17.216 ms** maximum guest
frame and **zero frames over 18 ms** on the configured DOSBox-X OpenGL backend.
This is an emulator measurement, not original-card throughput.
