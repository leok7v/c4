# C4 Development RFCs

This document collects Request for Comments (RFCs) on potential future features for the `c4` compiler. The goal is to evaluate the cost, complexity, and impact on the project's minimalist philosophy before implementation.

## Feature: Floating Point Support (`float`, `double`)

### Motivation
To support scientific computing, geometry, and clearer code that requires fractional numbers.

### Implementation Requirements
1.  **Type System**: Expand `enum { CHAR, INT, PTR }` to include `FLOAT` (and/or `DOUBLE`).
2.  **VM Instructions**:
    *   Need duplicating arithmetic instructions: `FADD`, `FSUB`, `FMUL`, `FDIV`.
    *   Need duplicating comparison instructions: `FEQ`, `FNE`, `FLT`, `FGT`, `FLE`, `FGE`.
    *   Need conversion instructions: `ITOF` (int to float), `FTOI` (float to int).
3.  **Parser Logic (`expr()`)**:
    *   **Parsing Literals**: Need to handle `1.23` or `1e-5` in `next()`. Currently only parses integers.
    *   **Type Promotion**: When binary operators encounter mixed types (`int + float`), the parser must inject conversion instructions (`ITOF`) before the operation.
    *   **Register Management**: The VM uses `long long` for registers. A `double` fits in `long long` (64-bit), but `float` (32-bit) would need to be handled or promoted to `double`.
4.  **Standard Library**: `printf` handling for `%f` requires implementation or bridging to host `printf`.

### Cost Analysis
*   **Code Size**: High (~200-300 lines). Doubling the number of opcodes and adding significant logic to `expr()`.
*   **Complexity**: High. Implicit type promotion is error-prone in a single-pass compiler.
*   **Impact**: Fundamental change to the "everything is an integer" architecture.

---

## Feature: Extended Integer Types (`[u]int8/16/32/64_t`)

### Motivation
To support standard C types, binary file formats, and networking code where exact widths matter.

### Approach 1: True Width Support
1.  **VM Instructions**:
    *   Currently `c4` supports `long long` (64-bit) and `char` (8-bit).
    *   Need to add `SC` (Store Char), `SS` (Store Short/16-bit), `SI` (Store Int/32-bit).
    *   Need matching Load instructions (`LC`, `LS`, `LI`) with correct sign extension logic.
2.  **Unsigned Support**:
    *   Strictly speaking, C requires different comparison logic for unsigned numbers (logical shift vs arithmetic shift, `ja` vs `jg`).
    *   Would require `BLTU`, `BGTU`, `BLEU`, `BGEU` (Branch/Compare Unsigned) opcodes.

### Approach 2: Storage-Only Support (Easier)
*   Treat all variables as full 64-bit integers in registers.
*   Only truncate when storing to memory (e.g., `int16_t *p; *p = val` masks to 16 bits).
*   Requires tracking the "storage size" in the symbol table type.

### Approach 3: Alias Support (Minimalist)
*   Via `#define` or built-in `typedef`.
*   `int8_t` -> `char`
*   `int16_t` -> `int` (if on 16-bit machine) or just alias all to `int` (64-bit) and ignore width. This breaks binary compatibility but allows compilation of logic.

### Cost Analysis
*   **True Support**: Medium (~100-150 lines). Mostly boilerplate opcode duplication.
*   **Alias Support**: Low (~20 lines).

---

## Feature: `typedef` / `struct` / `union`

### Motivation
Basic data structures are missing. Currently, `c4` only supports pointers and arrays (implicit).

### Requirements
*   **Structs**: Need to parse `{ int x; char y; }` and calculate offsets.
*   **Access**: Logic for `.` operator. `->` is just `(*ptr).`.
*   **Symbol Table**: Needs to store member names and offsets, scoped to the struct definition.

### Cost Analysis
*   **Code Size**: Medium-High. The single-pass nature makes forward declarations of structs tricky without backpatching or multi-pass.

---

## Feature: Preprocessor Macros (`#define` with args)

### Motivation
To support standard headers like `<stdint.h>`.

### Requirements
*   A macro expander that handles arguments, stringification, and valid C token pasting.
*   This is often as complex as the compiler itself.

### Cost Analysis
*   **Code Size**: Very High.
*   **Recommendation**: Keep restricting to simple constants (`#define X 1`) or use an external preprocessor (`cpp | c4`).
