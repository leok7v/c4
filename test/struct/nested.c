struct A { int a; char b; }; struct B { char c; struct A inner; }; int main() {   struct B b;   b.c = 'X';   b.inner.a = 50;   b.inner.b = 'Y';   printf("Nested(%c, %d, %c)
", b.c, b.inner.a, b.inner.b);   return 0; }
