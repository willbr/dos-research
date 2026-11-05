# Texture formats and compression gallery

C89 / Glide 2.43 lab for `texture-compression` in [FEATURES.md](../FEATURES.md).
Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`.
The default uses **software Voodoo** for correct NCC table switching on
DOSBox-X 2026.01.02. OpenGL's table-cache limitation is tested separately below.

Two original diagnostic images contain warm/cool shaded metal, sharp 3DFX
lettering, colour steps, fine edges and grayscale ramps. Each image is encoded
by the vendored **TexUS 2.01**, with identical source dimensions, no dithering,
no mipmaps and statistical NCC compression. All inputs, six `.3df` outputs,
and regeneration scripts are committed here; no user photographs are stored.

| Key | Action |
| --- | --- |
| 1 / 2 / 3 | Select RGB565 / P8 / YIQ in single view |
| C | Three-format gallery / single view |
| T | Warm image A / cool image B |
| E | Texture / CPU-decoded absolute RGB error, amplified 4x |
| Z | Full image / enlarged 64x64-texel crop |
| Arrows | Pan the enlarged crop |
| Space | Automatic crop panning on/off while zoomed |
| V | Two resident NCC slots, showing both YIQ images together |
| N | Correct / deliberately wrong NCC slot |
| P | Correct / deliberately wrong shared palette |
| B | Nearest / bilinear filtering |
| R | Reset controls |
| Tab | Profiler with FPS and frame-time graphs |
| Esc | Exit |

E and T apply to the ordinary gallery; V always shows the two original YIQ
images. N/P affect the relevant compressed format, while error maps describe
the correct files' decoded data. The numerical errors compare decoded RGB
against the stored RGB565 reference; they do not measure a filtered screenshot.
MAE is the mean absolute channel error on a 0..255 scale, and MAX is its peak.
Statistics always describe the entire image, including while zoomed.

## Memory and table formats

Each 256x256 image has one mip level:

| Format | Texel storage | Required table |
| --- | --- | --- |
| RGB565 | 131,072 bytes, 128 KiB | None |
| P8 | 65,536 bytes, 64 KiB | 256 RGB palette entries, 1,024-byte upload |
| YIQ/NCC | 65,536 bytes, 64 KiB | One of two 48-byte packed NCC register tables |

The 8-bit formats halve texel storage relative to RGB565. Table storage is
separate from the TMU texture-address allocation. NCC's 48-byte register
payload differs from the `.3df` table fields (80 bytes) and the SDK's CPU
structure, which also contains decoded coefficients and packed words.

P8 maps each index to one palette entry. A TMU has one shared palette, so
changing between the two indexed images uploads the matching palette. Palette
entry alpha is ignored for P8; the test verifies this through texture-alpha blending.

YIQ stores four Y bits, two I bits and two Q bits per texel. Its NCC table
reconstructs RGB from Y plus signed I/Q contributions. This is legacy table-based
compression, not DXT/S3TC or video chroma subsampling. Both NCC tables are
uploaded once with `grTexDownloadTable`; `grTexNCCTable` selects a resident
slot before binding its texture. V and N demonstrate this without changing
the stored texture bytes or re-uploading the NCC tables.

## Assets and error measurements

`make-assets.py` generates the two original 24-bit TGA inputs using Python's
standard library. `assets.ps1` runs the vendored Win32 TexUS executable with
`-rn -mn -dn -ys`, selecting `rgb565`, `p8` and `yiq`. TexUS requires a lowercase
output extension; the script stages each output and checks that it was created.
Rebuild the committed assets with `./build.ps1 -RebuildAssets` or `./assets.ps1`.
Normal builds use the committed outputs and do not require Python or conversion.

The DOS program loads `.3df` data using `gu3dfGetInfo` / `gu3dfLoad`, uploads
each sample once, and decodes each format on the CPU for independent checks.
NCC I/Q coefficients are sign-extended from their nine-bit hardware form.
Four error-map textures are generated once at startup; reference error is black.

| Image | P8 MAE / MAX | YIQ MAE / MAX |
| --- | --- | --- |
| A, warm | 1.041 / 16 | 8.894 / 41 |
| B, cool | 1.504 / 90 | 10.395 / 46 |

These results describe these inputs and TexUS settings, not a universal quality
ranking. See the [Programming Guide](../Glide_Programming_Guide_2.4_199707.pdf),
printed pp. 95-99 and 113-116, and [Reference](../Glide_Reference_Manual_2.4_199707.pdf),
printed pp. 94-95, 103 and 106-107.

## Verification and emulator limits

`./verify.ps1` runs the strict software-Voodoo tests and an explicit OpenGL
diagnostic. Software passes:

- 6,144 image samples across both images and all three formats, compared with
  CPU-decoded RGB, allowing 16-bit output rounding.
- All 256 P8/YIQ table codes for both images, plus both NCC slots in one frame.
- Deliberately wrong palette/NCC bindings produce visibly different pixels;
  restoring the correct binding restores the image.
- P8 palette-alpha behavior, repeatable paused frames, and startup-only sample
  texel/NCC uploads. Error and table-gallery captures are generated as well.

OpenGL renders the static images correctly, but changing only the NCC slot
can retain a cached YIQ conversion. The diagnostic identifies the exact stale
table's 256 decoded colours, two views retaining one slot, and an unchanged
image after selecting the wrong slot. These are reported as **LIMIT**, with
unexpected errors still failing. `/test` remains strict on either backend;
`opengl.conf` runs `/openglcheck` and does not overwrite the software captures.
This is an emulator observation, not a limitation of the original card.

Native framebuffer captures and logs are ignored. Source TGA and TexUS `.3df`
assets are tracked. No runtime RGB fallback disguises the indexed/NCC formats.
The checklist remains unchecked pending interactive review.

`./bench.ps1` measures 600 panning frames across formats, images, errors,
table slots and filters, with the profiler for the second half. This machine
measures **59.85 Windows-clock FPS**, maximum guest frame **17.241 ms**,
**zero frames over 18 ms**. During the run: **zero NCC table uploads**, **zero
sample texel uploads**, and four required palette changes. Font/profiler updates
are separate. The clear fills the background directly to avoid redundant
full-screen software rasterization; comparison images retain independent zoom.

Target: native emulated Voodoo Graphics, software rasterizer, 640x480 at 60 Hz,
2 MB framebuffer and one 2 MB TMU, `voodoo_maxmem=false`, `glide=false`.
Total texture allocation is **1,188,352 bytes**, including samples, error maps,
test codebooks, font and profiler. FPS counts completed refresh-paced frames
against RTC seconds; the benchmark separately checks Windows-clock throughput.
