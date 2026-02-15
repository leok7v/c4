# C4 Struct Implementation Status

## Overview
We are implementing `struct` support in `c4.c`. The implementation handles basic struct definitions, local/global variables, and member access via `.` and `->`.

## Implementation Details

### 1. Struct Definition (`struct Name { ... }`)
- **Symbol Table**: The identifier `Name` is stored with `Class = Struct`.
- **Metadata**: 
  - `Type` stores the address of the symbol itself (acting as a unique Struct ID).
  - `Sline` stores a pointer to the head of a linked list of members.
  - `Val` stores the total size of the struct in bytes.
- **Members**: Stored as a linked list of `malloc`'d integer arrays `int *m`.
  - `m[0]`: Hash (name).
  - `m[1]`: Type (e.g., `INT`, `CHAR`, or StructID).
  - `m[2]`: Offset from start of struct.
  - `m[3]`: Next member pointer.

### 2. Variable Declaration
- **Global/Local**: The parser consumes `struct Name var;`.
  - `id[Type]` is set to `bt` (the Struct ID).
  - Size calculation logic was updated to handle `ty > PTR`. If it is a pointer `(ty & 3) != 0`, size is `sizeof(int)`. If it is a struct value, size is `((int*)ty)[Val]` (the struct size stored in the Type symbol).

### 3. Expression Handling
- **Load Logic (`Id`)**: 
  - Struct values (Large Values) are **not** loaded into registers with `LI`/`LC`. The address is kept in the accumulator.
  - Logic: `if (ty > PTR && (ty & 3) == 0) ; // Keep address`.
- **Address-Of (`&`)**:
  - Modified to accept instructions other than `LC`/`LI` (like `LEA`, `IMM`, `ADJ`) if the type is a Struct Value, since we intentionally skipped the load.

### 4. Member Access
- **Dot (`.`)**:
  - Expects `ty` to be a Struct ID.
  - Traverses the member list stored in `((int*)ty)[Sline]`.
  - Emits `IMM offset; ADD`.
  - Updates `ty` to the member's type.
  - Loads value (`LI`/`LC`) unless the member is itself a struct/array.
- **Arrow (`->`)**:
  - Expects `ty` to be a Pointer to Struct (`Struct ID + PTR` or more).
  - Dereferences the pointer simply by using the value (address) already loaded.
  - Logic unwraps pointer layers: `ty = ty - PTR`.
  - Performs same member lookup and offset addition as `.`.

## CI/CD Automation

### GitHub Actions Workflow
- **File**: `.github/workflows/ci.yml`
- **Trigger**: Push to `master` or `main` branches
- **Runner**: `ubuntu-latest` (Ubuntu 24.04)
- **Build**: `gcc -o c4 c4.c -O3 -m64 -std=c11 -w`
  - `-w` flag suppresses warnings (from LP64 int/long issues)
  
### Test Coverage
1. **Struct Tests**:
   - `test/struct/simple.c`: Basic struct with `.` operator
   - `test/struct/ptr.c`: Struct pointers with `->` operator  
   - `test/struct/nested.c`: Nested struct definitions
2. **I/O Test**:
   - `test/io.c`: File operations (open, read, write syscalls)
3. **Self-Compilation**:
   - `./c4 c4.c`: Compiler compiling itself
4. **Meta Test**:
   - `./c4 c4.c test/test.c`: Self-compiled c4 running test suite

### Test Results
All tests passing with clean exit codes (0):
- simple.c: "Point(10, 20)" cycle=35
- ptr.c: "Pointer(30, 40)" cycle=43  
- nested.c: "Nested(X, 50, Y)" cycle=60
- io.c: File I/O operations successful

## Resolved Issues ✓

### 1. Segmentation Fault in `test/struct/ptr.c` - FIXED
- **Root Cause**: Double LC/LI emission in variable loading logic.
- **Solution**: Consolidated if-else chain in `expr()` (lines 167-180):
  - `CHAR` → `LC`
  - Struct values `((ty & 3) == 0 && ty > PTR)` → keep address, no load
  - `INT` and all pointers → `LI`
- **Status**: All struct tests passing (simple.c, ptr.c, nested.c)

### 2. Duplicate Member Access Handlers - FIXED
- **Root Cause**: Both `.` and `->` operators handled in two different loops (postfix and precedence climbing).
- **Solution**: Removed duplicate handler from precedence climbing loop, kept only postfix version.
- **Status**: Clean implementation, no code duplication.

### 3. OPEN Syscall Failures in CI - FIXED
- **Root Cause**: OPEN was hardcoded to 3-arg version, but read operations use 2-arg open().
- **Solution**: Check `pc[1]` (ADJ value) to determine argument count:
  ```c
  a = (pc[1] == 2) ? open((char *)sp[1], *sp) : open((char *)sp[2], sp[1], *sp);
  ```
- **Status**: test/io.c passing in CI.

## Future Work

### 1. Type System Migration (Priority: Medium)
**Current State**: c4 uses `#define int long long` which works on LP64 systems (Linux/macOS) but is problematic:
- Not portable to Windows (LLP64: long is 32-bit, long long is 64-bit)
- Confuses type semantics (int should be 32-bit by convention)
- Compiler warnings about format specifiers

**Required Changes**:
- Replace with proper stdint.h types: `int64_t`, `int32_t`, `int16_t`, `int8_t`
- Add unsigned type support: `uint64_t`, `uint32_t`, etc.
- Update VM to handle different integer sizes properly
- Fix format specifiers in printf/scanf

**Files to Modify**:
- c4.c: Type constants (INT, CHAR, PTR)
- c4.c: VM opcodes (LI, LC, SI, SC need size variants)
- Build flags: Can remove `-w` once warnings are properly addressed

### 2. Argv Support for Interpreted Programs (Priority: Low)
- **Issue**: test/args.c currently skipped because c4 doesn't properly pass argv to interpreted programs.
- **Fix**: Modify VM initialization to set up argc/argv on the stack before calling main().

### 3. Code Quality Improvements (Priority: Low)
- **Bad Address-Of Fragility**: The `&` operator checks generated instructions (`*(e-1)`) which is hacky. Consider adding a flag to track addressable expressions.
- **Memory Management**: Member lists are `malloc`'d but never freed (acceptable for a toy compiler).
