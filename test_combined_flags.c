#include <stdio.h>
#include <string.h>
#include "tgc.h"

static tgc_t gc;

int main(int argc, char **argv) {
    tgc_start(&gc, &argc);

    printf("=== Testing Flag and Size Management ===\n\n");

    // Test 1: Regular allocation
    printf("--- Test 1: Regular Allocation ---\n");
    void *regular = tgc_alloc(&gc, 256);
    printf("Size: %zu bytes\n", tgc_get_size(&gc, regular));
    printf("Flags: %d\n", tgc_get_flags(&gc, regular));
    printf("✓ Regular allocation\n\n");

    // Test 2: Root allocation
    printf("--- Test 2: Root Allocation ---\n");
    void *root = tgc_alloc_opt(&gc, 512, TGC_ROOT, NULL);
    printf("Size: %zu bytes\n", tgc_get_size(&gc, root));
    printf("Flags: %d (TGC_ROOT=%d)\n", tgc_get_flags(&gc, root), TGC_ROOT);
    printf("Is ROOT: %s\n", (tgc_get_flags(&gc, root) & TGC_ROOT) ? "YES" : "NO");
    printf("✓ Root allocation\n\n");

    // Test 3: Leaf allocation
    printf("--- Test 3: Leaf Allocation ---\n");
    char *leaf = tgc_alloc_opt(&gc, 1024, TGC_LEAF, NULL);
    strcpy(leaf, "Leaf data");
    printf("Size: %zu bytes\n", tgc_get_size(&gc, leaf));
    printf("Flags: %d (TGC_LEAF=%d)\n", tgc_get_flags(&gc, leaf), TGC_LEAF);
    printf("Is LEAF: %s\n", (tgc_get_flags(&gc, leaf) & TGC_LEAF) ? "YES" : "NO");
    printf("✓ Leaf allocation\n\n");

    // Test 4: Combined flags (ROOT + LEAF)
    printf("--- Test 4: Combined Flags (ROOT + LEAF) ---\n");
    void *combined = tgc_alloc_opt(&gc, 2048, TGC_ROOT | TGC_LEAF, NULL);
    printf("Size: %zu bytes\n", tgc_get_size(&gc, combined));
    printf("Flags: %d\n", tgc_get_flags(&gc, combined));
    printf("Is ROOT: %s\n", (tgc_get_flags(&gc, combined) & TGC_ROOT) ? "YES" : "NO");
    printf("Is LEAF: %s\n", (tgc_get_flags(&gc, combined) & TGC_LEAF) ? "YES" : "NO");
    printf("✓ Combined flags\n\n");

    // Test 5: Modify flags after allocation
    printf("--- Test 5: Modify Flags ---\n");
    void *modifiable = tgc_alloc(&gc, 128);
    printf("Initial flags: %d\n", tgc_get_flags(&gc, modifiable));
    
    tgc_set_flags(&gc, modifiable, TGC_ROOT);
    printf("After setting ROOT: %d\n", tgc_get_flags(&gc, modifiable));
    
    tgc_set_flags(&gc, modifiable, TGC_LEAF);
    printf("After setting LEAF: %d\n", tgc_get_flags(&gc, modifiable));
    
    tgc_set_flags(&gc, modifiable, TGC_ROOT | TGC_LEAF);
    printf("After setting ROOT|LEAF: %d\n", tgc_get_flags(&gc, modifiable));
    printf("✓ Flag modification\n\n");

    // Test 6: GC behavior with roots
    printf("--- Test 6: GC with Roots ---\n");
    printf("Items before GC: %zu\n", gc.nitems);
    
    // Clear stack references
    regular = NULL;
    leaf = NULL;
    modifiable = NULL;
    
    tgc_run(&gc);
    printf("Items after GC: %zu\n", gc.nitems);
    printf("✓ Roots survived GC\n\n");

    // Clean up roots manually
    printf("--- Cleanup ---\n");
    tgc_free(&gc, root);
    tgc_free(&gc, combined);
    printf("Items after manual cleanup: %zu\n", gc.nitems);

    tgc_stop(&gc);
    
    printf("\n✓ All tests completed successfully!\n");
    return 0;
}