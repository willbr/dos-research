# Winchester Cathedral / mipmapping and LOD

This independent C / DOS / Glide demo reuses the cathedral exterior, smooth
orbit controls, drone route and profiler from [`textures`](../textures/README.md).
It compares unmipmapped bilinear textures with native mip selection, shows
which mip levels Voodoo selects, and exposes LOD bias and dithered transitions.

Run `run.cmd` or `./run.ps1`. Build with `./build.ps1`, or use `./setup.ps1`
to obtain Open Watcom first. The checked-in executable is `MIPMAPS.EXE`.
Configs mount the parent `3dfx` folder so the DOS program can read the shared
material files from `../textures/assets/`. No new reference images or art
are needed or copied.

## Controls

| Input | Action |
| --- | --- |
| M | Toggle mipmaps in the single view; default on. |
| C | Same-camera comparison: mipmaps off on the left, on on the right. |
| G | Replace opaque material maps with colored mip levels; legend shows texel dimensions. |
| [ / ] | Decrease/increase LOD bias in 0.25 steps, from -4 to +4. |
| 0 | Reset bias to +0.50. |
| N | Toggle nearest versus dithered mip transitions, retaining the chosen bias. |
| K | Substitute the checkerboard for architectural materials. |
| F | Switch between orbit and the smooth drone tour. |
| Space | Pause/resume orbit or route progress. |
| Left drag / arrows | Orbit; returns from drone mode. |
| Wheel / right drag / + / - | Smooth zoom; returns from drone mode. |
| R / middle click | Reset orbit; middle click also pauses it. |
| Tab | Emulated stage timings and FPS/frame-time graphs. |
| Escape | Exit. |

Click inside DOSBox-X to capture the mouse; Ctrl+F10 releases it. Pause and
toggle M to inspect the stone, lead roof and paving. Then use F to see the
effect on moving texture aliasing. G with [ / ] makes level selection clear:
negative bias favors finer levels; positive bias favors coarser, blurrier
levels. Both comparison panes retain perspective correction and identical
geometry/depth values.

## Emulator backend and measured speed

**This demo defaults to `voodoo_card=software`.** DOSBox-X 2026.01.02's OpenGL
Voodoo path generates its own host mip chain from a base image instead of
using our uploaded colored levels. Its LOD-bias diagnostic stayed at level
zero for both bias settings; the software backend passed the same check.
The implementation can be inspected in DOSBox-X's
[`voodoo_opengl.cpp`](https://github.com/joncampbell123/dosbox-x/blob/master/src/hardware/voodoo_opengl.cpp),
where `glTexImage2D(..., 0, ...)` is followed by `glGenerateMipmapEXT`.
This is an emulator/backend limitation, not a limitation of Voodoo hardware.
The original texture lab retains its OpenGL-backed configuration.

The software backend trades host performance for the behavior needed here.
Windows-clock measurements over the same moving path, 240 frames per mode:

| Mode | Host FPS |
| --- | ---: |
| Mipmaps off | 27.0 |
| Nearest mipmaps | 27.1 |
| Dithered mipmaps | 26.8 |

These small differences are within normal run variation. They do not establish
a performance penalty on physical Voodoo hardware. Reproduce with
`./benchmark.ps1`; results are saved to [`benchmark-results.csv`](benchmark-results.csv).

**`EMU FPS` and the profiler use guest/emulated time.** In this configuration
they report about 60 even while the host completes about 27 frames per real
second. The software renderer can slow the emulator below real time without
that cost appearing as guest CPU instructions. Windows-clock scripts measure
actual throughput. The drone route takes 80 emulated seconds, so it takes
longer on the host when the emulator cannot keep up. None of these counters
measures the host compositor's individual presentation events.

## Texture pipeline

Stone, roof, lawn, paving and checker maps each have a complete RGB565 chain:
256, 128, 64, 32, 16, 8, 4, 2 and 1 texel square. The CPU repeatedly averages
2 x 2 RGB565 samples with channel rounding. Chains are built and uploaded
once at startup using `grTexDownloadMipMapLevel`; no regeneration or per-frame
texture uploads are needed for them.

One additional chain contains a different solid color at every level and
is shared by all five materials when G is enabled. Surface lighting remains
active, so colors vary in brightness with orientation. Alpha-tested windows,
doors, font and profiler textures retain their base levels. This keeps alpha
coverage changes separate from the opaque-material experiment.

`grTexSource` supplies the complete chain; `grTexMipMapMode` selects disabled,
nearest or nearest-dither mode. Disabled mode also clamps the source to its
largest level for an explicit base-only control. `grTexLodBiasValue` adjusts
selection. +0.50 is the initial bias because nearest selection truncates LOD;
the manual recommends it for rounding. Dither mode is explicitly permitted
with `grHints(GR_HINT_ALLOW_MIPMAP_DITHER,1)`. Bias remains adjustable in that
mode; the manual suggests trying 0.0 or 0.25 for dithered selection.

This is bilinear sampling within a selected mip level. Dithered transitions
are not trilinear filtering. One-TMU multipass trilinear blending remains a
separate experiment. UI rendering disables mip selection and resets bias
before drawing the single-level font/dashboard.

The standard 4 MB Voodoo configuration is retained: 2 MB framebuffer memory
and one TMU with 2 MB texture memory, `voodoo_maxmem=false`, native Glide
passthrough off. Texture allocation, including debug/UI maps, is 1,450,016
bytes. The build targets 32-bit DOS / Pentium / x87 with the vendored SDK.

## Checks

`dosbox-x.exe -conf software.conf` runs a short framebuffer/API check.
`-conf test.conf` adds clipping, per-mode moving benchmarks, and a 600-frame
complete route with the profiler enabled halfway through. It verifies:

- A known RGB565 2 x 2 reduction and all 54 mip-level uploads.
- Same-camera off/on framebuffer differences in the scene region.
- Colored levels change with bias: the sampled mean level rose from 2.752
  at -2 to 6.517 at +2, across 103,488 matched pixels.
- Nearest/dither selection changes scene pixels (96,546 in the recorded view).
- Mode transitions, paused flight, clipping, route closure and camera clearance.

Captures are `OFF.PPM`, `ON.PPM`, `SPLIT.PPM`, `BASECOL.PPM`, `LEVELS.PPM`,
`BIASNEG.PPM`, `BIASPOS.PPM`, `DITHER.PPM` and `PROFILER.PPM`; logs go to
`MIPMAPS.LOG`. Logs/captures and copied runtime files are ignored by Git.

`./soak.ps1` measures 1,440 frames over the full accelerated route on the
Windows clock. It checks guest frame budgets and route validity; the printed
Windows FPS is the real throughput result, not the guest 60 FPS target.
The recorded full-tour run measured 25.95 host FPS, including the profiler
for the second half; maximum guest frame time was 17.38 ms with zero guest
frames above 18 ms.

`opengl-check.conf` deliberately runs the short diagnostic against the OpenGL
Voodoo backend. It is expected to fail the custom-level/bias check on the
tested emulator version. It is retained to detect whether a future backend
fix changes the result; it is not the normal launch configuration.

API sources: [Glide Reference](../Glide_Reference_Manual_2.4_199707.pdf), printed
pp. 92 (level upload), 97 (LOD bias), 100 (mipmap mode), 55-56 (hints); and the
[Programming Guide](../Glide_Programming_Guide_2.4_199707.pdf) texture chapter.
