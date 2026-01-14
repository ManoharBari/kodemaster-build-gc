#include <stdio.h>
#include "tgc.h"

static tgc_t gc;

int main(int argc, char **argv) {
    tgc_start(&gc, &argc);
    
    printf("Garbage Collector Started!\n");
    printf("Stack bottom: %p\n", gc.bottom);
    printf("Load factor: %.1f\n", gc.loadfactor);
    
    tgc_stop(&gc);
    printf("Garbage Collector Stopped!\n");
    
    return 0;
}
