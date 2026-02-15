struct Point { int x; int y; };

int main()
{
  struct Point pts[3];
  struct Point *pp;
  int *ip;
  int a[4];

  // Setup struct array
  pts[0].x = 1; pts[0].y = 2;
  pts[1].x = 3; pts[1].y = 4;
  pts[2].x = 5; pts[2].y = 6;

  // Struct pointer arithmetic: pp + 1
  pp = pts;
  printf("pp->x=%d pp->y=%d\n", pp->x, pp->y);
  pp = pp + 1;
  printf("pp+1: x=%d y=%d\n", pp->x, pp->y);
  pp = pp + 1;
  printf("pp+2: x=%d y=%d\n", pp->x, pp->y);

  // Pre-increment struct pointer
  pp = pts;
  ++pp;
  printf("++pp: x=%d y=%d\n", pp->x, pp->y);

  // Post-increment struct pointer
  pp = pts;
  pp++;
  printf("pp++: x=%d y=%d\n", pp->x, pp->y);

  // Pointer subtraction (distance)
  pp = pts + 2;
  printf("pp-pts=%d\n", (int)(pp - pts));

  // Scalar int pre/post increment
  a[0] = 10; a[1] = 20; a[2] = 30; a[3] = 40;
  ip = a;
  printf("*ip=%d\n", *ip);
  ++ip;
  printf("++ip: *ip=%d\n", *ip);
  ip++;
  printf("ip++: *ip=%d\n", *ip);

  // Scalar variable inc/dec
  a[0] = 100;
  ++a[0];
  printf("++a[0]=%d\n", a[0]);
  a[0]++;
  printf("a[0]++=%d\n", a[0]);

  printf("All struct pointer arith tests passed!\n");
  return 0;
}
