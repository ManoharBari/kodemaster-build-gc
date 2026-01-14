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

static tgc_ptr_t *tgc_get_ptr(tgc_t *gc, void *ptr) {
    size_t i, j, h;
    i = tgc_hash(ptr) % gc->nslots;
    j = 0;
    while (1) {
        h = gc->items[i].hash;
        if (h == 0 || j > tgc_probe(gc, i, h)) {
            return NULL;  // Not found
        }
        if (gc->items[i].ptr == ptr) {
            return &gc->items[i];  // Found!
        }
        i = (i + 1) % gc->nslots;
        j++;
    }
    return NULL;
}

static void tgc_rem_ptr(tgc_t *gc, void *ptr) {
    size_t i, j, h, nj, nh;

    if (gc->nitems == 0) { return; }

    for (i = 0; i < gc->nfrees; i++) {
        if (gc->frees[i].ptr == ptr) {
            gc->frees[i].ptr = NULL;
        }
    }

    i = tgc_hash(ptr) % gc->nslots;
    j = 0;

    while (1) {
        h = gc->items[i].hash;
        if (h == 0 || j > tgc_probe(gc, i, h)) {
            return;  // Not found
        }
        if (gc->items[i].ptr == ptr) {
            memset(&gc->items[i], 0, sizeof(tgc_ptr_t));
            j = i;
            // Backward shift
            while (1) {
                nj = (j + 1) % gc->nslots;
                nh = gc->items[nj].hash;
                if (nh != 0 && tgc_probe(gc, nj, nh) > 0) {
                    memcpy(&gc->items[j], &gc->items[nj], sizeof(tgc_ptr_t));
                    memset(&gc->items[nj], 0, sizeof(tgc_ptr_t));
                    j = nj;
                } else {
                    break;
                }
            }
            gc->nitems--;
            return;
        }
        i = (i + 1) % gc->nslots;
        j++;
    }
}