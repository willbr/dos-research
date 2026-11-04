# Shared DOS/Glide bridge

`glide_demo.c` exposes validated integer-coordinate vertex calls, triangle
drawing, refresh-paced buffer swaps and Voodoo/VGA display switching to
both sectorforth and sectorlisp. `glide_demo.h` defines the native interface.

Each host retains its own evaluator, input handling and source program.
The bridge contains no scene geometry. Its optional test mode reads the
actual Voodoo front buffer to check the classic RGB test triangle, writes
`TRIANGLE.PPM` in the demo's working directory and reports capture results
to the host's log. The shared graphics code was extracted from the tested
sectorforth host; the repeat-display regression check applies to both demos.

Native entry points use `__cdecl`; calls into the vendored DOS Glide stack
library use the SDK's `__stdcall` convention. The context is retained while
the VGA text display is active and shut down once at program exit.

License: MIT; see `LICENSE`.
