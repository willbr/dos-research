# Palette-cycled lava

C89 / Glide 2.43 demo of `grTexDownloadTablePartial`, following `palette-lava`
in [FEATURES.md](../FEATURES.md). Run `./run.ps1` or `run.cmd`;
rebuild with `./build.ps1`.

The lava's indexed texels and UVs stay fixed. Rotating selected palette colours
makes hot bands travel through the procedural cracks. The second P8 image uses
the same palette and changes with it. The smaller RGB565 reference preserves
the startup colours and does not change. The palette strip exposes all 256 entries.

| Key | Action |
| --- | --- |
| Space | Pause/resume automatic cycling |
| N | Step the palette manually, including while paused |
| + / - | Palette speed, 0-64 entries per second |
| D | Reverse cycling direction |
| C | Partial-range / full-palette upload comparison |
| G | Active range 64-191 / narrower 96-159 |
| B | Bilinear / nearest filtering |
| Z | Full texture / fixed enlarged crop |
| R | Restore startup palette and default controls |
| Tab | Frame profiler with FPS and frame-time graphs |
| Esc | Exit |

At the default 24 steps/sec, the palette changes on roughly 24 of the 60 rendered
frames. Frames with no new palette step send no table data. Space freezes
automatic cycling; N, range/mode changes and R can still issue explicit uploads.
Changing to a narrower range leaves the other entries at their current colours.
R explicitly restores all entries with one full upload.

## Partial updates and counters

Three P8 textures upload once: lava 256x256, logo 256x128 and palette strip
256x32, totaling **106,496 indexed bytes**. Every P8 texture on TMU0 references
its one shared palette. Animated entries are 64-191 by default; rock and the
other guard ranges stay fixed. Source data, coordinates and texture allocations
do not change during palette cycling.

`grTexDownloadTablePartial(GR_TMU0, GR_TEXTABLE_PALETTE, &palette, first, last)`
takes the **whole palette pointer** and an **inclusive** entry range. A single
entry uses the same start/end index. The ordinary range sends 128*4 = **512 bytes**
per step; the narrow range sends **256 bytes**. Full mode sends **1,024 bytes**
for the identical resulting palette, without changing the texels either.

Default partial cycling sends about **12 KiB/sec** of palette payload, versus
24 KiB/sec for full updates at the same 24 steps/sec. The HUD reports palette
payload in the completed update step/current frame and cumulative payload,
including startup and reset uploads. It is not a PCI-bus or emulator-host traffic
counter. Font/profiler updates are separate from the fixed P8 source textures.
The two full-upload staging buffers avoid reusing a mutable table address.

The [API Reference](../Glide_Reference_Manual_2.4_199707.pdf), printed pp. 94-95,
and [Programming Guide](../Glide_Programming_Guide_2.4_199707.pdf), printed
pp. 114-115, describe palette downloads. Partial NCC uploads are not supported
by this API; this demo updates palette entries only. This is classic palette
animation, not changing geometry, texture scrolling or a fluid simulation.

All artwork is generated in C. A warped cellular field supplies the lava and
fixed rock indices. A periodic hot-colour ramp supplies the animated palette.
Output is opaque; palette-entry alpha is not a transparency control for P8.

## Validation

`./verify.ps1` runs native OpenGL and software Voodoo tests. Both pass on
DOSBox-X 2026.01.02:

- All 256 GPU palette codes after a single-entry update at 73, the unaligned
  inclusive range 71-78, endpoint updates at 0/255, and a full 0-255 partial call.
- Partial/full updates produce the same pixels with 512 versus 1,024-byte payloads.
- A 96-159 update preserves every other CPU/GPU palette entry.
- Paused cycling performs no uploads and produces repeatable frames.
- Cycling changes over 11,000 active lava pixels and about 2,900 logo pixels,
  with zero changes in static ranges or the frozen RGB565 reference.
- All three source index arrays retain their hashes and startup-only upload counts.

The static-range scene check uses nearest sampling to isolate each index.
Normal bilinear rendering can blend neighbouring animated/static colours at
their boundaries. Numeric colour probes allow small RGB565 output rounding.
Native captures and runtime logs are ignored; software tests do not overwrite
the native captures. Visual review remains pending on the roadmap.

`./bench.ps1` measures 600 cycling frames across partial/full uploads,
broad/narrow ranges, speed/reverse, filtering and crops, with the profiler
shown for the second half: **59.85 Windows-clock FPS**, maximum guest frame
**17.199 ms**, **zero frames over 18 ms**, **zero runtime P8 texture uploads**
and unchanged source hashes. Palette work is included in the Input stage.

Target: native emulated Voodoo Graphics, 640x480 at 60 Hz, 2 MB framebuffer
and one 2 MB TMU, `voodoo_maxmem=false`, `glide=false`. Total texture allocation
is **376,832 bytes**, including the frozen reference, font and profiler.
FPS counts completed refresh-paced frames against RTC seconds; the benchmark
also measures actual Windows-clock throughput.
