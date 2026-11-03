# 3dfx project ideas

These are future experiments, not implementations. Both would use our
standard Voodoo 1 target: 4 MB, one TMU, DOSBox-X, and the vendored Glide 2.43
SDK/runtime. The original interpreters are starting points; preserving the
512-byte boot-sector size is not a requirement for these ports.

## Port sectorforth and add Glide words

Build an interactive Forth graphics environment derived from
[sectorforth](https://github.com/cesarblum/sectorforth).

Port the 16-bit boot-sector interpreter to 32-bit protected-mode DOS under
DOS/4GW. Adapt cells, pointers, dictionary entries, stacks and execution
code for 32 bits; replace BIOS-based input/output with DOS-compatible
console handling. Add convenient number parsing and enough arithmetic for
graphics rather than retaining the original minimal bootstrap interface.

Expose Glide through native assembly words for initialization, vertex
creation, triangle drawing, textures, render state, buffer swaps and shutdown.
An initial bridge can accept integers or fixed-point values and construct
the floating-point `GrVertex` structures expected by Glide. Keep the Forth
data/return stacks distinct from the native calling stack and obey the DOS
Glide library's calling convention.

Start with a small C startup/Glide host and an assembly interpreter, then
move more startup work into assembly if useful. First milestone: define
and draw a colored triangle from the REPL. Next: define a rotating cube and
change its colors and speed interactively. Graphics mode also needs a
visible command interface, such as a Glide text overlay or an explicit
console/graphics toggle.

This is the suggested first interpreter experiment: Forth's stack and
explicit memory model make a small native bridge straightforward to design.

## Port sectorlisp and add Glide primitives

Build a small interactive Lisp graphics environment derived from
[sectorlisp](https://github.com/jart/sectorlisp).

Port the interpreter to the same 32-bit DOS/4GW host. Adapt its object
representation, pointers, heap/garbage collector and console handling;
account for the original implementation's compact encoding rather than
simply replacing 16-bit registers with 32-bit registers. The project also
includes a C implementation worth evaluating as a readable starting point.

Add numeric values, useful arithmetic, and a native primitive dispatch
mechanism for Glide. Start with integer/fixed-point coordinates converted
by the bridge, or design a floating-point numeric representation. Keep
native vertex/texture storage outside the moving Lisp heap, or explicitly
manage references across garbage collection; Glide must receive valid
native addresses.

Proposed primitives include graphics initialization, vertex/triangle
construction, clear, texture upload, render-state changes, swap and shutdown.
First milestone: evaluate a Lisp expression that draws a colored triangle.
Next: describe a scene as lists and animate it through Lisp functions, with
live changes from a visible REPL.

## Shared implementation boundaries

- Both projects call the existing 32-bit Glide library. They do not need a
  new Voodoo hardware driver, programmable shaders or a bare-metal OS.
- Use the vendored stack-call DOS import library and runtime. Native
  functions must match argument layout, symbol naming and register/stack
  preservation requirements.
- The CPU still performs transformations, projection, lighting and 3D
  clipping. Voodoo handles rasterization and its fixed-function effects.
- Keep frame presentation refresh-paced and preserve the standard memory
  configuration used by our existing demos.
- Develop each interpreter in its own folder and retain its upstream
  license and source attribution if we import code.
- Use the [feature roadmap](FEATURES.md) as a source of small graphics
  examples that can eventually be expressed in either language.
