#include <stdio.h>
#include "tgc.h"

static tgc_t gc;

int main(int argc, char **argv) {
    tgc_start(&gc, &argc);

    // Start with small array
    int *arr = tgc_alloc(&gc, sizeof(int) * 2);
    arr[0] = 10;
    arr[1] = 20;
    printf("Original: %d %d (items: %zu)\n", arr[0], arr[1], gc.nitems);

    // Grow it
    arr = tgc_realloc(&gc, arr, sizeof(int) * 5);
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;
    printf("Grown: %d %d %d %d %d (items: %zu)\n",
           arr[0], arr[1], arr[2], arr[3], arr[4], gc.nitems);

    // Shrink it
    arr = tgc_realloc(&gc, arr, sizeof(int) * 3);
    printf("Shrunk: %d %d %d (items: %zu)\n",
           arr[0], arr[1], arr[2], gc.nitems);

    tgc_stop(&gc);
    return 0;
}