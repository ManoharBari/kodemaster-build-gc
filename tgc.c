#include "tgc.h"

static size_t tgc_hash(void *ptr) {
    uintptr_t ad = (uintptr_t) ptr;
    return (size_t) ((13*ad) ^ (ad >> 15));
}

static size_t tgc_probe(tgc_t *gc, size_t i, size_t h) {
    long v = i - (h-1);
    if (v < 0) { v = gc->nslots + v; }
    return v;
}

static void tgc_add_ptr(
    tgc_t *gc, void *ptr, size_t size,
    int flags, void(*dtor)(void*)) {

    tgc_ptr_t item, tmp;
    size_t h, p, i, j;

    i = tgc_hash(ptr) % gc->nslots;
    j = 0;

    item.ptr = ptr;
    item.flags = flags;
    item.size = size;
    item.hash = i + 1;
    item.dtor = dtor;

    while (1) {
        h = gc->items[i].hash;
        if (h == 0) {
            gc->items[i] = item;
            return;
        }
        if (gc->items[i].ptr == item.ptr) {
            return; // Already exists
        }
        p = tgc_probe(gc, i, h);
        if (j >= p) {
            tmp = gc->items[i];
            gc->items[i] = item;
            item = tmp;
            j = p;
        }
        i = (i + 1) % gc->nslots;
        j++;
    }
}