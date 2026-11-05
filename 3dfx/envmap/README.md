# Diving helmet / environment mapping

The copper diving helmet with reflections of a procedural room's windows and
skylight. Copper, brass and steel use different reflection tints. The original
material grain, smooth mouse controls, turntable, fitting LODs and native edge
AA carry over. The original [helmet demo](../helmet/README.md) remains separate.

Run `./run.ps1` or `run.cmd`; rebuild with `./build.ps1`. This is native C89 /
DOS / Glide 2.43, using the shared vendored SDK/runtime and Open Watcom setup.
The configs retain 640x480 at 60 Hz, fixed 100000 cycles/ms and standard Voodoo
1 memory: 2 MB framebuffer, one 2 MB TMU, `voodoo_maxmem=false`, passthrough off.

| Input | Action |
| --- | --- |
| E | Environment reflections on/off |
| + / - | Increase/decrease reflection strength by 10, clamped to 0..100 |
| A | Native silhouette edge AA on/off |
| Q | Fast / original Detail geometry |
| T | Material textures / plain material colors |
| Tab | Frame profiler with an ENVMAP row and FPS/frame-time graphs |
| Left drag / arrows | Smooth turn; drag release coasts |
| Wheel / right drag | Smooth zoom |
| Space | Pause/resume turntable |
| R / middle click | Reset view |
| Esc | Exit |

Reflection strength starts at 60. Zoom uses the wheel/right drag here; the
original helmet keeps its keyboard zoom controls. Pause before comparing E,
A or different strengths at a fixed pose.

## Fixed-function sphere mapping

The CPU generates a 256x256 RGB565 sphere map of a simple room with windows and
a skylight. The map contains incident highlights; ambient surface lighting is
already supplied by the base material pass. No reference photograph is copied
or added as an asset, and building requires no image-generation service.

For a distant viewer along +Z, a front-facing unit normal N reflects the view
vector as `R = 2*N*N.z - (0,0,1)`. Classic sphere mapping reduces to
`S = 127.5*(1+N.x)`, `T = 127.5*(1-N.y)` on that hemisphere. The production
path transforms each shared normal group once per frame, then reuses its UVs
at matching vertices. The view and room stay fixed while the helmet turns.

An additional pass on the same TMU adds the environment texture with copper,
brass or steel tint, scaled by the selected strength. Glass and black fittings
retain their existing appearance. Base depth is reused with GEQUAL and no
depth writes. Texture coordinates remain perspective-correct, and reflection
vertices are prepared lazily once per visible vertex per frame.

Both base and environment passes use `grAADrawTriangle` at silhouette/boundary
edges. Source alpha supplies native edge coverage in the additive reflection
pass. Interior triangles use normal draws. This is primitive-edge AA, not
full-scene MSAA. There are no shaders, reflection rays or dynamic scene capture:
the environment is a static distant-room approximation, distinct from the
[graveyard's planar geometry reflections](../reflect/README.md).

## Geometry, memory and performance

To leave room for the second antialiased pass, this demo's Fast mesh uses
12 segments around the dome/breastplate, up to 10 around torus fittings, four
torus sides and four cylinder sides. It contains 1,206 vertices / 1,484 faces
nearby and 1,196 vertices / 1,468 faces at the distant fitting LOD. The original
helmet's Fast mesh is unchanged. Q restores its original 4,056-vertex /
5,784-face Detail model; Detail does not target 60 FPS.

The environment adds 131,072 bytes of texture data and the cached HUD adds
32,768 bytes. Total texture allocation is **696,320 bytes**, within one 2 MB
TMU. The HUD draws cached string quads and updates changed atlas rows only;
profiler rasterization and texture uploads are split across frames.

The final moving AA benchmark measured **59.86 Windows FPS** over 1,440 frames,
with a **17.212 ms** maximum guest frame and **zero frames over 18 ms**. It
includes two full orbits, pitch/zoom changes, LOD transitions and the profiler
throughout the second orbit. The Windows interval includes two warmup frames.
These are measurements of the configured DOSBox-X OpenGL backend, not original
Voodoo card throughput or host-compositor timestamps.

## Checks

`./verify.ps1` checks Fast and Detail scene captures, the profiler, normal-cache
UVs against the full reflection-vector equation, near/far LODs and 1,000 cached
mesh switches. Five additional pixel controls run on both OpenGL and software
Voodoo: additive equal-depth composition, foreground rejection, preserved
depth, alpha-scaled contribution and perspective-coordinate consistency.

Four scene comparisons cover a second angle, AA off and the distant model.
Zero strength must match E off exactly, added reflections must not darken the
base, and increasing strength must increase or preserve channel values,
allowing one framebuffer quantization step. Captures include `ENVON.PPM`,
`ENVOFF.PPM`, `ENVSIDE.PPM`, `ENVFAR.PPM` and `PROFILER.PPM`.

`./soak.ps1` performs the full Windows-timed AA benchmark and requires at least
59.5 Windows FPS and no guest frame over 18 ms. `stress.conf` runs a shorter
240-frame version of the same two-orbit motion for development. Runtime copies,
captures, logs and markers are ignored; the executable and shared vendored
tools remain in Git.
