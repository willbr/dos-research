# 3dfx demos

Each demo lives in its own subfolder.

See the [feature checklist and demo roadmap](FEATURES.md) for capabilities,
limitations, and the next experiments to build.
See [project notes](notes.md) for proposed sectorforth and sectorlisp ports
with interactive Glide graphics.

- [cube](cube/README.md): rotating rainbow cube using DOS Glide, with a live
  FPS counter and 60 Hz frame pacing. Launch with `cube/run.cmd` or
  `./cube/run.ps1`.

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
