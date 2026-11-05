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
| [x] | [`sectorforth`](sectorforth/README.md) | Native language binding to Glide | 32-bit assembly Forth compiles definitions that populate RGB vertices and draw the classic triangle; interpreter and framebuffer checks passed in DOSBox-X. |
| [x] | [`sectorlisp`](sectorlisp/README.md) | Lisp binding to Glide | 32-bit DOS C-reference port evaluates a Lisp-defined RGB triangle; recursive evaluation, persistent definitions, garbage collection and three graphics visits pass in DOSBox-X. |
| [ ] | [`helmet`](helmet/README.md) / anti-aliasing | Textured diving-helmet silhouette AA demo implemented in C. | Fixed-pose AA on/off framebuffer comparison and front/side visual checks pass in DOSBox-X. Review interactively before completing this item; near-horizontal lines and a minimal triangle control are still follow-up checks. Shared internal mesh edges are excluded. |
| [ ] | [`textures`](textures/README.md) / Winchester Cathedral | Perspective-correct versus affine texture mapping | C exterior model, orbit and smooth drone route; toggle native perspective/affine texture W or compare side by side, with optional checkerboard. Same-camera framebuffer and clipping/path checks implemented; interactive comparison awaits review. |
| [ ] | [`filtering`](filtering/README.md) | Nearest and bilinear filtering | Standalone checkerboard/stone lab with identical sample coordinates, split/single views, side swapping, magnify/minify, oblique perspective and slow UV motion. Native OpenGL passes analytic four-texel and independent min/mag controls. Software passes ordinary comparisons but selects the magnification setting in the single-level minification fixture; explicitly reported as a backend limit. 600 moving frames: 59.86 Windows FPS, max 17.202 ms, zero over 18 ms. Visual review pending; mipmapping deliberately disabled. |
| [ ] | [`mipmaps`](mipmaps/README.md) / cathedral | Mipmapping and LOD bias | Native off/on comparison, nine colored levels, bias and nearest/dither toggles. Software Voodoo framebuffer checks pass; OpenGL regenerates host mips and fails the custom-level/bias diagnostic on DOSBox-X 2026.01.02. Review moving shimmer/transitions interactively. |
| [ ] | [`blending`](blending/README.md) / candlelit nave | Alpha and additive blending | Stained glass, candle glows, colored floor patterns and mist, with independent toggles and reversed-order control. Six pixel controls pass on OpenGL and software Voodoo; scene comparisons and tour implemented. Review visually before completing the item. |
| [ ] | `depth` | Z and W buffers | Intersecting geometry; compare depth off/on, Z/W modes, comparison functions and depth writes. |
| [ ] | [`fog`](fog/README.md) / nave to graveyard | Fog color and lookup table | Reuses the candlelit nave, with an open doorway and a continuous tour among headstones and trees. Linear/exp/exp2 table fog, density/color controls, alpha and additive attenuation. Native pixel controls pass on OpenGL and software Voodoo; interactive review and vertex-alpha comparison remain. |
| [ ] | `combiners` | Fixed-function color/alpha operations | One textured object with replace, modulate, add and interpolate modes; label the equation used. |
| [ ] | [`lightmap`](lightmap/README.md) / nave and graveyard | Multipass lightmapping on one TMU | Independent base/light UVs, equal-depth multiplication, B on/off, warm candle pools, stained-glass floor patterns and moonlit ground shadows. Fog resolves once in a third raster pass; native PREFOG_COLOR works in software Voodoo but fails in OpenGL. Pixel/reference checks implemented; interactive review pending. |
| [ ] | `texture-combine` | Two-texture effects; TMU-dependent | Base texture plus lightmap; compare multiple passes with a single pass when two TMUs are available. |
| [ ] | [`project`](project/README.md) / moving lantern | Projected textures with independent projector W | Lantern/grid projected across the nave and gravestones over existing lightmaps. P toggle, O held-world projector, X diagnostic grid; native camera fog remains separate from projector Q. One additional additive pass on a single TMU; pixel and moving-camera checks implemented, interactive review pending. |
| [ ] | [`shadows`](shadows/README.md) / lantern ground shadows | CPU planar silhouettes and native depth-only masks | Pillars, headstones and crosses block the moving lantern on floor, grass and path. S comparison, overlapping-mask union, preserved baked lighting and foreground depth. OpenGL/software pixel controls and nave/graveyard comparisons implemented; interactive review pending. No stencil buffer, shadow texture or extra TMU allocation. |
| [ ] | [`reflect`](reflect/README.md) / graveyard puddle | Planar reflections with CPU clipping, independent raster depth and alpha blending | Mirrored coarse scenery inside an eight-sided water portal, native foreground depth rejection, fog and water tint. R comparison, preserved scene depth, bounded reflection scene and a separate profiler row. No stencil buffer, offscreen target or extra TMU allocation; interactive review pending. |
| [ ] | [`envmap`](envmap/README.md) / metallic diving helmet | CPU sphere-map coordinates and a second single-TMU reflection pass | Procedural room windows/skylight, material tints, E toggle and +/- strength. Native edge AA in both passes, shared-normal UV cache and 60 Hz Fast mode. Full-vector UV reference, monotonic-strength and depth controls implemented; interactive review pending. |
| [ ] | [`detail`](detail/README.md) / nearby masonry | Detail textures with independent tiling and distance fade | ALPHA8 grain modulates lit stone, with a fog-color correction on the single TMU. D comparison, strength/tiling controls, conservative group rejection and close pier flybys. Fog/depth controls and exact collection comparisons implemented; interactive review pending. |
| [ ] | [`foliage`](foliage/README.md) / windy graveyard | Alpha-tested cutouts, CPU wind and bounded leaf particles | Branch sprays, grass and ivy; compare depth-writing cutouts, sorted blending and solid cards. Traveling gusts bend plants and lift settled leaves. Adjustable 0-128 leaf count, direct walk/fly controls and profiler. Native OpenGL/software alpha/depth/fog checks pass. At 128 leaves, the 6,000-frame tour measures 59.94 Windows FPS, max 17.213 ms, zero over 18 ms (stone detail off); interactive review pending. |
| [ ] | [`water`](water/README.md) / wind-rippled puddle | Animated world-space UVs and two alpha-blended samples on one TMU | One shared 32x32 ARGB4444 tile (2 KiB) with mottled green patches and branching bright ripples. Opposing horizontal offsets and common downwind flow accelerate with gusts. Bounded planar reflections include low slab undersides and animated grass, leaves and tree proxies. V still/rippled comparison, strength control, 0-128 leaves and 13-stage profiler. Native/software pixel controls and contact/scene tests pass. 600-frame close-puddle test at 128 leaves: 59.67 Windows FPS, max 17.203 ms, zero over 18 ms; interactive review pending. |
| [ ] | [`rain`](rain/README.md) / rainy graveyard | Wind-driven weather, animated impact sprites and bounded particle pools | J/K selects off/light/medium/heavy rain, up to 96 world-space drops and 16 impact sprites. Exposed puddle hits create expanding rings and brief splashes; gusts slant streaks and increase water roughness. One shared 1 KiB ALPHA8 ring tile, alpha blending, foreground depth rejection, shoreline clipping and distance fades. Uses the water scene, walk/fly/tour controls, adjustable leaves and Tab graphs. Native/software controls and performance checks are documented in the demo README; interactive review pending. |

Entries without links are proposed folder names, not demos that already
exist. Continue the anti-aliasing comparisons in `helmet`, then work down
the list; each demo should remain runnable on its own.

For mipmap correctness, use the dedicated demo's software Voodoo config.
Our OpenGL-backed demos remain useful for other mechanisms, but that backend
does not establish native mip-chain, LOD-bias or dither behavior. The mipmap
demo's Windows-clock benchmark also separates actual host FPS from guest time.

## Effects and experiments found in the manuals

These extend the basic tests into visual demos. All remain untested on our
standard 4 MB, one-TMU target. The API mechanisms below are documented;
the scenes and comparisons are our proposed applications of them.
Page references are the manuals' printed page numbers, not PDF page indexes.

| Done | Proposed demo | What makes it interesting | Mechanism, limits and source |
| --- | --- | --- | --- |
| [ ] | `self-lit-city` | Fly past buildings: walls get darker as the light moves, but windows and signs stay bright. Toggle the mask to see the difference. | `grAlphaControlsITRGBLighting` uses the **most significant texture-alpha bit** to select vertex RGB or a constant color. With the constant set to white and multiplicative texture lighting, selected texels bypass vertex lighting in the same pass. This is self-illumination, not bloom or a light cast onto nearby objects. Reference p. 16. |
| [ ] | `specular-paint` | A moving highlight crosses a painted metal panel, appearing on exposed metal but disappearing on matte painted regions. | The same alpha-controlled lighting mode can mask the **addition** of vertex-computed specular RGB. Use a black constant for regions without highlights. The CPU computes the highlight; this is not per-pixel Phong shading or normal mapping. Reference p. 16; Guide pp. 45–46. |
| [ ] | `palette-lava` | Animate lava, flowing color bands, or water on a textured object without re-uploading its texels. Show a palette strip and count upload bytes. | Upload a `P_8` texture once, then rotate selected palette entries with `grTexDownloadTablePartial`. Each TMU has only one palette, so every indexed texture using it sees that palette change. Palette-entry alpha is ignored; use a suitable texture format if texel alpha is needed. Guide pp. 114–115; Reference pp. 94–95. |
| [ ] | `procedural-fire` | A CPU-generated fire texture plays on a rotating cube or screen. Compare full uploads against a controlled animation where only a band of rows changes. | `grTexDownloadMipMapLevelPartial` updates an inclusive range of **whole rows**, not arbitrary rectangles. Keep the base address at the start of the mip level, with 8-byte alignment; do not cross a 2 MB texture-memory boundary. Generate pixels on the CPU. Updating only the bottom row is not enough if the fire simulation changes the rest. Reference p. 93; Guide pp. 104–108. |
| [ ] | `model-impostor` | Render a model into a small hidden image, then put that image on a camera-facing billboard. Compare live recapture, a cached image, and the original mesh. | Voodoo 1 has no direct framebuffer-to-texture render target: draw into a 128×128 region of the back buffer without swapping, read RGB565 pixels to system RAM with `grLfbReadRegion`, and upload them with `grTexDownloadMipMapLevel`. Clear the scratch color/depth before drawing the final scene. RGB565 readback has no alpha; start with a chroma-key background and nearest filtering, then investigate edge fringes. Measure readback/upload stalls; cache images or update less often. Reference pp. 62, 66, 92; this workflow is our composition of those APIs. |
| [ ] | `masked-window` | A scene appears through a circular or irregular window while the surrounding area stays hidden. Display the mask beside the result. | The guide describes writing nearest-depth values into the depth buffer to reject fragments in masked areas. Use a matching depth comparison and retain the mask while drawing; do not clear it away before rendering the scene. This consumes the depth buffer and is a limited masking trick, not native stencil operations or a general recursive portal system. Guide p. 75; Reference `grLfbLock`, pp. 59–61. |
| [ ] | `fogged-lightmap-room` | Render a textured room with a moving lightmap, then fade it into fog. Toggle a deliberately incorrect implementation that fogs each pass independently. | On one TMU, apply the textures in separate passes. Match fog/blend state to the intended additive or multiplicative composition; the manual supplies separate recipes using `GR_FOG_ADD2`, `GR_FOG_MULT2`, and `GR_BLEND_PREFOG_COLOR`. Repeatedly adding the fog color produces a visibly wrong result. Guide pp. 67–72. |
| [ ] | `fog-curves` | A corridor compares linear fog, exponential fog and exponential-squared fog. An optional custom table turns distance into colored-looking brightness bands against one fog color. | `guFogGenerateLinear`, `guFogGenerateExp`, and `guFogGenerateExp2` build the 64-entry fog table. A deliberately non-monotonic custom table is our experiment, not a stock fog model; one fog color and the table's nonuniform depth sampling limit the effect. Guide pp. 63–66; Reference pp. 114–117. |
| [ ] | `pixel-pipeline` | Send a CPU-drawn bitmap through depth tests and alpha blending so it can pass behind a 3D object. Compare direct framebuffer writes with pipeline-enabled writes. | `grLfbLock(..., pixelPipeline=FXTRUE, ...)` can process CPU pixel writes through the pixel pipeline. Check lock success: hardware/backend support and write format matter. Set constant alpha/depth before locking when the format lacks them. Honor the returned stride; call no unrelated Glide routines while locked. This does not enable texture mapping of arbitrary CPU writes or provide GPU render-to-texture. Reference pp. 57–61; Guide chapter 11. |
| [ ] | `texture-compression` | An image gallery compares RGB565, indexed color and 3dfx's YIQ/NCC compression, showing memory usage and enlarged color errors. | Prepare textures with the vendored TexUS tools. YIQ formats require an NCC decompression table on the same TMU; two NCC table slots can be switched without re-uploading the table every draw. This is legacy table-based compression, not DXT/S3TC. Guide pp. 95–99, 113–116; Reference pp. 94–95, 103, 106–107. |
| [ ] | `subpixel-lab` | Slowly move tiny triangles across a magnified pixel grid; compare coordinate snapping and inspect shared edges for artifacts. | The manual warns that floating-point setup can disagree with the hardware's 12.4 fixed-point coordinates. Compare explicit snapping to a **1/16-pixel grid**. Its fast add/subtract technique requires 24-bit x87 precision; save/restore FPU state and verify the implementation rather than blindly copying the example. Emulator artifacts may differ from hardware. Guide pp. 145–146. |
| [ ] | `overdraw-lab` | Layer opaque objects and cutout sprites; show which pixels are rejected by depth, alpha and chroma tests. Compare front-to-back with back-to-front opaque rendering. | Use `grSstPerfStats` and `grSstResetPerfStats`. Drain queued work before sampling. The hardware counters are only **24 bits**; reset or handle wrap. Clears and LFB writes affect `pixelsOut`, so raw output/input ratios are not a complete overdraw metric. Check emulator counter behavior before treating results as hardware measurements. Guide p. 136; Reference pp. 71, 74. |

Suggested placement in the sequence:

1. After `textures`/`filtering`, try `self-lit-city` first: a striking effect
   with a documented single-pass path on our one-TMU card.
2. After basic indexed textures and texture uploads, build `palette-lava`,
   then `procedural-fire`; keep palette changes separate from pixel changes.
3. After `combiners`, add `specular-paint` and the compression gallery.
4. After `depth` and framebuffer access, try `masked-window` and
   `pixel-pipeline`. Combine framebuffer readback with texture uploads for
   `model-impostor`; start with a cached capture before attempting live updates.
5. Combine `fog` and `texture-combine` into `fogged-lightmap-room`;
   extend the basic fog demo with `fog-curves`.
6. Use `subpixel-lab` and `overdraw-lab` to investigate correctness and
   performance once several scenes exist.

Do not merge all these effects into the cube: keep a small control scene
and an independent toggle for each mechanism.

## Remaining feature inventory

Split these into individual demos as we reach them.

| Done | Feature | Support and limits | Proposed test |
| --- | --- | --- | --- |
| [ ] | Flat versus Gouraud shading | Supported; colors/lighting supplied by CPU. | Identical geometry with constant, face and vertex colors. |
| [ ] | Points, lines and convex polygons | Supported drawing functions; general meshes use triangles. | Primitive gallery, winding and polygon triangulation. |
| [ ] | Backface culling | Supported via `grCullMode`; winding depends on screen origin. | Toggle culling and origin on a labeled cube. |
| [ ] | Clip rectangle | `grClipWindow` limits pixel writes; this is not hardware 3D frustum clipping. | Move triangles through a visible clip rectangle. |
| [ ] | CPU near-plane clipping | Application responsibility. | Move a textured triangle across the camera's near plane without exploding coordinates. |
| [ ] | Alpha testing | Supported threshold/comparison test, distinct from blending. | [`foliage`](foliage/README.md): branch sprays, grass, ivy and wind-blown leaves; cutout/blend/solid comparison with native depth controls. |
| [ ] | Chroma key | Supported color-based rejection. | Sprite with a keyed background; compare with alpha testing. |
| [ ] | Texture wrapping and clamping | Supported independently for S and T. | Coordinates outside the texture boundaries. |
| [ ] | Texture sizes and aspect ratios | Power-of-two sizes, up to 256×256; supported rectangular ratios through 8:1 and 1:8. | Texture-size/aspect gallery and boundary cases. |
| [ ] | Texture formats | Multiple RGB, alpha, intensity, palette and YIQ formats; conversion/upload required. | Same image in several formats, including palette/table changes. |
| [ ] | Mipmap blending and trilinear filtering | One TMU needs two passes; two TMUs permit one pass. | Compare bilinear mipmapping, dithered LOD transitions and trilinear blending. |
| [ ] | Detail textures | Texture combining and multipass compositing; pass count depends on TMUs and fog handling. | [`detail`](detail/README.md): stone porosity gains fine texture nearby, with strength, tiling and distance-fade comparisons. |
| [ ] | Projected textures | CPU supplies suitable coordinates; combiners/passes apply the texture. | Moving projected spotlight on a wall. |
| [ ] | Environment mapping | CPU sphere-map UVs and an additive single-TMU pass. | [`envmap`](envmap/README.md): metallic helmet, material tints, strength comparison and native edge AA; static distant-environment approximation. |
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
- Check interactions explicitly: texture alpha may serve transparency,
  edge coverage, or a lighting mask; these uses can compete for the same data.
  Likewise, a depth-buffer mask must coexist with the scene's depth test.
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
