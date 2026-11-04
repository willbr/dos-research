# sectorforth with 3dfx Glide

A 32-bit assembly port of Cesar Blum's sectorforth, hosted by DOS/4GW,
with native Glide words. `TRIANGLE.F` compiles Forth definitions that supply
three colored vertices and draw the classic RGB triangle. Geometry and
the draw sequence live in Forth; `host.c` provides the DOS/Glide bridge.
The graphics implementation is shared with sectorlisp in
[`../common/`](../common/README.md); `host.c` retains the Forth console host.

## Run

Launch `run.cmd` or `./run.ps1` with DOSBox-X on PATH. The triangle appears
at 640×480 using standard 4 MB, one-TMU Voodoo 1 emulation.

Press any key to close graphics and return to the text REPL. Try:

```forth
6 7 + .
: twice dup + ;
21 twice .
show-triangle
```

`show-triangle` opens graphics again and waits for a key. Enter `bye` on
its own line to quit, or `reset` to clear stacks and user definitions.
After reset, the definitions from `TRIANGLE.F` must be entered again.
Use `sf3dfx.exe filename.f` inside DOSBox-X to load a different file before
entering the REPL. To change the initial image, edit `TRIANGLE.F` on the host;
no assembly or C rebuild is required.

## What was ported

The original link/name/flags dictionary, direct-threaded dispatch, colon
compiler, hidden definitions, immediate semicolon, original eight core
primitives and five variables are adapted in `forth.asm`.

- Cells, addresses and threaded instructions are now 32 bits.
- EBX holds the data-stack pointer, EBP the return-stack pointer, and ESI
  the threaded instruction pointer. ESP remains a native C-call stack.
- Colon definitions start with a five-byte jump to `DOCOL` and contain
  threaded word addresses. Compiled numbers use an inline literal primitive.
- A 256 KB dictionary and two 1024-cell stacks replace the boot-sector map.
- Names are case-insensitive. Decimal signed 32-bit numbers, `$` hex numbers,
  negative numbers, backslash line comments and `( comments )` are supported.
- Checked stack/dictionary limits and error recovery replace the boot-sector
  reset routine. An error discards an incomplete definition but retains prior
  completed definitions. Definitions may span REPL lines.
- `key`/`emit` use the DOS host instead of real-mode BIOS calls. Console I/O
  is visible in text mode; this version has no graphical text overlay.

This remains a small experimental Forth, not an ANS Forth. It does not yet
have floating-point words, branches/loops, general foreign-function calls,
file words or a sandbox. `@`, `!`, `here`, and `latest` provide unrestricted
native memory access: invalid pointers or dictionary edits can crash it.

## Words

Original core: `@ ! sp@ rp@ 0= + nand exit`.
Variables: `tib state >in here latest` (`tib` returns the current input base;
`>in` is a byte offset). Compiler/I/O: `: ; key emit`.

Convenience words: `dup drop swap over - * = depth . assert`.
True flags are -1; false is 0. Arithmetic uses 32-bit cells.

| Glide word | Stack effect | Behavior |
| --- | --- | --- |
| `glide-init` | `( -- )` | Open Voodoo graphics, with iterated RGB shading. |
| `glide-close` | `( -- )` | Deactivate graphics and restore the text display; retain the context for reuse. |
| `clear` | `( -- )` | Clear the back buffer to a dark background. |
| `vertex!` | `( x y r g b index -- )` | Set one of 32 vertices; integer screen coordinates become floats in the bridge. |
| `triangle` | `( a b c -- )` | Draw three initialized vertex indices. |
| `present` | `( -- )` | Wait for retrace and swap buffers. |

Colors must be 0–255, X 0–639, Y 0–479, and indices 0–31. The bridge
validates these ranges. Native C functions use `__cdecl`; Glide calls use
the SDK's `__stdcall` ABI through its DOS stack import library.

Returning to graphics reactivates the existing Voodoo context and waits
across two retrace boundaries before drawing. This allows DOSBox-X's host
display switch to finish; rendering immediately could lose the next frame
and leave a black screen on the second `show-triangle`. Glide is fully shut
down when the process exits.

Error codes: 1 unknown word/invalid number, 2 stack underflow, 3 stack
overflow, 4 compiler/comment error, 5 dictionary full/allocation failure,
6 failed assertion, 7 invalid graphics state/argument or graphics failure.

## Build and verify

Run `./setup.ps1` once, then `./build.ps1` after source edits. Requires NASM
and Open Watcom; the compiler is shared with `../cube/tools/watcom`.
Setup downloads that compiler if absent. The SDK and DOS runtimes come from
`../vendor`. Generated runtime copies beside the EXE are ignored by Git.

From this folder, run `dosbox-x.exe -conf test.conf` for automated checks:

- `TESTS.F` exercises the port's core, memory words, compiled literals,
  nested definitions, early exit, return-stack access and case folding.
- Host checks verify cross-evaluation state, multiline compilation, stack
  underflow/overflow, dictionary exhaustion and recovery, failed assertions,
  unknown words, malformed numbers and graphics-state errors.
- The same `TRIANGLE.F` runs with `key` returning immediately. Real Voodoo
  framebuffer samples check RGB corner dominance, blended interior and
  triangle coverage. The compiled `show-triangle` word is then evaluated
  twice more after returning to text, checking the framebuffer on all three
  visits to catch display-reactivation regressions. The test writes
  `TRIANGLE.PPM` and `FORTH.LOG`, then exits.

The RGB framebuffer was also inspected visually. These are emulator tests,
not verification on physical 3dfx hardware.

## Upstream and license

Upstream: https://github.com/cesarblum/sectorforth

Baseline revision: `32031ac6e77e30817c2f65ba11b1ccda07d564f9`.
The unmodified original assembly and MIT license are retained in `upstream/`.
This port removes the boot-sector size constraint and retains the original
author's attribution. The port and bridge are distributed under the same
MIT terms; the separately vendored SDK/runtime retain their original terms.
