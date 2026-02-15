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

### 1. Type System Migration (Priority: HIGH - IN PROGRESS)
**Current State**: c4 uses `#define int long long` which works on LP64 systems (Linux/macOS) but is problematic:
- Not portable to Windows (LLP64: long is 32-bit, long long is 64-bit)
- Confuses type semantics (int should be 32-bit by convention)
- Compiler warnings about format specifiers
- **BLOCKS SELF-COMPILATION**: c4 cannot compile itself because it doesn't expand `#define INT INT64`

**Implementation Status** (Feb 14, 2026):
- ✅ Added `int32_t` and `int64_t` keywords
- ✅ Added INT32/INT64 type constants (CHAR=0, INT32=1, INT64=2, PTR=3)
- ✅ Added LI32/SI32 VM opcodes for 32-bit load/store
- ✅ Updated all type parsing in sizeof, casts, declarations
- ✅ Fixed increment/decrement to use proper sizes
- ✅ Fixed pointer arithmetic for different element sizes
- ✅ Test suite passes: test/int32_64.c validates all features
- ✅ **Output matches gcc exactly**

**Implementation Status** (Feb 15, 2026):
✅ Self-compilation works: `./c4 c4.c` succeeds
✅ test/int32_64.c passes (all int32_t/int64_t operations work correctly)
✅ test/struct/simple.c passes (basic struct member access with `.` operator)
✅ CI/CD format warnings fixed (cast int64_t to (int) for printf %d)
✅ CI/CD popen/pclose errors fixed (added _POSIX_C_SOURCE 200809L)
✅ Infinite loop bug fixed (PTR stripping had no iteration limit for large StructIDs)

**Remaining Issues**:
1. **test/struct/ptr.c fails**: Arrow operator (`->`) pointer detection broken
   - Error: "not a struct pointer"
   - Root cause: Cannot reliably distinguish StructID from StructID+PTR using alignment heuristic
   - StructID values are ~5 billion (symbol table addresses)
   - StructID+PTR = StructID+3, alignment check `(ty & 7) == 0` is unreliable
   - **Attempted fixes that failed**:
     - Iteration limit: both StructID and StructID+PTR take 100+ iterations to strip
     - Alignment check: both may or may not be 8-byte aligned depending on allocator
   - **Potential solutions**:
     - Change type encoding to separate pointer flag from base type
     - Use modulo arithmetic: `ty % (some_prime)` to encode pointer layers
     - Track type metadata separately (breaks minimalist design)

2. **GitHub CI still has warnings**:
   - Format warnings on Linux (int64_t is `long`, not `long long`)
   - Should use %ld instead of %lld, but self-compilation requires no casts

**Next Steps**:
1. **Fix struct pointer detection** (PRIORITY 1 - BLOCKED):
   - Current type encoding: `StructID + n*PTR` cannot distinguish n=0 vs n>0 for large StructIDs
   - Need fundamental design change or better heuristic
   - Consider: check if `(ty - PTR) % 8 == 0` vs `ty % 8 == 0` (both StructID aligned)

2. **CI/CD format warnings** (PRIORITY 2):
   - Linux uses %ld for int64_t, macOS/current setup uses %lld
   - Options: suppress with -w flag, or use PRId64 macro (but c4 doesn't support complex macros)

### 2. Argv Support for Interpreted Programs (Priority: Low)
- **Issue**: test/args.c currently skipped because c4 doesn't properly pass argv to interpreted programs.
- **Fix**: Modify VM initialization to set up argc/argv on the stack before calling main().

### 3. Code Quality Improvements (Priority: Low)
- **Bad Address-Of Fragility**: The `&` operator checks generated instructions (`*(e-1)`) which is hacky. Consider adding a flag to track addressable expressions.
- **Memory Management**: Member lists are `malloc`'d but never freed (acceptable for a toy compiler).

## Development Notes

### Local Development Workflow

**IMPORTANT: Always follow these practices to avoid permission issues and ensure clean testing:**

1. **Temporary Test Files**: Use `./tmp/` directory (NOT `/tmp/`)
   - VSCode asks for permission on every `/tmp/` file access
   - `./tmp/` is gitignored and available for local testing
   - Example: `cat > ./tmp/test.c << 'EOF' ... EOF`

2. **Build Outputs**: Use `./build/` directory for compiled binaries
   - Keep build artifacts separate from source
   - `./build/` is gitignored
   - Example: `clang -o ./build/c4 c4.c`

3. **Test CI Locally Before Pushing**: Always run `gh` workflow test before pushing
   - GitHub CLI (`gh`) is available on this host
   - Test workflow locally: `gh workflow run ci.yml` or use act/gh features
   - Verify changes don't break CI before committing
   - Saves CI minutes and catches issues early

4. **Debug Binaries**: Also gitignored: `c4_debug`, `c4_gcc`, `c4_old`, `c4_debug.dSYM/`
