#include <stdio.h>
#include "tgc.h"

static tgc_t gc;

int main(int argc, char **argv) {
    tgc_start(&gc, &argc);

    printf("=== Testing TGC_ROOT Flag ===\n\n");

    // Regular allocation
    void *regular = tgc_alloc(&gc, 100);
    printf("✓ Created regular allocation\n");

    // Root allocation
    void *root = tgc_alloc_opt(&gc, 100, TGC_ROOT, NULL);
    printf("✓ Created root allocation\n");

    printf("\nBefore GC: %zu items\n", gc.nitems);

    // Clear our stack references
    regular = NULL;
    printf("✓ Cleared regular reference from stack\n");

    printf("\nRunning GC...\n");
    tgc_run(&gc);
    
    printf("After GC: %zu items\n", gc.nitems);
    
    int root_flags = tgc_get_flags(&gc, root);
    printf("Root flags: %d\n", root_flags);
    printf("Root still exists: %s\n", (root_flags & TGC_ROOT) ? "YES" : "NO");

    // Must manually free roots!
    printf("\nManually freeing root...\n");
    tgc_free(&gc, root);
    printf("After free: %zu items\n", gc.nitems);

    tgc_stop(&gc);
    
    printf("\n✓ Test completed successfully!\n");
    return 0;
}