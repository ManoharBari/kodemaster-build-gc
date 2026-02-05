#include <stdio.h>
#include <string.h>
#include "tgc.h"

static tgc_t gc;

int main(int argc, char **argv) {
    tgc_start(&gc, &argc);

    printf("=== Testing TGC_LEAF Flag ===\n\n");

    // Large buffer as leaf (no scanning needed)
    char *buffer = tgc_alloc_opt(&gc, 100000, TGC_LEAF, NULL);
    strcpy(buffer, "This is a large string that won't be scanned for pointers!");

    printf("✓ Created large leaf buffer\n");
    printf("Buffer size: %zu bytes\n", tgc_get_size(&gc, buffer));
    printf("Buffer flags: %d (LEAF=%d)\n",
           tgc_get_flags(&gc, buffer), TGC_LEAF);
    printf("Is LEAF set: %s\n", 
           (tgc_get_flags(&gc, buffer) & TGC_LEAF) ? "YES" : "NO");
    
    printf("\nBuffer content: %.60s...\n", buffer);

    tgc_stop(&gc);
    
    printf("\n✓ Test completed successfully!\n");
    return 0;
}