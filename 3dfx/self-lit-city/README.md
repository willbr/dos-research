# Self-lit city

C89 / Glide 2.43 demo of **`grAlphaControlsITRGBLighting`**, following
the `self-lit-city` entry in [FEATURES.md](../FEATURES.md).
Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`.

Twelve procedural buildings share warm/cool window textures and neon 3DFX signs.
A moving CPU light shades their walls while the camera circles the city.
The default split shows ordinary vertex lighting on the left and the native
self-lit mask on the right. Both views use the same geometry, textures and light.

| Key | Action |
| --- | --- |
| E | Enable / disable alpha-controlled lighting |
| C / S | Split / single view; swap comparison sides |
| M | Show the source alpha mask instead of the self-lit view |
| + / - | Vertex-light strength, from zero to 100 |
| Space | Move / hold the CPU light |
| F | Automatic camera flypast on/off |
| Left drag | Manual camera orbit |
| Right drag | Zoom in/out |
| R | Reset the scene |
| Tab | Frame profiler with FPS and frame-time graphs |
| Esc | Exit |

Set light strength to zero to isolate the effect: ordinary walls and windows
go black, while masked windows and signs retain their texture colors. Space
holds the light independently of camera movement; F holds the camera path.

## Native single-pass mechanism

The [Glide Reference](../Glide_Reference_Manual_2.4_199707.pdf), printed p. 16,
documents the selector. With `grAlphaControlsITRGBLighting(FXTRUE)`, the most
significant texture-alpha bit overrides the color combiner's local RGB source:

| Texture alpha bit | Local RGB | Result in this demo |
| --- | --- | --- |
| 0 | Iterated vertex RGB | Wall texture multiplied by CPU lighting |
| 1 | Constant RGB, set to white | Window/sign texture retains its brightness |

The original 128x128 ARGB1555 textures store that bit alongside their color.
Roofs and ground use alpha-zero texels and remain ordinarily lit. The color
combiner multiplies texture RGB by local RGB. Output alpha is constant opaque;
alpha testing is disabled, blending is ONE/ZERO, and depth testing/writes remain
enabled. Thus alpha-zero walls remain solid rather than becoming cutout holes.

Each visible face is drawn once per view. Mask on/off does not add geometry,
texture uploads or a second illumination pass; split view draws the two
comparisons independently. This is self-illumination, not bloom or light cast
onto other surfaces. Texture alpha is dedicated to the lighting mask here.
The mask preview uses point sampling to display the original binary alpha.
HUD/background drawing explicitly disables the selector to avoid leaking it.

## Validation

`./verify.ps1` runs both native OpenGL and software Voodoo controls:

- Identical RGB texels with alpha bit zero/one, at zero, intermediate and full
  vertex light, with the feature enabled and disabled.
- A nonwhite constant, proving alpha-one texels select constant RGB while
  alpha-zero texels continue to follow vertex RGB.
- Disabling the feature restores ordinary lighting.
- Same draw count with mask on/off, visible scene differences, repeatable fixed
  poses, moving-light differences and zero interior mismatches after swapping.

Both backends pass on DOSBox-X 2026.01.02. Software Voodoo differs at the
outermost right clip-window column; the swap comparison excludes the one-pixel
outer borders. Local projected X coordinates snap to the hardware's 1/16-pixel
grid before adding viewport offsets, avoiding rounding differences between
the comparison halves. Native captures and logs are ignored; software tests
read the framebuffer without overwriting the native captures.

`./bench.ps1` measures 600 moving camera/light frames with split/single views,
mask on/off/preview and the profiler visible for the second half. On this machine:
**59.90 Windows-clock FPS**, maximum guest frame **17.208 ms**, **zero frames
over 18 ms**. The model has 61 faces; the benchmark draws at most 68 visible
faces across both comparison views. One fixed-view mask comparison draws
34 faces with either state.

Target: native emulated Voodoo Graphics, 640x480 at 60 Hz, 2 MB framebuffer
and one 2 MB TMU, `voodoo_maxmem=false`, `glide=false`. Total texture allocation:
**237,576 bytes**, including font, probe and profiler textures. Everything is
generated in C; no reference photographs, external assets or shaders are used.
The on-screen FPS counts completed refresh-paced frames against RTC seconds.
The checklist remains unchecked until the visual comparison is reviewed.
