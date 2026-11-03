# 3dfx feature checklist and demo roadmap

Our initial target is **Voodoo Graphics (Voodoo 1), DOS, Glide 2.43**, running
in DOSBox-X. This is a growing checklist of capabilities to explore one demo
at a time. Features of later cards belong in a separate target group.

Hardware support, Glide support, and correct emulation are separate questions.
An entry marked supported still needs a demo on our configured emulator.

## Anti-aliasing and shaders

**Anti-aliasing: yes, for primitive edges.** Glide provides `grAADrawPoint`,
`grAADrawLine`, and `grAADrawTriangle`, plus polygon helpers. These use pixel
coverage and alpha blending to soften edges. Triangle edges can be enabled
individually: shared internal edges should not automatically receive the
same treatment as an object's silhouette. Alpha state and drawing order
matter. See the Programming Guide, printed pp. 34–37.

This is not modern full-scene MSAA. Voodoo 1 has no general full-scene AA
switch. Bilinear filtering and mipmapping address texture aliasing rather
than polygon silhouettes. Later Voodoo hardware's full-scene AA is a
separate subject and is outside this Voodoo 1 checklist.

**Programmable shaders: no.** Our target has no vertex/pixel shader programs,
GLSL, or HLSL. Its color, alpha, and texture combiners expose predefined
operations such as multiplication, addition, and interpolation. We can
combine these with CPU calculations and multiple rendering passes to make
effects, but cannot upload arbitrary per-vertex or per-pixel programs.

The CPU transforms and projects vertices, computes lighting, and handles
3D clipping. Glide accepts screen-space vertices; Voodoo rasterizes and
textures the resulting primitives. Gouraud shading interpolates supplied
vertex colors; it does not calculate a lighting model for us.

## First demos

Only check an item after its comparison has been reviewed. The existing cube
exercises Gouraud shading, depth testing, dithering, double buffering and
refresh pacing, but does not independently establish every setting's behavior.

| Done | Demo | Feature and support | What to demonstrate |
| --- | --- | --- | --- |
| [x] | [`cube`](cube/README.md) | Basic triangle rendering and smooth color interpolation | Rotating rainbow cube, hidden surfaces, readable FPS; baseline passed in DOSBox-X. |
| [ ] | `anti-aliasing` | Edge AA: supported through `grAADraw*` | Toggle AA on slow rotating lines and a triangle; then compare a cube silhouette without introducing seams on shared edges. |
| [ ] | `textures` | Perspective-correct texture mapping | Checkerboard on a tilted quad; CPU-generated affine version beside Glide's perspective-correct result. |
| [ ] | `filtering` | Nearest and bilinear filtering | Magnify the same checkerboard with both filters; compare pixel edges and blur. |
| [ ] | `mipmaps` | Mipmapping and LOD bias | Receding checkerboard floor, mipmaps on/off, colored mip levels, adjustable LOD bias; inspect shimmer and transitions. |
| [ ] | `blending` | Alpha and additive blending | Overlapping translucent quads and glowing particles; compare blend factors and drawing orders. |
| [ ] | `depth` | Z and W buffers | Intersecting geometry; compare depth off/on, Z/W modes, comparison functions and depth writes. |
| [ ] | `fog` | Fog color and lookup table | Objects receding into fog; compare table-based depth fog and vertex-alpha fog. |
| [ ] | `combiners` | Fixed-function color/alpha operations | One textured object with replace, modulate, add and interpolate modes; label the equation used. |
| [ ] | `texture-combine` | Two-texture effects; TMU-dependent | Base texture plus lightmap; compare multiple passes with a single pass when two TMUs are available. |

These are proposed folder names, not demos that already exist. Start with
`anti-aliasing`, then work down the list; each demo should remain runnable
on its own.

## Remaining feature inventory

Split these into individual demos as we reach them.

| Done | Feature | Support and limits | Proposed test |
| --- | --- | --- | --- |
| [ ] | Flat versus Gouraud shading | Supported; colors/lighting supplied by CPU. | Identical geometry with constant, face and vertex colors. |
| [ ] | Points, lines and convex polygons | Supported drawing functions; general meshes use triangles. | Primitive gallery, winding and polygon triangulation. |
| [ ] | Backface culling | Supported via `grCullMode`; winding depends on screen origin. | Toggle culling and origin on a labeled cube. |
| [ ] | Clip rectangle | `grClipWindow` limits pixel writes; this is not hardware 3D frustum clipping. | Move triangles through a visible clip rectangle. |
| [ ] | CPU near-plane clipping | Application responsibility. | Move a textured triangle across the camera's near plane without exploding coordinates. |
| [ ] | Alpha testing | Supported threshold/comparison test, distinct from blending. | Cutout fence/foliage with clean depth writes. |
| [ ] | Chroma key | Supported color-based rejection. | Sprite with a keyed background; compare with alpha testing. |
| [ ] | Texture wrapping and clamping | Supported independently for S and T. | Coordinates outside the texture boundaries. |
| [ ] | Texture sizes and aspect ratios | Power-of-two sizes, up to 256×256; supported rectangular ratios through 8:1 and 1:8. | Texture-size/aspect gallery and boundary cases. |
| [ ] | Texture formats | Multiple RGB, alpha, intensity, palette and YIQ formats; conversion/upload required. | Same image in several formats, including palette/table changes. |
| [ ] | Mipmap blending and trilinear filtering | One TMU needs two passes; two TMUs permit one pass. | Compare bilinear mipmapping, dithered LOD transitions and trilinear blending. |
| [ ] | Detail textures | Supported using texture combining; pass count depends on TMUs. | Surface gaining a fine detail pattern as it approaches. |
| [ ] | Projected textures | CPU supplies suitable coordinates; combiners/passes apply the texture. | Moving projected spotlight on a wall. |
| [ ] | Environment mapping | Effect built from CPU texture-coordinate generation and normal texturing. | Reflective-looking sphere; distinguish approximation from ray-traced reflections. |
| [ ] | Multipass lighting and highlights | Effects assembled from CPU lighting, combiners and blending. | Lightmap and specular-highlight comparisons. |
| [ ] | Dithering and color precision | Voodoo 1 color output is 16-bit; dithering reduces visible banding. | Smooth gradients with dithering off, 2×2 and 4×4; inspect screenshots closely. |
| [ ] | Gamma control | Glide exposes gamma correction; emulator/output behavior needs checking. | Grayscale and color ramps at several gamma settings. |
| [ ] | Destination alpha | Alpha buffering is supported, but competes with depth/triple buffering for auxiliary storage. Source-alpha blending does not itself require a destination-alpha buffer. | Compare ordinary transparency with a destination-alpha compositing case. |
| [ ] | Buffering and presentation | Front/back buffers, swaps and retrace queries; available buffer counts depend on memory/mode. | Compare double/triple buffering where available; measure paced frames and latency. |
| [ ] | Color/depth write masks and clears | Supported state controls. | Preserve color while changing depth and vice versa. |
| [ ] | Depth bias | Supported via `grDepthBiasLevel`. | Coplanar decal with bias off/on; inspect z-fighting. |
| [ ] | Linear framebuffer access | Read/write/lock functions expose buffers with format/stride constraints. | Upload an image, read it back, and check colors, stride and origin. |
| [ ] | Texture memory management | Explicit per-TMU downloads and address ranges. | Allocate several textures, switch them, and handle memory exhaustion cleanly. |
| [ ] | State save/restore | Glide exposes render-state capture/restore. | Switch between two scenes without leaking fog, alpha or texture settings. |
| [ ] | Hardware/performance queries | Query board/TMU configuration and available counters. | Display detected hardware, texture memory and available statistics. |

## Boundaries and later targets

| Feature | Voodoo 1 / our current target |
| --- | --- |
| Programmable vertex, pixel or compute shaders | No. Demonstrate combiners instead. |
| Hardware transform and lighting | No. Transform, project and light on the CPU. |
| Native stencil buffer | No. The guide describes a restricted depth-buffer masking workaround, not a full stencil buffer (printed p. 75). |
| Full-scene AA / modern MSAA | No native Voodoo 1 facility. Keep later-card AA experiments separate from primitive-edge AA. |
| Native modern normal mapping, shadow mapping or deferred shading pipelines | No ready-made pipeline or shader stage. Any approximation needs a specific CPU/fixed-function/multipass design. |
| Later Voodoo 2, Banshee, Voodoo 3 and VSA-100 features | Separate future targets; do not infer support from DOSBox-X's host GPU. |

Our baseline sets `voodoo_maxmem=false`: the standard 4 MB configuration,
with 2 MB for framebuffer/depth storage and 2 MB for one TMU's textures.
The cube's run, test and benchmark configs all use this setting.
Query the actual configuration in each new demo. For TMU-dependent features,
verify the one-TMU/multipass implementation first. An optional, explicitly
labeled expanded test may enable `voodoo_maxmem=true` (12 MB, two TMUs),
but that configuration is not our baseline retail Voodoo 1.

## What counts as tested

- Keep a visible toggle or side-by-side control comparison for the feature.
- Record the expected result, Glide calls, relevant render state and limitations.
- Record DOSBox-X version, backend, resolution, runtime and detected TMUs.
- Save an image when useful; include a moving test for temporal aliasing.
- Measure completed refresh-paced frames. Do not confuse render-loop speed
  with the host monitor's presented frame rate.
- Mark emulator results as such; an emulator failure alone does not prove
  that original 3dfx hardware lacks the feature.
- Update this checklist with the demo link and result once reviewed.

## Sources

- [Glide Programming Guide 2.4](Glide_Programming_Guide_2.4_199707.pdf):
  printed pp. 1–6 (pipeline), 27–37 (primitives/AA), 39–46 (color),
  49–60 (alpha/depth), 63–75 (fog/tests/stencil workaround),
  77–116 (textures/TMUs/memory), and chapters 11–12 (buffers/housekeeping).
  Some internal headings still say “Glide 2.2”; the document cover identifies 2.4.
- [Glide API Reference 2.4](Glide_Reference_Manual_2.4_199707.pdf):
  exact parameters and state interactions for the functions listed above.
- [Vendored SDK headers](vendor/glide/Glide/Src/Sst1/include/glide.h):
  declarations and constants used by our builds.
- [DOSBox-X Voodoo guide](https://dosbox-x.com/wiki/Guide:Setting-up-3dfx-Voodoo-in-DOSBox%E2%80%90X):
  emulated hardware, memory/TMU configurations and backend options.
