# Alpha-masked specular paint

C89 / Glide 2.43 demo of the `specular-paint` experiment in
[FEATURES.md](../FEATURES.md). Run `./run.ps1` or `run.cmd`;
rebuild with `./build.ps1`.

A moving highlight crosses a curved metal panel with a chipped blue paint
stripe and painted 3DFX lettering. The default split compares an unmasked
highlight on the left with matte paint and shiny exposed metal on the right.
Both views share the same prepared geometry, texture and CPU specular values.

| Key | Action |
| --- | --- |
| E | Paint-mask selector on/off |
| G | Specular highlight on/off |
| C / S | Split / single view; swap comparison sides |
| M | Source mask preview: white paint, black exposed metal |
| N | Gloss exponent: 8, 16, 32, 64 |
| + / - | Highlight strength from zero to 150 |
| L | Move / hold the light |
| F | Automatic panel orbit on/off |
| Left drag | Manual panel orientation |
| Right drag | Zoom |
| Space | Pause/resume light and orbit animation |
| R | Reset the scene |
| Tab | Frame profiler with FPS and frame-time graphs |
| Esc | Exit |

Higher gloss exponents make the vertex-computed highlight narrower. With E
disabled the highlight crosses paint too. G or zero strength isolates the
unchanged base texture. Paint and chips are original procedural texture data.

## One-pass mechanism

The [Glide Reference](../Glide_Reference_Manual_2.4_199707.pdf), printed p. 16,
describes using `grAlphaControlsITRGBLighting` for specular paint. This uses
the same selector as [self-lit-city](../self-lit-city/README.md), with an
additive equation and **constant black**:

| ARGB1555 alpha bit | Local RGB | Result |
| --- | --- | --- |
| 0: bare metal | Iterated vertex specular S | clamp(texture + S) |
| 1: paint | Constant black | texture + 0 |

`grColorCombine` uses SCALE_OTHER_ADD_LOCAL, factor ONE, local ITERATED and
other TEXTURE. The alpha-controlled selector overrides local RGB per texel.
Final alpha is constant opaque, alpha testing is disabled, blending is ONE/ZERO,
and ordinary Z tests/writes stay enabled. Alpha-zero metal remains solid.
Every panel cell is drawn once per view; the mask adds no second illumination
pass, framebuffer readback or texture upload. Split mode renders two views.

The CPU computes a Blinn highlight from the transformed smooth normal N,
point-light direction L and view direction V. With H = normalize(L+V),
S is proportional to max(dot(N,H),0)^exponent, suppressed for backlighting.
The permitted power-of-two exponents use repeated squaring. Static mesh normals
and UVs are cached, and transformed positions/specular RGB are shared by both
views. The 20x12 curved grid provides 273 vertices and 240 planar cells.

This is vertex-computed specular interpolated across polygons, not per-pixel
Phong shading, normal mapping or a general physically based material. Base
color is baked into the texture: this particular single-pass combiner adds
specular rather than also multiplying the base by dynamic diffuse lighting.
The paint mask is one bit; ordinary rendering uses bilinear texture sampling,
while the mask preview uses point sampling to show its binary source.

## Validation

`./verify.ps1` runs native OpenGL and software Voodoo controls. Both pass on
DOSBox-X 2026.01.02:

- 15 additive selector probes: both alpha bits, zero/intermediate/saturating
  specular RGB, enabled/disabled selection, nonblack constant and state restoration.
- Repeated-squaring specular matches a `pow` reference at all 273 vertices
  across all four gloss exponents.
- A point-sampled scene isolates paint from metal: over 35,000 paint pixels
  remain **exactly unchanged** by the masked highlight, while more than 28,000
  bare-metal pixels change. Disabling the mask makes painted pixels change too.
- The same 240 cell draws with mask on/off, swapped-view interiors, repeatable
  fixed poses, moving-light differences and finite projection.

The native swap comparison has one pixel differing by one RGB565 green level;
software has none. The check allows at most 16 pixels differing by one encoded
channel level and rejects larger differences. The outermost viewport columns
are excluded, as in the other labs. Native captures and logs are ignored;
software checks read the framebuffer without overwriting native images.

`./bench.ps1` measures 600 moving light/orbit frames across split/single,
mask on/off/preview, highlight on/off, gloss and strength, with the profiler
shown for the second half. This machine measures **59.82 Windows-clock FPS**,
maximum guest frame **17.205 ms**, **zero frames over 18 ms**, and at most
**480 cell draws** across both comparison views.

Target: native emulated Voodoo Graphics, 640x480 at 60 Hz, 2 MB framebuffer
and one 2 MB TMU, `voodoo_maxmem=false`, `glide=false`. Texture allocation is
**172,040 bytes**, including probe, font and profiler textures. FPS counts
completed refresh-paced frames against RTC seconds. Visual review remains pending.
