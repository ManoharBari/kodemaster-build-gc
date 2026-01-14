#include <stdio.h>
#include "tgc.h"

static tgc_t gc;

void cleanup(void *ptr) {
    printf("🧹 Cleaning up %p\n", ptr);
}

int main(int argc, char **argv) {
    tgc_start(&gc, &argc);

    // Allocate with destructor
    void *data = tgc_alloc_opt(&gc, 100, 0, cleanup);
    printf("Allocated: %p\n", data);

    // Free triggers destructor
    tgc_free(&gc, data);
    printf("Done!\n");

    tgc_stop(&gc);
    return 0;
}