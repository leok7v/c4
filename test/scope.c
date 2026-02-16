#include <stdio.h>

// Test block-scoped declarations, init-in-declaration, and (void) params

int passed;
int failed;

void check(int test_num, int cond, char *msg) {
    if (cond) { printf("Test %d PASS: %s\n", test_num, msg); passed = passed + 1; }
    else      { printf("Test %d FAIL: %s\n", test_num, msg); failed = failed + 1; }
}

int returns_42(void) { return 42; }

int add(int a, int b) { return a + b; }

int main() {
    int t;
    int x;
    passed = 0;
    failed = 0;
    t = 1;

    // Test 1: (void) parameter list
    check(t++, returns_42() == 42, "(void) param function");

    // Test 2: init in declaration
    {
        int a = 10;
        check(t++, a == 10, "init-in-decl int a = 10");
    }

    // Test 3-4: block scoped variable
    x = 100;
    {
        int x = 200;
        check(t++, x == 200, "block x shadows outer x");
    }
    check(t++, x == 100, "outer x restored after block");

    // Test 5-7: nested block scopes
    x = 1;
    {
        int x = 2;
        {
            int x = 3;
            check(t++, x == 3, "inner nested x == 3");
        }
        check(t++, x == 2, "middle x == 2 after inner block");
    }
    check(t++, x == 1, "outer x == 1 after all blocks");

    // Test 8: init with expression
    {
        int a = 10;
        int b = 20;
        int c = a + b;
        check(t++, c == 30, "init c = a + b");
    }

    // Test 9: init with function call
    {
        int r = returns_42();
        check(t++, r == 42, "init r = returns_42()");
    }

    // Test 10: char init
    {
        char ch = 'Z';
        check(t++, ch == 'Z', "char init ch = Z");
    }

    // Test 11: pointer init
    {
        int val = 99;
        int *p = &val;
        check(t++, *p == 99, "pointer init *p == 99");
    }

    // Test 12: multiple declarations on one line
    {
        int a = 5, b = 6;
        check(t++, a + b == 11, "multi-decl a=5 b=6");
    }

    // Test 13-14: reuse variable name across sibling blocks
    {
        int z = 10;
        check(t++, z == 10, "sibling block 1 z=10");
    }
    {
        int z = 20;
        check(t++, z == 20, "sibling block 2 z=20");
    }

    // Test 15: non-void params still work
    check(t++, add(3, 4) == 7, "non-void params still work");

    // Test 16: function-top init
    {
        int ft = 77;
        check(t++, ft == 77, "function-top init ft = 77");
    }

    printf("SUMMARY: %d/%d tests passed\n", passed, passed + failed);
    return failed;
}
