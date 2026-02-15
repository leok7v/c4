#include <stdio.h>
#include <stdint.h>

// Test struct padding, sizeof(struct), casts, and pointer arithmetic

struct Point { int x; int y; };

struct Padded { char c; int i; }; // c at 0, padding, i at 8 (c4) or 4 (cc)

struct Nested { char a; struct Padded inner; };

int main(int argc, char **argv) {
    struct Padded m;
    struct Nested n;
    struct Point pts[3];
    struct Point *pp;
    int32_t x;
    int32_t y;
    int passed;
    int total;

    passed = 0;
    total = 0;

    // Test 1: Struct padding - char followed by int should not corrupt
    total++;
    m.c = 'A';
    m.i = 42;
    if (m.c == 'A' && m.i == 42) {
      printf("Test 1 PASS: padding c=%c i=%d\n", m.c, m.i);
      passed++;
    } else {
      printf("Test 1 FAIL: padding corruption c=%d i=%d\n", m.c, m.i);
    }

    // Test 2: Nested struct with padding
    total++;
    n.a = 'B';
    n.inner.c = 'C';
    n.inner.i = 99;
    if (n.a == 'B' && n.inner.c == 'C' && n.inner.i == 99) {
      printf("Test 2 PASS: nested padding\n");
      passed++;
    } else {
      printf("Test 2 FAIL: nested corruption\n");
    }

    // Test 3: Post-increment on int32_t
    total++;
    x = 10;
    y = x++;
    if (x == 11 && y == 10) {
      printf("Test 3 PASS: post-inc x=%d y=%d\n", x, y);
      passed++;
    } else {
      printf("Test 3 FAIL: post-inc x=%d y=%d\n", x, y);
    }

    // Test 4: sizeof(struct) matches 2 * sizeof(int)
    total++;
    if (sizeof(struct Point) == 2 * sizeof(int)) {
      printf("Test 4 PASS: sizeof(struct Point)=%d\n", (int)sizeof(struct Point));
      passed++;
    } else {
      printf("Test 4 FAIL: sizeof(struct Point)=%d expected %d\n",
        (int)sizeof(struct Point), (int)(2 * sizeof(int)));
    }

    // Test 5: Struct pointer arithmetic with ++
    total++;
    pts[0].x = 1; pts[0].y = 2;
    pts[1].x = 3; pts[1].y = 4;
    pts[2].x = 5; pts[2].y = 6;
    pp = pts;
    pp++;
    if (pp->x == 3 && pp->y == 4) {
      printf("Test 5 PASS: struct ptr++ (%d,%d)\n", pp->x, pp->y);
      passed++;
    } else {
      printf("Test 5 FAIL: struct ptr++ (%d,%d)\n", pp->x, pp->y);
    }

    // Test 6: Cast to struct pointer
    total++;
    pp = (struct Point *)pts;
    if (pp->x == 1 && pp->y == 2) {
      printf("Test 6 PASS: (struct Point *) cast\n");
      passed++;
    } else {
      printf("Test 6 FAIL: (struct Point *) cast\n");
    }

    printf("SUMMARY: %d/%d tests passed\n", passed, total);
    return (passed == total) ? 0 : -1;
}
