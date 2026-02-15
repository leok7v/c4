struct Point { int x; int y; }; int main() {   struct Point p;   struct Point *pp;   pp = &p;   pp->x = 30;   pp->y = 40;   printf("Pointer(%d, %d)
", pp->x, pp->y);   return 0; }
