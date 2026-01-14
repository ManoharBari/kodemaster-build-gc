#include <stdio.h>
#include "tgc.h"

static tgc_t gc;

void create_garbage() {
    // Allocate memory that becomes unreachable
    for (int i = 0; i < 100; i++) {
        tgc_alloc(&gc, 1000);
    }
    // All 100 allocations are garbage after this function returns!
}

int main(int argc, char **argv) {
    tgc_start(&gc, &argc);

    printf("Initial: %zu allocations\n", gc.nitems);

    create_garbage();
    printf("After garbage: %zu allocations\n", gc.nitems);

    // Force a collection
    tgc_run(&gc);
    printf("After GC: %zu allocations\n", gc.nitems);

    // Keep one alive
    int *keep = tgc_alloc(&gc, 100);
    *keep = 42;

    create_garbage();
    printf("More garbage: %zu allocations\n", gc.nitems);

    tgc_run(&gc);
    printf("After GC (keep=%d): %zu allocations\n", *keep, gc.nitems);

    tgc_stop(&gc);
    printf("Done!\n");

    return 0;
}