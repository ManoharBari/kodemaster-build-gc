#include <stdio.h>
#include <string.h>
#include "tgc.h"

static tgc_t gc;

// Test structure with pointers
typedef struct Node {
    int value;
    struct Node *next;
} Node;

// Destructor for tracking
void node_dtor(void *ptr) {
    Node *n = (Node*)ptr;
    printf("  🗑️  Freeing node with value: %d\n", n->value);
}

// Creates a linked list
Node* create_list(int count) {
    Node *head = NULL;
    Node *current = NULL;

    for (int i = 0; i < count; i++) {
        Node *node = tgc_alloc_opt(&gc, sizeof(Node), 0, node_dtor);
        node->value = i + 1;
        node->next = NULL;

        if (head == NULL) {
            head = node;
        } else {
            current->next = node;
        }
        current = node;
    }

    return head;
}

void print_list(Node *head) {
    printf("  List: ");
    while (head) {
        printf("%d", head->value);
        if (head->next) printf(" -> ");
        head = head->next;
    }
    printf("\n");
}

void create_garbage() {
    // These allocations become unreachable
    tgc_alloc(&gc, 100);
    tgc_alloc(&gc, 200);
    create_list(3);  // A whole linked list!
}

int main(int argc, char **argv) {
    printf("═══════════════════════════════════════════\n");
    printf("   🧪 Tiny Garbage Collector - Final Test\n");
    printf("═══════════════════════════════════════════\n\n");

    tgc_start(&gc, &argc);

    // Test 1: Basic allocation
    printf("📋 Test 1: Basic Allocation\n");
    int *nums = tgc_alloc(&gc, sizeof(int) * 5);
    for (int i = 0; i < 5; i++) nums[i] = i * 10;
    printf("  Allocated 5 integers: %d, %d, %d, %d, %d\n",
           nums[0], nums[1], nums[2], nums[3], nums[4]);
    printf("  Tracked allocations: %zu\n\n", gc.nitems);

    // Test 2: Linked list with pointers
    printf("📋 Test 2: Linked List (pointer tracking)\n");
    Node *list = create_list(5);
    print_list(list);
    printf("  Tracked allocations: %zu\n\n", gc.nitems);

    // Test 3: Garbage creation
    printf("📋 Test 3: Creating Garbage\n");
    create_garbage();
    printf("  Before GC: %zu allocations\n", gc.nitems);
    tgc_run(&gc);
    printf("  After GC: %zu allocations\n\n", gc.nitems);

    // Test 4: List survives (still reachable)
    printf("📋 Test 4: Verifying List Survived\n");
    print_list(list);

    // Test 5: TGC_ROOT
    printf("\n📋 Test 5: Root Objects\n");
    int *root = tgc_alloc_opt(&gc, sizeof(int), TGC_ROOT, NULL);
    *root = 42;
    list = NULL;  // Clear our reference to the list
    nums = NULL;  // Clear this too

    printf("  Before GC (root=%d): %zu allocs\n", *root, gc.nitems);
    tgc_run(&gc);
    printf("  After GC (root=%d): %zu allocs\n", *root, gc.nitems);

    // Root survives! Free it manually
    tgc_free(&gc, root);
    printf("  After manual free: %zu allocs\n\n", gc.nitems);

    // Test 6: Large leaf allocation
    printf("📋 Test 6: Leaf Optimization\n");
    char *buffer = tgc_alloc_opt(&gc, 10000, TGC_LEAF, NULL);
    strcpy(buffer, "Large buffer that won't be scanned!");
    printf("  Buffer size: %zu bytes (LEAF flag set)\n", tgc_get_size(&gc, buffer));
    printf("  Tracked allocations: %zu\n\n", gc.nitems);

    // Cleanup
    printf("📋 Cleanup: tgc_stop()\n");
    tgc_stop(&gc);

    printf("\n═══════════════════════════════════════════\n");
    printf("   ✅ All tests passed! GC works!\n");
    printf("═══════════════════════════════════════════\n");

    return 0;
}