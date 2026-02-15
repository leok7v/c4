#include <stdio.h>
#include <stdint.h>

// Stricter padding and overlap tests (from Grok, fixed for c4 compatibility)

struct Point {
    int x;
    int y;
};

// char + int + char: tests padding between members
struct Misaligned {
    char c;
    int i;
    char d;
};

struct Nested {
    char a;
    struct Misaligned inner;
};

// char + int64_t + char: strictest padding test
struct StrictPad {
    char c;
    int64_t i;
    char d;
};

int main(int argc, char **argv) {
    char buf[10];
    int i;
    int32_t x;
    int32_t y;
    struct Point pts[2];
    struct Point *pp;
    struct Nested n;
    struct Misaligned m;
    struct StrictPad arr[2];
    int passed;
    int total;

    passed = 0;
    total = 7;

    // Test 1: Struct padding (char + int + char, no corruption)
    m.c = 'A';
    m.i = 42;
    m.d = 'B';
    if (m.c == 'A' && m.i == 42 && m.d == 'B') {
        printf("Test 1 PASS: padding c=%c i=%d d=%c\n", m.c, m.i, m.d);
        passed++;
    } else {
        printf("Test 1 FAIL: c=%d i=%d d=%d\n", m.c, m.i, m.d);
    }

    // Test 2: Nested struct padding
    n.a = 'B';
    n.inner.c = 'C';
    n.inner.i = 99;
    n.inner.d = 'D';
    if (n.a == 'B' && n.inner.c == 'C' && n.inner.i == 99 && n.inner.d == 'D') {
        printf("Test 2 PASS: nested padding\n");
        passed++;
    } else {
        printf("Test 2 FAIL: nested corruption\n");
    }

    // Test 3: Post-increment on int32_t
    x = 10;
    y = x++;
    if (x == 11 && y == 10) {
        printf("Test 3 PASS: post-inc x=%d y=%d\n", x, y);
        passed++;
    } else {
        printf("Test 3 FAIL: post-inc x=%d y=%d\n", x, y);
    }

    // Test 4: Char array allocation
    i = 0;
    while (i < 9) { buf[i] = 'A' + i; i++; }
    buf[9] = 0;
    printf("Test 4 PASS: char array: %s\n", buf);
    passed++;

    // Test 5: Struct pointer arithmetic
    pts[0].x = 1; pts[0].y = 2;
    pts[1].x = 3; pts[1].y = 4;
    pp = pts;
    pp++;
    if (pp->x == 3 && pp->y == 4) {
        printf("Test 5 PASS: struct ptr++ (%d,%d)\n", pp->x, pp->y);
        passed++;
    } else {
        printf("Test 5 FAIL: struct ptr++ (%d,%d)\n", pp->x, pp->y);
    }

    // Test 6: sizeof(struct) is portable (2 * sizeof(int))
    if (sizeof(struct Point) == 2 * sizeof(int)) {
        printf("Test 6 PASS: sizeof(Point)=%d\n", (int)sizeof(struct Point));
        passed++;
    } else {
        printf("Test 6 FAIL: sizeof(Point)=%d\n", (int)sizeof(struct Point));
    }

    // Test 7: Stricter padding with array (detect overlap between elements)
    arr[0].c = 'X';
    arr[0].i = 0x7FFFFFFFFFFFFFFF;
    arr[0].d = 'Y';
    arr[1].c = 'Z';
    if (arr[1].c == 'Z' && arr[0].d == 'Y' && arr[0].c == 'X') {
        printf("Test 7 PASS: strict padding, no overlap\n");
        passed++;
    } else {
        printf("Test 7 FAIL: overlap corruption\n");
    }

    printf("SUMMARY: %d/%d tests passed\n", passed, total);
    return (passed == total) ? 0 : -1;
}
