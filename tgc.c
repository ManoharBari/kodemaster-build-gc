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