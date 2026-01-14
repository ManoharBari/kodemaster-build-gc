#include <stdio.h>
#include "tgc.h"

static tgc_t gc;

int main(int argc, char **argv) {
    tgc_start(&gc, &argc);

    // Allocate some memory!
    char *str = tgc_alloc(&gc, 32);
    if (str) {
        strcpy(str, "Hello, Garbage Collector!");
        printf("%s\n", str);
    }

    int *numbers = tgc_alloc(&gc, sizeof(int) * 10);
    if (numbers) {
        for (int i = 0; i < 10; i++) {
            numbers[i] = i * i;
        }
        printf("Squares: ");
        for (int i = 0; i < 10; i++) {
            printf("%d ", numbers[i]);
        }
        printf("\n");
    }

    printf("Allocations tracked: %zu\n", gc.nitems);

    tgc_stop(&gc);
    return 0;
}