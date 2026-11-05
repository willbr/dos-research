# Graveyard foliage and traveling wind

A C89 / Glide 2.43 demo of native **alpha testing** on the standard DOSBox-X
Voodoo Graphics target: 640x480 at 60 Hz, 2 MB framebuffer, one 2 MB TMU,
`voodoo_maxmem=false`, no host Glide wrapper.

Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`. Opens outside the
nave in FPS walk mode, looking toward the trees. The automatic cathedral /
graveyard tour remains available. All images are original procedural textures;
the user's reference photographs and maps are not copied into this demo.

| Key | Action |
| --- | --- |
| U / I / N | FPS walk / FPS fly / drag-based manual view |
| WASD, mouse, Shift | Move, direct mouse look, move faster |
| Q / E | Lower / raise in FPS fly |
| F | Automatic tour / leave tour |
| G | Tree canopies, grass and ivy on/off |
| C | Cycle alpha cutouts / sorted alpha blending / solid cards |
| Z | Wind on/off |
| M | Send an additional gust across the scene (wind must be on) |
| L | Loose leaves on/off |
| , / . | Decrease / increase leaf count by 16, from 0 to 128 |
| T / Y | Stone detail / ground shadows |
| P / O / X | Lantern on/off / hold in world / projection grid |
| B / H | Lightmaps / fog |
| Tab | Frame profiler, FPS and frame-time graphs |
| Esc | Exit |

Starts with 64 loose leaves. The HUD displays the selected count and how many
are airborne or resting. The pool is allocated once for 128 leaves; count
changes do not allocate memory or upload textures. Disabling loose leaves hides
them while their simulation continues. Zero count stops particle simulation.
The G switch affects planted foliage independently of loose leaves.

The inherited stone-detail pass starts off; T can enable it for comparison.
This variant uses eight vault/rib steps and six arcade steps (instead of 12/8)
to leave CPU headroom when looking through the doorway. Other demos retain
their existing geometry.
The nave's glass and candles remain enabled. In this demo G, C and L control
foliage instead of the older glass/candle/light-pattern switches. Q changes fog
color outside FPS fly; in fly mode it controls height.

## Rendering and wind

Each tree replaces the old opaque polygon crown with intersecting textured
branch sprays. Grass clumps and ivy use the same cutout technique. Three
256x256 ARGB1555 textures contain branch/leaf clusters, grass and a copper leaf.
They add 393,216 bytes, bringing texture use to **1,630,208 bytes** of the
standard 2 MB TMU. All textures upload once at startup.

The default mode uses `grAlphaTestFunction(GR_CMP_GREATER)` with reference 127,
ordinary depth tests and depth writes. Transparent texels change neither color
nor depth; opaque leaves occlude later sprites. Bilinear filtering softens the
sample before the cutout threshold. This is not full-scene AA or alpha-to-coverage.
The blend comparison sorts cards back to front, disables depth writes and uses
source-alpha blending. Solid-card mode exposes the textured rectangles.

A baseline breeze and a 4.5-second gust envelope repeat every 14 seconds, with
position-dependent delay so the gust crosses the graveyard from west to east.
Tree crowns sway together, smaller sprays flutter, and grass bends near its
base. Simulation and vertices run on the CPU; no programmable shaders are used.
A 1,024-sample interpolated sine table and conservative tree/grass visibility
checks reduce per-frame work. Static model bounds remain unchanged in other demos.

The bounded leaf pool runs at a fixed 60 Hz, with drag, gravity, terminal fall
speed and flutter. Leaves settle and remain at rest; a later strong gust can
lift them again. Ground/path and simplified grave/slab support heights prevent
resting leaves from sinking through receivers. The world boundary contains
particles; this is not a general collision or rigid-body simulation. Gusts do
not break new leaves off branches. Existing baked ground lighting and coarse
lantern shadows remain; alpha foliage does not cast detailed leaf shadows or
receive the separate projected-lantern pass.

The profiler's final FOLIAGE row includes both planted/loose-leaf drawing and
the inherited stone-detail pass. Frame-time and FPS graphs retain their existing
meaning; Windows marker timing is used for throughput measurements.

## Checks

`./verify.ps1` checks native OpenGL and software Voodoo alpha/depth/fog controls.
It also tests direct FPS look, held controls, settling, gust lift, propagation,
count bounds and same-camera scene differences. Native captures include
`CUTOUT.PPM`, `GUST.PPM`, `BLENDED.PPM`, `SOLID.PPM` and `LEAVES.PPM` (ignored).

`./soak.ps1 -FullTour` measures the complete 6,000-frame tour at **128 leaves**;
`./soak.ps1 -Stress` measures 600 moving outdoor frames at the same maximum.
`./soak.ps1 -ReturnPath` checks the expensive doorway/nave approach separately.
The profiler appears halfway through each run. The checks require at least
59.5 Windows-clock FPS and no guest frames over 18 ms. These measurements apply
to the default alpha-cutout mode (stone detail off) and scripted routes, not
unrestricted free flight or the deliberately heavier blended comparison.

Measured with DOSBox-X 2026.01.02 on this machine: the full tour completed at
**59.94 Windows-clock FPS**, maximum guest frame **17.213 ms**, **zero frames
over 18 ms**, with 128 leaves and the profiler visible for the second half.
The 600-frame doorway approach measured 59.74 Windows FPS, max 17.212 ms,
zero frames over 18 ms. Both native OpenGL and software Voodoo pixel controls
passed; the simulation test settled 128/128 leaves and a later gust lifted 119.
