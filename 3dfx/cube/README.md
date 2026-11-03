# Rainbow Voodoo cube

A native 32-bit DOS program using the 3dfx Glide 2 API: twelve triangles,
eight rainbow-coloured vertices, perspective projection, three-axis rotation,
hardware Gouraud shading, depth buffering and double buffering at 640x480.

## Run

Double-click `run.cmd`, or run `./run.ps1` from PowerShell. DOSBox-X must be on
PATH. Space pauses/resumes rotation; Escape quits and closes the emulator.
The top-left corner shows completed, refresh-paced frames per second,
updated once per complete RTC second. The demo waits for the next Voodoo
vertical retrace before swapping, targeting the configured 60 Hz.
It shows `FPS --` while collecting the first sample. The counter includes
its own rendering overhead and continues updating while rotation is paused.

The local build is ready to run. `CUBE.EXE`, `DOS4GW.EXE` and `GLIDE2X.OVL`
must be together. Regular DOSBox 0.74 does not supply the required Voodoo
emulation; this configuration uses DOSBox-X's emulated Voodoo hardware with
its OpenGL backend, with Glide passthrough disabled. See the
[DOSBox-X Voodoo guide](https://dosbox-x.com/wiki/Guide:Setting-up-3dfx-Voodoo-in-DOSBox%E2%80%90X).

All three configs use `voodoo_maxmem=false` for a standard Voodoo 1:
4 MB total, split into 2 MB framebuffer/depth storage and 2 MB texture
memory, with one texture mapping unit. The demo uses 640×480 at 60 Hz.
The configured 32 MB of system RAM is separate from the card's memory.

## Build

With `curl.exe` and `7z.exe` on PATH, run `./setup.ps1` once to fetch and
extract the Open Watcom compiler locally and build. Subsequent edits only need
`./build.ps1`. If your PowerShell policy blocks scripts, use
`powershell -NoProfile -ExecutionPolicy Bypass -File ./setup.ps1` (or `build.ps1`).

The build uses Open Watcom's DOS/4GW target, stack calling convention and
case-insensitive symbol matching for the original Glide import library.
The original headers declare the calls as `__stdcall`.

The SDK and DOS runtimes are included in Git under `../vendor/`;
Open Watcom is downloaded, not installed system-wide:

- [Open Watcom](https://github.com/open-watcom/open-watcom-v2/releases/tag/Current-build)
- [Original Glide 2.43 SDK archive](https://3dfxarchive.com/downloads/glide_sdk-243.zip)
- [Voodoo 1 runtime overlay](https://github.com/efliks/nostalgic-glide-demo/blob/master/glide2x.ovl)

Third-party SDK/runtime files retain their original terms. Builds copy the
vendored runtimes into this directory; these generated copies are ignored.
The compiler download follows Current-build, so future setup runs
may use a newer compiler.

## Verification

From this directory, run `dosbox-x.exe -conf test.conf`. This renders at least
120 frames and waits for two complete FPS samples, then reads the actual
Voodoo front buffer into `CUBE.PPM`, checks for a
substantial rendered region, writes PASS/FAIL to `CUBE.LOG`, and exits.
The captured image was also visually checked. The capture check detects
blank output; it is not a complete geometric correctness test.

Run `./benchmark.ps1` with other demo instances closed to measure 600
frames using the Windows wall clock. Startup and framebuffer capture are
excluded. Marker polling introduces a few milliseconds of uncertainty.
This measures completed, refresh-paced frames. It does not measure the
host desktop compositor or monitor presentation directly. Rotation uses
one small step per refresh instead of the coarse BIOS clock, so animation
stays smooth. If emulation cannot sustain 60 Hz, rotation will slow down.
