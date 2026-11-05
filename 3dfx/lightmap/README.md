# Winchester lightmaps and fog

The nave-to-graveyard journey with native, single-TMU lightmapping: warm
candle pools and colored stained-glass patterns on the nave floor, light
falloff along the stone walls, and cool moonlit ground with baked grave
shadows outside. The cathedral references remain reference-only.

Run `./run.ps1` or `run.cmd`. Build with `./build.ps1`; the compiler and DOS
runtimes are shared with the other [3dfx demos](../README.md). The standard
Voodoo 1 settings remain unchanged: one 2 MB TMU, 2 MB framebuffer,
`voodoo_maxmem=false`, fixed 100000 CPU cycles/ms, 640x480 at 60 Hz.

| Key | Action |
| --- | --- |
| B | Lightmaps / base textures only |
| H | Distance fog off/on |
| F | Journey / manual look |
| F2 / F3 / F4 | FPS walk / FPS fly / drag-based manual look |
| WASD / Shift | Held movement / faster movement in FPS modes |
| Space / Ctrl | Rise / descend in FPS fly |
| Space | Pause |
| Left drag / arrows | Smooth look with inertia |
| Wheel / right drag | Local dolly |
| 1 / 2 / 3 | Linear / exponential / exponential-squared fog |
| [ / ] | Fog density |
| Q | Fog color |
| Tab | Profiler, FPS and frame-time graphs |
| Esc | Exit |

FPS modes use button-free look, collision-aware walking or noclip flight;
see the [shared camera controls](../fog/README.md).

The original blending demo's G/C/M/L/S switches remain available. Sprite mist
and the old additive floor-light cards start off here, so the lightmaps can
be assessed independently. Lightmaps are static; moving candle flame/glow
sprites remain animated. The ground shadows and colored patterns are
procedural baked approximations, not dynamic shadow maps or ray-traced light.

## Rendering

An RGB565 256x256 atlas contains four independently mapped surfaces: nave
floor, left wall, graveyard ground and right wall. The base stone/grass UVs
tile independently of the lightmap UVs. Atlas construction happens once at
startup in C. The complete texture allocation is **1,122,304 bytes**, including
the profiler buffers, within the standard 2 MB TMU budget.

With fog disabled, the base-texture pass writes depth, then a second texture
pass multiplies the framebuffer by the lightmap using `DST_COLOR, ZERO`.
The second pass tests **equal depth** and never writes depth; its clipped
vertices are cached from the base pass. This keeps the light on its surface
and prevents it from painting foreground piers, candles or headstones.

With fog enabled, receiver surfaces use three raster passes:

1. Base texture with `GR_FOG_ADD2`: `(1-f) * base`.
2. Lightmap texture, fog disabled, destination-color multiplication:
   `(1-f) * base * lightmap`.
3. `GR_FOG_MULT2`, additive blending: add `f * fogColor` once.

Glass and additive lights draw afterward with their normal fog/blending
rules. The resulting equation is `f * fogColor + (1-f) * base * lightmap`.
Nonreceiver geometry retains its original lighting and ordinary fog.

The manual also provides a two-pass formulation using `GR_BLEND_PREFOG_COLOR`
in the final blend. Pixel controls pass with that factor on software Voodoo
but fail on DOSBox-X 2026.01.02's OpenGL backend. The explicit fog resolve
above works on both backends. Two texture passes plus the fog-color resolve
are therefore the default. See the Programming Guide, printed pp. 68–72.

## Optimization and validation

The renderer shares [the fog demo's optimizations](../fog/README.md): object
and opening visibility, coarse distant gravestones, shared position transforms,
constant face colors, quad clipping, and shared reciprocal depth. Everything
is C. Tab shows the extra lightmap/fog-resolve work as **Lightmaps**.

`./verify.ps1` runs the full OpenGL scene test and software Voodoo pixel
controls. Controls cover native multiplication, half fog, full fog independent
of lightmap tint, and fog disabled, in addition to all 16 fog controls.
Nave and graveyard captures compare B on/off. Culling must exactly match the
unculled framebuffer at 16 keys. Constant/iterated color and quad/triangle
reference comparisons check for missing surfaces and visible color changes.
The route and camera transitions retain the fog demo's clearance checks.

`./soak.ps1` measures 1,440 frames by Windows wall clock, with the profiler
visible for half the route. `./soak.ps1 -FullTour` renders 6,000 frames, sampling
the whole 100-second route at 1/60-second increments. Both fail on any guest
frame over 18 ms or host throughput below 59.5 FPS. Logs and captures are
ignored local outputs. Run performance tests with other demos closed.

On this machine, the full 6,000-frame tour measured **59.98 Windows FPS**
(100.037 seconds), **60.04 guest FPS**, a maximum guest frame time of
**17.218 ms**, and **zero frames over 18 ms**. The profiler was visible for
the second half. Culling matched the full framebuffer at all 16 keys;
constant-color reference comparisons had zero pixels differing by over
16/255, and quad/triangle comparisons had at most eight such edge pixels.
