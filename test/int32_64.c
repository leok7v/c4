// Test int32_t and int64_t support in c4
#include <stdio.h>

int main()
{
  int32_t a32;
  int64_t b64;
  char c8;
  int32_t *p32;
  int64_t *p64;
  int32_t x;
  int64_t y;
  
  // Test basic assignment  
  a32 = 42;
  b64 = 123456789;
  c8 = 'X';
  
  printf("int32_t: %d\n", a32);
  printf("int64_t: %d\n", b64);
  printf("char: %c\n", c8);
  
  // Test sizeof
  printf("sizeof(char)=%d\n", sizeof(char));
  printf("sizeof(int32_t)=%d\n", sizeof(int32_t));
  printf("sizeof(int64_t)=%d\n", sizeof(int64_t));
  
  // Test arithmetic
  a32 = 100 + 50;
  b64 = a32 * 2;
  printf("a32=%d, b64=%d\n", a32, b64);
  
  // Test increment
  a32++;
  b64--;
  printf("After ++/--: a32=%d, b64=%d\n", a32, b64);
  
  // Test pointers
  p32 = &a32;
  p64 = &b64;
  
  printf("*p32=%d, *p64=%d\n", *p32, *p64);
  
  *p32 = 999;
  *p64 = 888;
  
  printf("After store: a32=%d, b64=%d\n", a32, b64);
  
  printf("All tests passed!\n");
  
  return 0;
}
