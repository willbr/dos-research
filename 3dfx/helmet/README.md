# Diving helmet / edge anti-aliasing

For the separate environment-reflection demo, see
[`../envmap`](../envmap/README.md). This demo retains its original Fast meshes
and keyboard zoom controls.

A procedural C model inspired by the user's diving-helmet photograph from
Winchester Cathedral: an aged copper dome and breastplate, collar rings,
brass/metal window bezels, dark glass, front grille, side locking pins,
hose fittings and shoulder studs. This is an interpreted model, not a
photogrammetric reconstruction or a historically exact engineering model.

Run `run.cmd` or `./run.ps1` with DOSBox-X on PATH. The supplied configs use
640×480 at 60 Hz, standard Voodoo 1 memory (`voodoo_maxmem=false`, 4 MB,
one TMU), and OpenGL-backed hardware emulation with Glide passthrough off.

## Controls

| Input | Action |
| --- | --- |
| Left mouse drag | Orbit with smooth acceleration; release to coast and slow down. Pauses the automatic turntable. |
| Mouse wheel | Smooth zoom. |
| Right mouse drag | Smooth zoom, including when no wheel is available. |
| Middle click | Smoothly reset the view and stop the automatic turntable. |
| A | Toggle silhouette/boundary edge AA; status appears at top right. |
| Tab | Show/hide frame profiler, FPS graph and frame-time graph. |
| Q | Switch between Fast (default) and Detail geometry/lighting. |
| Space | Pause or resume the slow turntable. |
| Left/right | Adjust target yaw; a damped spring accelerates and settles the turn. |
| Up/down | Adjust target pitch with the same smoothing. |
| T | Toggle material textures to compare with plain colored surfaces. |
| + / - | Zoom in or out. |
| R | Smoothly reset the view and cancel flick momentum. |
| Escape | Exit. |

For the most useful AA comparison, pause first, then toggle A without
moving the camera. Inspect the dome, bezel outlines, pins and grille.
Click inside DOSBox-X to capture the mouse; Ctrl+F10 releases it. A new
left-button grab catches the current pose and cancels any coast. Flick
velocity is filtered from recent relative motion and decays with friction;
holding still before release produces little momentum. Right-drag zoom and
the keyboard remain available if a mouse driver lacks wheel support.
The supplied configs disable wheel-to-key conversion so the DOS WheelAPI
receives the real wheel counter. Mouse input uses INT 33h polling, not host
OS hooks. The DOS cursor is hidden while the model is shown.

Camera motion uses a critically damped spring rather than jumping to each
keypress. Its time step uses the recent measured FPS (30 FPS initially),
so it is approximate during sudden performance changes; low rendered FPS
still limits how many smooth intermediate poses can actually be displayed.
The FPS counter updates over complete RTC seconds and counts refresh-paced
frames; it does not measure host-compositor presentation directly. Rotation
advances once per rendered frame, so it slows down if 60 Hz cannot be met.

## Rendering

`model.c` constructs a triangle mesh from a revolved shell, an annular
shoulder surface, elliptical tori, cylinders and glass disks. Positions are
welded for adjacency even at UV/normal seams. The build rejects capacity
overflow and nonmanifold edges. Overlapping fittings are separate components;
the helmet is not one Boolean-unioned, watertight solid.

The CPU transforms/projects vertices, computes diffuse lighting and specular
vertex intensities, culls backfaces, and identifies silhouette edges. Glide
renders textured triangles with a depth buffer and bilinear filtering.
Fast uses 1,732 vertices / 2,238 faces nearby and folds approximate highlights
into vertex lighting. Below zoom 0.84, small bars, locking pins, shoulder studs
and the maker's-plate ring use fewer segments (1,578 vertices / 2,060 faces).
The dome, window rims, collar rings and shoulder silhouette retain their
geometry. Above zoom 0.92, the near fittings return; the gap prevents rapid
switching around a threshold. Both Fast meshes are cached before rendering
and selected by pointer, without rebuilding or copying a mesh during zoom.
The status reads `Q FAST FAR` when the distant mesh is active.

Both quality modes now compute lighting once per exact normal/material group
per frame, then reuse those intensities for vertices with the same normal.
The production rendering path is C. Detail preserves the original 4,056 vertices / 5,784 faces
and adds a separate additive Gouraud highlight pass. There are no programmable
shaders, normal maps, hardware transform/lighting or extra TMUs.

Fast draws interior triangles normally, then uses `grAADrawTriangle` for
triangles that touch a silhouette or open boundary. Their interiors and edge
coverage are rendered once, with depth writes enabled. Detail draws all opaque
geometry first, then redraws boundary triangles using a greater-or-equal depth
comparison and disabled depth writes, followed by the specular pass. Both
paths use iterated alpha and source-alpha blending; shared internal mesh
edges are excluded.
This is coverage-based primitive-edge AA, not full-scene MSAA. Specular
highlights are not independently antialiased outside the base silhouettes.

Three 256×256 RGB565 material maps and an alpha-only font atlas consume
about 392 KiB; two profiler textures add 128 KiB, for 520 KiB of the single
TMU's 2 MiB texture memory. The source material
tile was generated from the photograph as a reference; see
[`assets/README.md`](assets/README.md) for provenance and the exact prompt.
The font is generated deterministically from small bitmaps in `helmet.c`.

## Profiler and performance

Tab opens stage timings for input, background, transform/culling, opaque
triangles, edge AA, highlights, HUD, profiler and presentation/wait. It also
shows average FPS/frame time, P95 and maximum frame time, and rolling graphs
of the last 128 frames. The graphs mark 60 FPS / 16.7 ms; frame times above
18 ms appear red. The panel refreshes roughly three times per second, with
rasterization and texture uploads spread across frames to limit overhead.

Timing uses DOSBox-X's integration-device emulated-millisecond counter
(`integration device=true` in the supplied configs). These are guest CPU/API
times, including refresh waits, rather than host GPU timestamps or compositor
presentation measurements. Without that device the panel reports an
unavailable timer. Windows wall-clock scripts provide a separate FPS check.

At the supplied fixed 100,000 cycles/ms and standard 4 MB Voodoo configuration,
Fast AA completed two consecutive 1,440-frame, two-orbit tests at 60.02–60.03
Windows wall-clock FPS (60.04 emulated FPS), with no frames above 18 ms
(maximum about 17.21 ms), including the profiler throughout the second orbit
and zoom-driven fitting-LOD switches. The CPU transforms, material batching and AA submission
were optimized; Fast also reduces tessellation and highlight detail. Q
restores the original Detail quality, which does not maintain 60 FPS.
Performance depends on the host and emulator backend.
An earlier run recorded one 18.06 ms outlier while still averaging 60.04 FPS;
slow-frame logging now records camera/LOD state and stage costs for diagnosis.

## Build and checks

Run `./setup.ps1` once to obtain Open Watcom if needed, then `./build.ps1`
after C changes. The compiler is shared with `../cube/tools/watcom`; SDK
and runtime files are vendored under `../vendor`. Material data is already
included, so building does not require image generation, Python or FFmpeg.

Run `dosbox-x.exe -conf test.conf` from this directory for a finite test.
It checks DOS mouse/WheelAPI detection and synthetic input sequences for
drag targets, pitch/zoom limits, spring acceleration/settling and flick
momentum/cancellation. It validates mesh construction, renders a fixed pose with AA off and on,
reads the actual Voodoo framebuffer into `AAOFF.PPM` and `AAON.PPM`, checks
that the scene is nonblank and AA changes pixels within the model region,
and captures a second angle as `SIDE.PPM`. Results go to `HELMET.LOG`.

`profile.conf` runs these checks plus a profiler capture (`PROFILER.PPM`)
and timer checks. `detail.conf` tests the original higher-detail model and
its AA path. `./soak.ps1` measures 1,440 AA frames with the Windows wall
clock, varying the camera through two full orbits and enabling the profiler
halfway through. It fails if any measured frame exceeds 18 ms. Capture and
model-build pauses are excluded from this steady-state performance test.
The second orbit varies zoom from 0.65 to 1.29, exercising cached fitting-LOD
switches with the profiler visible.

`shared.conf` compares the production C lighting path with per-vertex reference
lighting over 24 views per quality, then benchmarks three 360-pose trials and
128 rendered AA frames. Results go to `SHARED.LOG`. On the production build,
transform/culling fell from 4.358 to 4.052 ms in Fast and 9.978 to 8.888 ms in
Detail, with identical RGB/specular values in the sampled views. Maximum
projection deviation was 0.000061 pixels and AA edge flags matched exactly.

`lod.conf` compares near/distant fittings over 240 moving AA frames at zoom
0.72, captures `NEAR.PPM` / `FAR.PPM`, checks nonblank output and mesh reduction,
and validates 1,000 cached switches plus threshold hysteresis. Measured frame
work excluding presentation/wait fell from 13.589 to 12.653 ms; edge AA fell
from 6.345 to 5.815 ms and transform/culling from 4.052 to 3.749 ms. Results
go to `HELMET.LOG`.

`./benchmark.ps1` measures 60 frames in each of three configurations using
the Windows wall clock: textures/highlights without AA, with AA, then textures
without either extra pass. Run with other demo instances closed. All three
use the same changing camera pose; startup and two warm-up frames per phase
are excluded. This separates the cost of edge AA and highlights from the
baseline CPU transform/lighting work and thousands of Glide triangle calls.

The front and side captures are also reviewed visually. Changed-pixel counts
establish that the switch affects output, not that every AA pixel is correct
on physical hardware. Emulated rendering and performance can differ from
an original Voodoo card. Generated screenshots/logs are ignored by Git;
model source, source photo, material tile and renderer input are included.

### Moving-camera optimization experiments

Run `./optbench.ps1` (roughly four minutes) for `/optbench`. It compares the
original C renderer with shared-normal lighting, compact position/UV/material
streams, interpolated specular and trig tables, and an inline x87 projection
routine. Combinations and a matching experimental C control are included.
Every kernel is compiled separately with constant options, so per-vertex
option dispatch does not distort the comparison. The normal demo uses the
production shared-normal C renderer; reference and alternative kernels remain
available for benchmarks. Table/cache construction is excluded from
the timings. The recorded CSV can also be regenerated from an existing
successful log with `./optbench.ps1 -ResultsOnly`.

Both Fast and Detail use AA, textures and highlights, with the profiler panel
hidden. Each of three transform/culling trials samples 360 moving camera
poses around a full orbit, varying pitch and zoom. Each variant also renders
128 complete AA frames; `work` excludes presentation/wait and `frame` includes
it. Timings use the integration-device emulated clock with the supplied fixed
CPU configuration, rather than the Windows wall clock. Refresh pacing can
hide CPU improvements in the complete-frame result. These measurements do
not establish cache or instruction scheduling performance on a real Pentium.

Before timing, each kernel is compared with the reference at 24 views per
quality: screen coordinates, RGB, specular strength, perspective texture
coordinates, finite outputs, per-face visibility and individual AA edge
flags. The allowed maximum deviations are 0.02 screen pixels, 0.08 RGB/specular
units (0–255 scale), 0.02 perspective UV units, and zero topology differences.
Raw results are written to ignored `OPTBENCH.LOG`; the recorded results are
in `benchmark-results.csv`.

Recorded before production integration on 2026-11-04 with the supplied DOSBox-X configuration and Open Watcom
`-5s -fp5 -ox`. Mean transform/culling time across three 360-pose trials:

| Variant | Fast, ms | Detail, ms |
| --- | ---: | ---: |
| Reference C | 4.232 | 9.655 |
| Experimental C control | 4.370 | 9.978 |
| Shared-normal lighting | 4.008 | 8.767 |
| Specular LUT (1,024 intervals, linear interpolation) | 4.977 | 11.392 |
| Compact position/UV/material streams | 4.405 | 10.059 |
| Trig LUT (2,048 intervals, linear interpolation) | 4.374 | 9.982 |
| Compact streams + inline x87 projection | 4.404 | 10.058 |

Shared-normal lighting improves the reference stage by 5.3% in Fast and
9.2% in Detail. Lighting is still recomputed every frame, once per exact
normal/material group: 1,075 groups for 1,732 Fast vertices, or 1,944 for
4,056 Detail vertices. This works while the camera moves.

The tested compact layout is slightly slower than its matching C control;
the assembly routine saves only 0.0005 / 0.0013 ms against that compact C
path. Interpolated LUTs are slower than the existing arithmetic. Complete
AA frames remain around 60 FPS in Fast and 20 FPS in Detail on the tested
path. Shared-normal lighting was selected for production. The other kernels
remain available for comparison through `/optbench`.

Relevant documentation: [Glide Programming Guide](../Glide_Programming_Guide_2.4_199707.pdf),
printed pp. 34–37 (edge AA) and pp. 39–46 (color/lighting), plus the
[API Reference](../Glide_Reference_Manual_2.4_199707.pdf).
