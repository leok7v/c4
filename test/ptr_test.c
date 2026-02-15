#include <stdio.h>

int main()
{
  int x;
  int *p;
  
  x = 42;
  p = &x;
  
  printf("x=%d, *p=%d\n", x, *p);
  
  *p = 99;
  
  printf("After *p=99: x=%d\n", x);
  
  return 0;
}
