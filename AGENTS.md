# C4 Development Guide

## Architecture

c4 is a self-compiling minimal C compiler. It compiles a subset of C into bytecode and interprets it in a stack-based VM. The compiler can compile its own source code: `./build/c4 c4.c test/hello.c`.

### Type System (PTR=256 encoding)

| Type | Encoding | Check |
|------|----------|-------|
| CHAR | 0 | `ty == CHAR` |
| INT32 | 1 | `ty == INT32` |
| INT64 | 2 | `ty == INT64` |
| Structs | 3..255 | `ty > INT64 && ty < PTR` |
| CHAR* | 256 | `ty >= PTR` |
| INT32* | 257 | |
| Struct A* | 259 | |
| CHAR** | 512 | |

- Pointers: `ty >= PTR` (>= 256). Strip one level: `ty - PTR`.
- Struct values: `ty > INT64 && ty < PTR` (3..255). Keep address in accumulator, no load.
- Struct lookup: `struct_syms[ty - INT64 - 1]` maps sequential ID to symbol table entry.

### Key Globals

- `struct_syms` — pointer to array mapping struct IDs (0-252) to symbol entries
- `num_structs` — number of defined structs (max 253)
- `e` — code emission pointer; `*++e = opcode` emits instructions
- `id` — current identifier in symbol table (array of Idsz=12 int64_t fields)
- `sym` — symbol table base

### Symbol Table Fields (Idsz = 12)

`Tk, Hash, Name, Class, Type, Val, HClass, HType, HVal, Utyedef, Extent, Sline`

- `Extent` — array size (0 for non-arrays). Arrays skip load in Id handler.
- `Sline` — for structs: linked list of members `m[0]=hash, m[1]=type, m[2]=offset, m[3]=next`
- `Val` — for structs: total size in bytes (with padding)

### Key Functions

- `next()` — lexer/tokenizer
- `memacc()` — shared member access for `.` and `->` operators
- `expr(lev)` — expression parser with precedence climbing
- `stmt()` — statement parser (if/while/return/block)
- `main()` — declarations parser + VM interpreter

### Struct Support

- **Definitions**: Sequential IDs assigned (3, 4, 5, ...). Members stored as linked list.
- **Padding**: Members aligned to natural boundary (char=1, int32=4, int64/ptr/struct=8). Struct size padded to 8-byte boundary.
- **Member access**: `memacc()` emits `PSH; IMM offset; ADD` then loads based on member type. Nested structs keep address (no load).
- **sizeof(struct X)**: Supported. Returns padded struct size.
- **Casts**: `(struct X *)` supported in cast expressions.

### Array Support

- Declaration: `int a[5];` (local) or `int g[4];` (global). Stored in `id[Extent]`.
- Type becomes `element_type + PTR`. Arrays skip load in Id handler (address IS the value).
- Element size scaling handles all types including structs via `struct_syms[...][Val]`.
- Postfix `[i]` chains with `.` and `->`: `pts[i].x` works.

### VM Opcodes

Arithmetic: LEA, IMM, PSH, LI, LC, LI32, SI, SC, SI32, ADD, SUB, MUL, DIV, MOD, etc.
Control: JMP, JSR, BZ, BNZ, ENT, ADJ, LEV.
System: OPEN, READ, CLOS, PRTF, MALC, FREE, MSET, MCMP, EXIT, WRIT, SYST, POPN, PCLS, FRED.

## Build & Test

```sh
# Build
clang -o build/c4 c4.c -O3 -m64 -std=c11 -Wall

# Run individual tests
./build/c4 test/struct/simple.c
./build/c4 test/grok.c
./build/c4 test/arrays.c

# Self-compilation (c4 compiles itself, then runs a test)
./build/c4 c4.c test/struct/simple.c

# Run with cc/gcc to cross-check (all tests have #include headers)
cc test/grok.c -o /tmp/grok && /tmp/grok

# Full test suite
./build/c4 test/test.c
```

## CI/CD

- **File**: `.github/workflows/ci.yml`
- **Build**: `clang -o c4 c4.c -O3 -m64 -std=c11 -w` (uses `-w` for Linux int64_t format warnings)
- **Tests**: struct tests, io.c, self-compilation, all-tests summary

## Self-Compilation Constraints

All c4.c code must be valid c4 input:
- No forward references to globals (declare before use)
- No mid-block variable declarations (all at top of function/block)
- No complex macros (c4 treats `#` as comment-to-EOL)
- No `unsigned`, `float`, `double`, `for`, `do`, `switch`
- `int` = `int64_t` (8 bytes). Use `(int)` casts for printf `%d`.
- c4's printf supports max 5 data arguments (format + 5 values)

## Development Workflow

- Build outputs: `./build/` (gitignored)
- Temp files: `./tmp/` (gitignored, avoids VSCode permission prompts on `/tmp/`)
- Always test self-compilation after changes: `timeout 10 ./build/c4 c4.c test/struct/simple.c`
- Test with cc/gcc to catch semantic differences (int size 4 vs 8)
