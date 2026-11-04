# 3dfx demos

Each demo lives in its own subfolder.

See the [feature checklist and demo roadmap](FEATURES.md) for capabilities,
limitations, and the next experiments to build.
See [project notes](notes.md) for proposed sectorforth and sectorlisp ports
with interactive Glide graphics.

- [cube](cube/README.md): rotating rainbow cube using DOS Glide, with a live
  FPS counter and 60 Hz frame pacing. Launch with `cube/run.cmd` or
  `./cube/run.ps1`.
- [helmet](helmet/README.md): textured C diving helmet inspired by the user's
  Winchester photograph, with silhouette-edge anti-aliasing, inertial mouse
  controls, and a Tab profiler with FPS/frame-time graphs.
  Launch with `helmet/run.cmd` or `./helmet/run.ps1`.
- [textures](textures/README.md): Winchester Cathedral exterior with smooth
  orbit controls, an automatic drone flight, perspective/affine texture
  comparison and the Tab profiler. Launch with `textures/run.cmd` or
  `./textures/run.ps1`.
- [mipmaps](mipmaps/README.md): cathedral mipmapping comparison, colored mip
  levels, LOD bias and dithered transitions. Uses the software Voodoo backend
  to honor uploaded levels; see its measured host FPS and backend notes.
  Launch with `mipmaps/run.cmd` or `./mipmaps/run.ps1`.
- [sectorforth](sectorforth/README.md): 32-bit assembly Forth port with Glide
  words and an RGB triangle defined in Forth. Launch with `sectorforth/run.cmd`
  or `./sectorforth/run.ps1`.
- [sectorlisp](sectorlisp/README.md): DOS port of sectorlisp's C reference
  evaluator with native Glide functions and an RGB triangle defined in Lisp.
  Launch with `sectorlisp/run.cmd` or `./sectorlisp/run.ps1`.

## Manuals

- [Glide 2.4 API Reference](Glide_Reference_Manual_2.4_199707.pdf) (137 pages).
- [Glide 2.4 Programming Guide](Glide_Programming_Guide_2.4_199707.pdf) (172 pages).

Original 3dfx manuals, downloaded from the
[Bitsavers mirror](https://bitsavers.trailing-edge.com/components/3dfx/).

## Tools

The complete Glide SDK and DOS runtime files are vendored in
[`vendor/`](vendor/README.md) and included in Git for reuse across demos.
Open Watcom is downloaded separately into `cube/tools/watcom` by
`cube/setup.ps1`. Build scripts copy the vendored runtimes beside each demo.
