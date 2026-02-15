#include <stdio.h>

int main(int argc, char **argv) {
    int x;
    int *p;
    x = 0;
    p = &x;
    *p = 42;
    printf("x = %d\n", x);
    return 0;
}
