# Winchester Cathedral / texture perspective

A native C / DOS / Glide exterior model of Winchester Cathedral, guided by
the supplied photographs, aerial views, floor plan and transverse section.
It includes the long nave, lower aisles, buttresses, west-front window and
pinnacles, square crossing tower, transepts and stepped eastern chapels.
Repeated architectural details and proportions are interpreted rather than
surveyed. The model has 4,334 vertices and 2,078 triangles.

The reference images were used only for visual guidance and were not saved
into the project. The original material atlas and its generation prompt are
documented in [`assets/README.md`](assets/README.md).

Run `run.cmd` or `./run.ps1` with DOSBox-X on PATH. The executable is
`CATHEDRL.EXE` to keep its DOS filename within eight characters.

## Controls

| Input | Action |
| --- | --- |
| F | Switch between orbit and the automatic drone flight. |
| Space | Pause/resume the current orbit or flight. |
| Left mouse drag | Orbit smoothly; release to coast. Returns from drone mode. |
| Wheel / right mouse drag | Smooth zoom; returns from drone mode. |
| Middle click / R | Reset the orbit view. Middle click also pauses it. |
| Arrow keys | Adjust orbit angle with damped acceleration. Returns from drone mode. |
| + / - | Zoom in/out. |
| P | Toggle perspective-correct / affine textures in the single view. |
| C | Side-by-side comparison: perspective left, affine right. |
| K | Substitute a checkerboard on stone, roofs and paving. |
| Tab | Toggle stage timings, FPS/frame-time graphs, P95 and maximum. |
| Escape | Exit to DOS. |

Click the window to capture the mouse; Ctrl+F10 releases it. For a direct
comparison, pause, enable K and toggle P. C uses the same camera in both
panes. Split view renders the scene twice; the 120-frame moving comparison
averaged 44.90 FPS on the emulated clock.
The live FPS counter counts completed refresh-paced frames over RTC seconds;
it becomes available after its initial sampling interval.

## Drone camera

The flight approaches the west front, passes the nave, sweeps around the
crossing and east end, and pulls back for a wider view. It loops over roughly
80 seconds. Position and look target follow closed Catmull-Rom curves; a
2,048-sample arc-length table keeps travel speed approximately constant.
The horizon stays level. This is an exterior tour rather than an interior
walkthrough or a general collision-controlled free camera.

F enters the route near the current camera position. Switching modes blends
position and look target over 1.5 seconds; returning to orbit derives its
angle from the current camera. Flight timing uses recent measured frame time.
Space freezes route progress. Mouse dragging, zooming or arrow input returns
to orbit, where the helmet demo's spring acceleration and flick inertia are
retained.

## Rendering and comparison

The CPU supplies camera transforms, backface culling, six-plane 3D clipping
and static face lighting. Only vertices referenced by facing triangles are
transformed. Clip planes containing the complete source triangle are skipped;
the remaining planes interpolate position, texture coordinates and color
before projection. Fully visible triangles use cached projected vertices.

Voodoo supplies textured rasterization, bilinear filtering, dithering and
16-bit depth testing. The native texture-coordinate comparison follows the
vendored SDK's [`test29.c`](../vendor/glide/Glide/Src/Sst1/glide/tests/test29.c):

- Perspective mode supplies `s/z`, `t/z` and texture reciprocal W `1/z`.
- Affine mode supplies raw `s`, `t` and constant texture reciprocal W `1`.
- `grHints(GR_HINT_STWHINT, GR_STWHINT_W_DIFF_TMU0)` separates texture W from
  the vertex/depth value. Geometry and Z-buffer values stay the same.

Both paths run on Voodoo; this is not a software texture rasterizer.
Pointed windows and doorways are opaque alpha-tested texture cards with
procedural frames/mullions. Small depth biases keep these cards and paving
stable against their underlying surfaces. There is no programmable shading.
This demo uses only base texture levels; mipmapping remains a separate demo.
Grass and roof contrast are reduced at load time to keep their material
variation readable at the intended viewing distances.

The supplied configuration uses DOSBox-X's OpenGL Voodoo backend, Glide
passthrough off, fixed 100,000 cycles/ms, 32 MB system RAM, 640 x 480 at 60 Hz,
and `voodoo_maxmem=false`. Hardware queries confirmed 2 MB framebuffer memory
and one TMU with 2 MB texture memory. Material maps, cutouts, checkerboard,
font and two profiler textures use 1,056,768 bytes (1,032 KiB) of that TMU.

## Build and checks

Run `./setup.ps1` once, or `./build.ps1` after C changes. Builds share Open
Watcom from `../cube/tools/watcom` and use the vendored Glide SDK/runtime.
The profiler clock/history module is shared with `../helmet/profiler.c`.
Checked-in RGB24 material files are sufficient: building/running does not
require image generation, Python, FFmpeg or saved reference photos.

From this folder, `dosbox-x.exe -conf test.conf` runs a finite test. It checks:

- Orbit/drone mode changes preserve the starting position; pausing freezes
  flight progress.
- Same-camera perspective/affine output changes pixels, with checker textures
  and identical geometry/depth setup.
- Close-camera near/frustum clipping produces finite screen coordinates.
- A complete route stays outside conservative building bounds, closes at
  the same point and has approximately constant travel steps.
- Actual rendering over 600 route samples, with the profiler enabled halfway.
- A separate 120-frame moving split-screen comparison.

Captures include `CATHEDRL.PPM`, `PERSPECT.PPM`, `AFFINE.PPM`, `SPLIT.PPM`,
`WEST.PPM`, `EAST.PPM`, `CLIP.PPM` and `PROFILER.PPM`. Results are written to
`CATHEDRL.LOG`. Captures/logs and copied runtime binaries are ignored by Git.
The source, renderer assets and compiled demo are included.

`./soak.ps1` measures 1,440 single-view frames on the Windows wall clock,
sampling a full drone route and enabling the profiler for its second half.
It fails if any emulated frame exceeds 18 ms or a route/geometry check fails.
Startup, capture readback and camera-table construction are excluded. The
profiler measures guest CPU/API time, including refresh waits; it does not
provide host GPU timestamps or compositor presentation measurements.

The initial scalar implementation ran near 30 FPS. Backface-first vertex
selection and selective clipping reached 60.03 Windows wall-clock FPS over
1,440 frames. The final build measured 60.01 FPS, with no emulated frame over
18 ms (maximum 17.36 ms). Results
depend on the host/backend; use the supplied test to check a different setup.

API sources: [Glide Reference](../Glide_Reference_Manual_2.4_199707.pdf),
`grHints` (printed p. 55), `grDepthBiasLevel` (p. 31), and the
[Programming Guide](../Glide_Programming_Guide_2.4_199707.pdf) texture chapter.
