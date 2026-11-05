# Wind-rippled graveyard water

C89 / Glide 2.43 demo combining the graveyard's planar puddle reflection with
two scrolling ripple textures, wind-driven roughness and floating leaves.
The standard target remains native emulated Voodoo Graphics, 640x480 at 60 Hz,
2 MB framebuffer plus one 2 MB TMU, `voodoo_maxmem=false`, no Glide wrapper.

Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`. Opens in FPS walk
beside the puddle. Twelve of the initial 64 leaves float on the surface and
four more start above it. The cathedral/graveyard tour is available with F.

| Key | Action |
| --- | --- |
| V | Rippled / still water comparison |
| + / - | Ripple strength, clamped to 0-100% |
| R | Planar reflection on/off |
| M / Z | Send a gust / wind on/off |
| , / . | Leaf count down/up by 16, from zero to 128 |
| L / G / C | Loose leaves / planted foliage / cutout-blend-solid comparison |
| U / I / N | FPS walk / FPS fly / manual drag view |
| WASD / mouse / Shift | Move / direct look / move faster |
| Q / E | Descend / ascend in FPS fly |
| F | Automatic tour / leave tour |
| P / O / X | Lantern / hold lantern in world / diagnostic grid |
| B / H | Lightmaps / fog |
| T / Y | Optional stone grain / ground shadows |
| Tab | Frame profiler with FPS and frame-time graphs |
| Esc | Exit |

The HUD shows ripple strength, the smoothed gust response, total leaf count
and floating count. The inherited manual drag view pauses scene animation
when selected or dragged; Space resumes it outside FPS modes. FPS walk/fly
remain animated with direct mouse look. All textures are original procedural
assets; no reference photographs or maps are saved.

## Mechanism

The reflection is the bounded, CPU-clipped planar reflection from `reflect`.
It mirrors nearby stones (including the low grave slabs), trunks and the west
facade into the eight-sided puddle, with raster depth evaluated on the water
plane. Thin slabs are double-sided in the reflection so their undersides remain
visible. Animated grass and loose leaves use their current world geometry;
tree crowns use camera-facing reflection proxies matching their overall extent.
Static and animated surfaces share one back-to-front sort. Reflected plants
are rejected against the puddle's view volume and visible room apertures before
geometry clipping.

A grazing-angle reflection approximation varies the water tint across the
plane: shallow sight lines show stronger reflections than a downward view.
Two 256x256 ALPHA8 textures provide visible, contrasting patches of water detail
without narrow bright lines. The layers have different color and strength. Each draws
in a separate additive pass on the single TMU. World-space UVs give the layers
different tiling, rotation and scroll direction; only coordinates change each
frame. Textures upload once at startup, adding 131,072 bytes for total TMU use
of **1,761,280 bytes**. The ripple effect needs no framebuffer readback,
texture re-upload or expanded Voodoo configuration.

Local gust strength feeds a damped water-roughness value: it builds quickly,
then eases back toward calm. Roughness modulates texture opacity, scrolling speed and leaf bobbing.
Scrolling phases are integrated over time so gust-speed changes stay continuous.
UV drift is derived from world-space velocity (with the required negative sign),
so stronger gusts carry both patterns along the leaves' +X/-Z wind direction.
The layers keep different small base drifts; gust speed grows with wind strength.
The direction regression checks recover world velocity from the measured UV
change, and verify that strong gusts increase the flow rather than reverse it.
The water plane and reflected geometry remain flat: these are animated surface
highlights, not normal mapping, physical wave geometry, refraction or a distortion
of the reflected scene. Still mode removes the two highlight passes; zero
strength gives the same scene pixels as still mode.

The layers share the reflection's CPU clipping and foreground depth rejection,
keep depth writes off, and use `GR_FOG_ADD2` so highlights fade away in fog
without adding another copy of fog color. The existing lantern/lightmap passes
and alpha-tested vegetation remain enabled. The stone-detail pass starts off;
this variant shares foliage's cheaper nave curves to keep CPU headroom.

Leaves retain the fixed 60 Hz bounded-pool simulation. A descending leaf over
exposed water catches the surface and becomes a floater. Floaters drift with
wind, rotate gently and bob; crossing the shoreline or reaching a raised receiver
strands them at the ground/support height. Dry leaves retain ordinary settling
and gust lift. This is a simple surface-contact model, with approximate grave
support heights rather than general rigid-body collision. V changes the water
highlight comparison; it does not switch off wind or horizontal leaf drift.

The profiler has 13 numeric stage rows; WATER measures reflection plus ripple
passes, while FOLIAGE includes the inherited optional detail pass. The compact
layout retains both history graphs and uses the existing texture allocation.

## Validation

`./verify.ps1` runs native OpenGL and software Voodoo pixel controls for reflection,
texture-alpha highlights, foreground rejection, preserved depth, additive fog,
zero alpha, both layers and foliage cutouts. Additional native tests check direct
FPS controls, floating/ground contact, drift, stranding, gust rise/decay and bounds.
Fixed-camera captures check clipping, still/zero-strength equality, repeatability,
UV motion and visible floaters. Additional views independently compare
slab and vegetation reflections from outside and through the nave doorway,
with zero reflection pixels outside the puddle. Optimized
plant culling matches an unculled reference image exactly. Captures and logs
are ignored.

`./soak.ps1 -Stress` measures 600 moving close-puddle frames at 128 leaves;
`./soak.ps1 -Doorway` checks the outward view through the nave doorway;
`./soak.ps1 -ReturnPath` checks the returning approach;
`./soak.ps1 -FullTour` measures the full 6,000-frame tour at 128 leaves.
The profiler opens halfway through each run. The checks require at least
59.5 Windows-clock FPS and no guest frames over 18 ms, for default alpha-tested
foliage with optional stone detail off. Unrestricted free flight and the heavier
blended-foliage comparison are outside that performance claim.

The latest simplified 600-frame close-puddle test at 128 leaves measures
**59.74 Windows FPS**, max guest frame **17.198 ms**, **zero over 18 ms**.
Native OpenGL and software Voodoo pixel/reflection checks pass.

The previous full-tour baseline with DOSBox-X 2026.01.02 on this machine at
128 leaves measured **59.96 Windows FPS**, max guest frame **17.215 ms**,
**zero frames over 18 ms**, with the profiler shown for the second half.
The dedicated doorway run also passes at 59.86 Windows FPS, max 17.205 ms,
zero over 18 ms. Both native OpenGL and software Voodoo pixel controls pass;
the optimized mirrored-plant culling matches its reference image exactly.
