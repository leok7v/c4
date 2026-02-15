# C4 Development RFCs

This document collects Request for Comments (RFCs) on potential future features for the `c4` compiler. The goal is to evaluate the cost, complexity, and impact on the project's minimalist philosophy before implementation.

---

## ✅ COMPLETED: Extended Integer Types (`int32_t`, `int64_t`)

### Status: IMPLEMENTED (Feb 14, 2026)
**Implementation**: Storage-and-Computation Support

### What Works:
- ✅ `int32_t` and `int64_t` keywords recognized
- ✅ `sizeof(int32_t)` returns 4, `sizeof(int64_t)` returns 8
- ✅ New VM opcodes: LI32 (load 32-bit), SI32 (store 32-bit)
- ✅ Proper 32-bit sign extension on load
- ✅ Pointer arithmetic scales by element size
- ✅ Increment/decrement work correctly
- ✅ **Output matches gcc exactly**

### Known Limitations:
1. **Self-compilation broken**: c4 cannot compile itself due to `#define INT INT64` macro
2. **Struct tests regressed**: Changes broke test/struct/simple.c and test/struct/ptr.c  
3. **No unsigned support**: Only signed int32_t/int64_t implemented

### Test Coverage:
- test/int32_64.c: Comprehensive test validates all operations
- Comparison: `./c4 test/int32_64.c` matches `gcc test/int32_64.c` output

### Next Steps (Prioritized):
- **PRIORITY 1**: Fix self-compilation by making c4.c int-precision independent (grep/replace `int` → `int64_t` where needed)
- **PRIORITY 2**: Fix struct test regression (debug struct value detection logic)
- **PRIORITY 3**: Add uint32_t/uint64_t support (LOW priority)

---

## Feature: Extended Integer Types - Phase 2 (Unsigned Support)

### Motivation
Support `uint32_t` and `uint64_t` for bitwise operations, binary formats, and unsigned arithmetic.

### Requirements (Beyond Current Implementation):
1. **VM Instructions**:
   - Unsigned comparisons: BLTU, BGTU, BLEU, BGEU
   - Unsigned division/modulo (different behavior on x86)
   - Logical right shift (vs arithmetic shift)

2. **Type System**:
   - Distinguish signed vs unsigned in type encoding
   - Implicit conversion rules (C spec is complex here)

3. **Printf Support**:
   - `%u` format specifier handling

### Cost Analysis
- **Code Size**: Medium (~100 lines)
- **Complexity**: Medium - need to track signedness
- **Priority**: LOW - can be added incrementally

---

## Feature: Floating Point Support (`float`, `double`)

### Motivation
To support scientific computing, geometry, and clearer code that requires fractional numbers.

### Implementation Requirements
1.  **Type System**: Expand `enum { CHAR, INT32, INT64, PTR }` to include `FLOAT` (and/or `DOUBLE`).
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

## Feature: `typedef` / `struct` / `union`

### Status: `struct` IMPLEMENTED (See AGENTS.md)

### Motivation
Basic data structures are missing. Currently, `c4` only supports pointers and arrays (implicit).

### Struct Support (DONE):
- ✅ Basic struct definitions
- ✅ Member access via `.` operator
- ✅ Pointer member access via `->` operator
- ⚠️ Currently broken after int32_t/int64_t changes

### Remaining Work:
*   **typedef**: Alias support for types
*   **union**: Overlapping member storage

### Cost Analysis
*   **typedef**: Low (~30 lines). Just symbol table management.
*   **union**: Medium (~50 lines). Similar to struct but members share offset 0.

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
