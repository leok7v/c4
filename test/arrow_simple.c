#include <stdio.h>
struct S { int a; };

int main(int argc, char **argv) {
    struct S s;
    struct S *ps;
    ps = &s;
    ps->a = 99;
    printf("s.a = %d\n", s.a);
    return 0;
}
