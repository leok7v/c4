#include <stdio.h>

struct Point {
    int x;
    int y;
};

int main(int argc, char **argv){
    struct Point p;
    p.x = 10;
    p.y = 20;   
    printf("Point(%d, %d)", p.x, p.y);       
    return 0; 
}
