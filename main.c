#include <stdio.h>
#include "tgc.h"

int main(int argc, char **argv) {
    int x = 42;
    int y = 43;
    
    printf("Address of x: %p\n", (void*)&x);
    printf("Address of y: %p\n", (void*)&y);
    printf("Garbage Collector - Hash function ready!\n");
    
    return 0;
}