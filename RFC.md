# C4 Development RFCs

Request for Comments on potential features. Evaluate cost, complexity, and impact on minimalist philosophy before implementation.

---

## Completed: Extended Integer Types (`int32_t`, `int64_t`)

**Status**: Fully implemented and working (Feb 2026)

- `int32_t` and `int64_t` keywords, sizeof, casts, arithmetic
- LI32/SI32 VM opcodes with proper 32-bit sign extension
- Pointer arithmetic scales by element size (1/4/8)
- Self-compilation works, all tests pass, output matches gcc

---

## Completed: Struct Support

**Status**: Fully implemented (Feb 2026)

- Struct definitions with sequential type IDs (PTR=256 encoding)
- Member access via `.` and `->` (shared `memacc()` function)
- Nested struct support with correct offset chaining
- Struct padding/alignment (char=1B, int32=4B, int64/ptr=8B boundaries)
- `sizeof(struct X)` returns padded struct size
- `(struct X *)` cast expressions
- Arrays of structs with correct element size scaling
- Struct pointer arithmetic (add/sub/inc/dec scale by struct size)

---

## Completed: Array Declarations

**Status**: Fully implemented (Feb 2026)

- Local and global arrays: `int a[5];`, `char buf[10];`
- Arrays of all types: char, int32_t, int64_t, pointers, structs
- Array decay to pointer (type = element_type + PTR)
- Postfix `[]` chains with `.` and `->`: `pts[i].x`
- Correct element size scaling for all types

---

## RFC: Unsigned Integer Types (`uint32_t`, `uint64_t`)

### Requirements
- Unsigned comparisons: BLTU, BGTU, BLEU, BGEU
- Unsigned division/modulo
- Logical right shift (vs arithmetic)
- Type encoding: distinguish signed vs unsigned
- `%u` printf format

### Cost: Medium (~100 lines). Priority: LOW.

---

## RFC: Floating Point (`float`, `double`)

### Requirements
- New VM opcodes: FADD, FSUB, FMUL, FDIV, FEQ, FNE, FLT, FGT, FLE, FGE, ITOF, FTOI
- Literal parsing: `1.23`, `1e-5` in `next()`
- Type promotion rules (int + float)
- `%f` printf support

### Cost: High (~200-300 lines). Fundamental change to "everything is an integer" architecture.

---

## RFC: typedef / union

### typedef
Alias support for types. Cost: Low (~30 lines).

### union
Overlapping member storage (all members at offset 0). Cost: Medium (~50 lines).

---

## RFC: Preprocessor Macros (`#define` with args)

Full macro expander nearly as complex as the compiler itself.
**Recommendation**: Keep restricting to simple constants or use external preprocessor (`cpp | c4`).
