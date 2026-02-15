# C4 Project Notes

## What is c4
c4 is a minimal self-compiling C compiler by Robert Swierczek. It compiles a subset of C to bytecode and interprets it in a stack-based VM. The entire compiler fits in ~800 lines of C.

## Type System
- `enum { CHAR=0, INT32=1, INT64=2, PTR=256 }`
- Struct types occupy IDs 3..255 (sequential assignment via `num_structs`)
- `struct_syms[ty - INT64 - 1]` maps struct ID to symbol table entry
- Pointer: add PTR (256). `int*` = 258, `int**` = 514, `struct A*` = 259
- Check pointer: `ty >= PTR`. Check struct value: `ty > INT64 && ty < PTR`

## c4's int is int64_t
In c4, `int` is 8 bytes (int64_t). This differs from cc/gcc where int is 4 bytes. Tests should use relative comparisons (e.g., `sizeof(struct Point) == 2 * sizeof(int)`) not absolute byte values.

## Self-Compilation Constraints
c4.c must be valid c4 input. This means:
- No forward references to globals
- No mid-block variable declarations (all at top of block)
- No `#define` macros (c4 treats `#` as comment-to-EOL)
- No `for`, `do`, `switch`, `unsigned`, `float`, `double`
- No `&&` or `||` short-circuit in variable declarations
- printf limited to 5 data arguments (format + 5 = 6 total args)
- After changing c4.c, always verify: `timeout 10 ./build/c4 c4.c test/struct/simple.c`

## Key Implementation Details
- `memacc()` handles both `.` and `->` member access (shared code)
- Arrays use `id[Extent]` to store size; arrays skip load in Id handler
- Struct members stored as linked list: `m[0]=hash, m[1]=type, m[2]=offset, m[3]=next`
- Struct padding: members aligned to natural boundary, struct size padded to 8 bytes
- Postfix loop `while (tk == Brak || tk == '.' || tk == Arrow)` handles chaining like `pts[i].x`
- Postfix Brak must use `*++e = PSH` (append), never `*e = PSH` (replace) — replacing breaks pointer subscripts and caused self-compilation hang

## Build & Test
```sh
clang -o build/c4 c4.c -O3 -m64 -std=c11 -Wall    # build
./build/c4 test/grok.c                               # direct test
timeout 10 ./build/c4 c4.c test/struct/simple.c      # self-compilation test
cc test/grok.c -o /tmp/grok && /tmp/grok             # cross-check with system compiler
```

## Test Files
All test files have `#include` headers so they compile with both c4 and cc/gcc/clang.
- test/grok.c — padding, sizeof(struct), cast, struct ptr arith
- test/arrays.c — all array types including struct arrays
- test/struct_ptr_arith.c — pointer arithmetic, pre/post increment
- test/struct/{simple,ptr,nested}.c — basic struct operations
- test/int32_64.c — int32_t/int64_t operations
- test/io.c — file I/O (open/read/write)
- test/test.c — test runner (discovers and runs all tests)

## CI
`.github/workflows/ci.yml` — builds on ubuntu-latest with `clang -w` (suppresses Linux int64_t format warnings), runs struct tests, io, self-compilation.

## Common Pitfalls
- Lexer maps `[` to `Brak` token (169), not ASCII `[` (91). Use `tk == Brak`, not `tk == '['`.
- `id` pointer changes after `next()` — save to local `s` or `d` before calling next().
- Element size must handle structs everywhere: Add, Sub, Inc, Dec, Brak (both postfix and precedence climbing), local/global array allocation.
- PSH instruction pushes `a` without modifying it — subsequent LI correctly reloads from address still in `a`.
