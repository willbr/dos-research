# sectorlisp with 3dfx Glide

A 32-bit DOS port based on the **C reference implementation** in
[jart/sectorlisp](https://github.com/jart/sectorlisp), with a text REPL and
native Glide functions. This is not a port of its boot-sector assembly.
The boot-sector size constraint is removed.

`TRIANGLE.LSP` supplies all triangle coordinates, colors, and draw sequencing.
The native host only bridges Lisp calls to Glide. The shared bridge in
`../common/` also serves the sectorforth demo.

## Run

Launch `run.cmd` or `./run.ps1` with DOSBox-X on PATH. The RGB triangle opens
at 640×480 with standard 4 MB, one-TMU Voodoo 1 emulation.

Press any key to return to the text REPL. Try:

```lisp
(+ 6 7)
(define twice (lambda (x) (+ x x)))
(twice 21)
(show-triangle)
(bye)
```

Use parentheses: `(show-triangle)` calls the function. Bare `show-triangle`
evaluates to its lambda list. Names are case-insensitive. Definitions can
span lines; the prompt changes from `*` to `...` until parentheses balance.

`(reset)` on its own REPL line clears definitions and the cons heap while
retaining native bindings. The triangle functions must then be defined
again. `sl3dfx.exe filename.lsp` loads another file before entering the REPL.
Edit `TRIANGLE.LSP` to change the initial image without rebuilding C.

## Port and language

The list evaluator retains upstream's `Car`, `Cdr`, `Cons`, `Evlis`,
`Pairlis`, `Assoc`, `Evcon`, `Apply`, and `Eval` structure, including the
negative-cons/nonnegative-atom representation.

Changes for this DOS graphics environment:

- Replace Bestline/Unicode terminal support with a DOS text REPL and a
  bounded source reader. Keep the original reference source unmodified
  under `upstream/` for comparison.
- Add signed 32-bit decimal integers, arithmetic, quote shorthand,
  semicolon comments, dotted lists, persistent `define`, and `progn`.
- Lambda expressions can be evaluated directly or quoted. Functions use
  **dynamic scope**, as in the reference evaluator; these are not lexical
  closures. `label` supports locally named recursive functions.
- Replace per-evaluation copying collection with stable cons handles and
  mark/sweep collection **between top-level expressions**, rooted in the
  global environment. This preserves stored definitions and data without
  invalidating active native pointers. No collection runs inside an active
  expression; a single expression can exhaust the cons heap.
- Use 16,384 cons cells and 128 KB of interned atom storage. Atom names
  have a 63-character limit. Atoms, including numeric spellings, are not
  reclaimed; exhausted atom storage requires restarting the program.
- Bound reader and evaluator nesting at 128 and execution at 100,000
  evaluator/application steps per top-level expression. Signed arithmetic
  overflow, malformed lists, arity errors and unknown symbols report errors.

Core forms/functions: `quote cond read print atom car cdr cons eq lambda label`.
Extensions: `define progn + - * < assert` and the native functions below.
Arithmetic functions take two arguments. `nil` is false and the empty list;
`t` is true. **Zero is true**, as in Lisp. `car` and `cdr` of `nil` return
`nil`. `eq` compares object identity; canonical integer spellings compare
equal. It is not structural list equality.

`read` reads the next form from the current source stream, not a separate
keyboard prompt. `(reset)` is a REPL command, not a callable Lisp function.
Errors abandon the current input but preserve completed definitions and
earlier side effects. `print` is visible in text mode; there is no graphics
console overlay. This is neither Common Lisp nor Scheme and has no strings,
floating-point numbers, lexical closures, macros or general foreign calls.

## Native graphics functions

| Function | Arguments | Behavior |
| --- | --- | --- |
| `glide-init` | none | Activate the Voodoo output and configure RGB shading. |
| `glide-close` | none | Return to VGA text; retain the graphics context for reuse. |
| `clear` | none | Clear the back buffer to a dark background. |
| `vertex` | `index x y red green blue` | Populate one of 32 vertices. Integer values become floats in the bridge. |
| `triangle` | `a b c` | Draw three initialized vertex indices. |
| `present` | none | Wait for retrace, swap buffers and finish queued work. |
| `key` | none | Wait for a key and return its code. |
| `bye` | none | Exit after the current input finishes. |

All native arguments must be integers. Vertex indices must be 0–31,
coordinates X 0–639 and Y 0–479, and RGB values 0–255. The bridge validates
ranges and graphics state. Native operations return 0 on successful graphics
actions; `key` returns the key code. A native failure raises an error.

The shared Glide bridge retains its context across text/graphics switches
and waits across two retrace boundaries on reactivation, preventing the
black second frame found during the sectorforth port. It fully shuts down
Glide on process exit. No Lisp object pointer is passed into Glide; vertices
are copied into stable native storage.

## Build and verify

Run `./setup.ps1` once, then `./build.ps1` after C changes. The compiler is
Open Watcom, shared with `../cube/tools/watcom`; the SDK and runtime come
from `../vendor/`. This C-reference port does not require NASM. The DOS
executable uses DOS/4GW and a 256 KB native stack.

Run `dosbox-x.exe -conf test.conf` from this folder. It runs `TESTS.LSP` and
checks recursive functions, local `label`, quoted/dotted lists, numbers,
persistent definitions, `read`, repeated garbage collection and recovery
from heap exhaustion, excessive nesting, nonterminating calls, type/arity
errors, bad syntax, arithmetic overflow, and invalid graphics state.

It then loads the real `TRIANGLE.LSP`, returns to text, and invokes the
same `(show-triangle)` function twice more. Real Voodoo framebuffer samples
check RGB corners, blended interior and triangle coverage on all three
visits. Results go to `LISP.LOG`; the last capture is `TRIANGLE.PPM`.
The captured triangle was also checked visually. These are DOSBox-X tests,
not tests on physical hardware.

## Upstream and license

Baseline: https://github.com/jart/sectorlisp/commit/162c845c1dc3d0ac445951b6ef5af9f660682143

Unmodified `lisp.c`, `sectorlisp.S` and the ISC license are in `upstream/`.
The Lisp port uses the same ISC terms; see `LICENSE`. The shared Glide
bridge is MIT-licensed; its license is in `../common/LICENSE`. The vendored
SDK/runtime retain their original terms.
