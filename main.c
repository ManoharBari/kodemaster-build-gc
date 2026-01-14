#include <stdio.h>
#include "tgc.h"

static tgc_t gc;

int main(int argc, char **argv) {
    tgc_start(&gc, &argc);

    // Allocate zeroed memory
    int *arr = tgc_calloc(&gc, 10, sizeof(int));
    
    printf("Zeroed array: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", arr[i]);  // Should all be 0
    }
    printf("\n");

    // Allocate zeroed buffer
    char *buffer = tgc_calloc(&gc, 100, 1);
    printf("Buffer length: %zu (should be 0)\n", strlen(buffer));

    tgc_stop(&gc);
    return 0;
}