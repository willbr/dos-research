# Procedural fire and partial texture uploads

C89 / Glide 2.43 demo of `grTexDownloadMipMapLevelPartial`, following
[FEATURES.md](../FEATURES.md). Launch `./run.ps1` or `run.cmd`;
rebuild with `./build.ps1`.

A CPU fire simulation produces a 128x128 RGB565 image. Two GPU copies display
the same image: one receives full texture uploads, the other receives only the
inclusive span of changed rows. The default split view compares them directly.
No palette data is uploaded; the heat-to-colour lookup runs on the CPU.

| Key | Action |
| --- | --- |
| B | Full fire / controlled bottom band |
| G | Controlled band height: 16 / 32 / 64 rows |
| C | Split comparison / single view |
| U | Select full-upload / partial-upload single view |
| S | Swap comparison sides |
| Space | Pause/resume automatic simulation |
| N | Advance one simulation step, including while paused |
| + / - | Simulation rate, 0-60 steps/sec |
| T | Bilinear / nearest filtering |
| Z | Whole image / enlarged lower crop |
| R | Reseed simulations and restore defaults |
| Tab | Frame profiler with FPS and frame-time graphs |
| Esc | Exit |

Rendering runs at 60 Hz; simulation defaults to 30 steps/sec. Both GPU copies
are updated even in single view. Frames without a simulation step or explicit
scenario change send no fire texture data. Switching scenarios uses cached,
prewarmed states; R explicitly rebuilds them.

## Row uploads

Each level occupies **32,768 bytes**, with **256 bytes per row**. The GPU bases
are 0 and 32,768, both 8-byte aligned and contained within the 2 MB TMU.
The partial call keeps its GPU address at the start of the mip level and passes
`pixels + first_row * 128` as its CPU data pointer. Its start/end rows are
inclusive. This pointer convention differs from partial palette downloads.

The demo compares every final RGB565 row with the previous uploaded image.
It uploads the first-through-last changed row, including any unchanged gaps.
Uploading only the fuel row would leave stale fire above it. Controlled-band
simulation confines steady updates to the selected bottom rows; changing modes
can require a larger upload to clear the previous image.

Representative verified updates:

| Simulation | Changed-row span | Partial payload | Full payload |
| --- | --- | --- | --- |
| Full fire | 59-127 | 17,664 B | 32,768 B |
| Bottom 16 | 112-127 | 4,096 B | 32,768 B |
| Bottom 32 | 100-127 | 7,168 B | 32,768 B |
| Bottom 64 | 81-127 | 12,032 B | 32,768 B |

HUD counters measure texture payload, including initial uploads, resets and
scenario synchronization. They do not measure PCI-bus or emulator-host traffic.
Font/profiler texture updates are separate. Simulation, colour conversion and
fire uploads are included in the profiler's Input stage.

The CPU caches heat/background colour combinations. A tracked first hot row
allows simulation to skip cold rows that cannot receive heat this step; the
destination prefix is cleared to prevent stale double-buffer data. Upload
coverage is still determined from the complete final RGB image.

See the [API Reference](../Glide_Reference_Manual_2.4_199707.pdf), printed p. 93,
and [Programming Guide](../Glide_Programming_Guide_2.4_199707.pdf), pp. 104-108.
All artwork is generated in C; no reference photographs are stored.

## Validation

`./verify.ps1` passes on native OpenGL and software Voodoo in DOSBox-X
2026.01.02. Tests read all 16,384 texels of each GPU image and compare them
with the CPU image and each other. Full/partial GPU images match exactly;
CPU comparisons allow one RGB565 rounding level.

Checks cover single rows 0/127, ranges 7-12, 31-32 and 63-64, the complete
0-127 range, untouched rows, inclusive payload counts, all four simulations,
pause/manual stepping, seeded repeatability, aligned allocations and the
CPU cold-prefix invariant. Native captures and runtime logs are ignored;
software tests preserve native captures. Visual review remains pending.

`./bench.ps1` measures 600 frames through full fire and all three bands,
30/60 simulation Hz, split/single views, filtering, cropping and profiler
display. Full fire is exercised at 60 simulation Hz. Result: **59.96 Windows-clock
FPS**, maximum guest frame **17.226 ms**, **zero frames over 18 ms**.
Full uploads sent **14,811,136 bytes** versus **4,970,240 bytes** through the
partial path, with **zero palette bytes**. Explicit R rewarming is outside
this steady-state benchmark.

Target: native emulated Voodoo Graphics, 640x480 at 60 Hz, 2 MB framebuffer,
one 2 MB TMU, `voodoo_maxmem=false`, `glide=false`. Total texture allocation,
including font/profiler, is **204,800 bytes**. Interactive FPS counts completed
refresh-paced frames; the benchmark also measures actual Windows-clock time.
