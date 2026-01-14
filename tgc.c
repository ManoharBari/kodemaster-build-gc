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

enum {
    TGC_PRIMES_COUNT = 24
};

static const size_t tgc_primes[TGC_PRIMES_COUNT] = {
    0,       1,       5,       11,
    23,      53,      101,     197,
    389,     683,     1259,    2417,
    4733,    9371,    18617,   37097,
    74093,   148073,  296099,  592019,
    1100009, 2200013, 4400021, 8800019
};

static size_t tgc_ideal_size(tgc_t *gc, size_t size) {
    size_t i, last;
    size = (size_t)((double)(size + 1) / gc->loadfactor);
    for (i = 0; i < TGC_PRIMES_COUNT; i++) {
        if (tgc_primes[i] >= size) {
            return tgc_primes[i];
        }
    }
    last = tgc_primes[TGC_PRIMES_COUNT - 1];
    for (i = 0;; i++) {
        if (last * i >= size) {
            return last * i;
        }
    }
    return 0;
}

static int tgc_rehash(tgc_t *gc, size_t new_size) {
    size_t i;
    tgc_ptr_t *old_items = gc->items;
    size_t old_size = gc->nslots;

    gc->nslots = new_size;
    gc->items = calloc(gc->nslots, sizeof(tgc_ptr_t));

    if (gc->items == NULL) {
        gc->nslots = old_size;
        gc->items = old_items;
        return 0;
    }

    for (i = 0; i < old_size; i++) {
        if (old_items[i].hash != 0) {
            tgc_add_ptr(gc,
                old_items[i].ptr, old_items[i].size,
                old_items[i].flags, old_items[i].dtor);
        }
    }

    free(old_items);
    return 1;
}

static int tgc_resize_more(tgc_t *gc) {
    size_t new_size = tgc_ideal_size(gc, gc->nitems);
    size_t old_size = gc->nslots;
    return (new_size > old_size) ? tgc_rehash(gc, new_size) : 1;
}

static int tgc_resize_less(tgc_t *gc) {
    size_t new_size = tgc_ideal_size(gc, gc->nitems);
    size_t old_size = gc->nslots;
    return (new_size < old_size) ? tgc_rehash(gc, new_size) : 1;
}

void tgc_start(tgc_t *gc, void *stk) {
    gc->bottom = stk;
    gc->paused = 0;
    gc->nitems = 0;
    gc->nslots = 0;
    gc->mitems = 0;
    gc->nfrees = 0;
    gc->maxptr = 0;
    gc->items = NULL;
    gc->frees = NULL;
    gc->minptr = UINTPTR_MAX;
    gc->loadfactor = 0.9;
    gc->sweepfactor = 0.5;
}

void tgc_stop(tgc_t *gc) {
    free(gc->items);
    free(gc->frees);
}

void tgc_pause(tgc_t *gc) {
    gc->paused = 1;
}

void tgc_resume(tgc_t *gc) {
    gc->paused = 0;
}

void tgc_run(tgc_t *gc) {
    // Will be: tgc_mark(gc); tgc_sweep(gc);
}

static void *tgc_add(
    tgc_t *gc, void *ptr, size_t size,
    int flags, void(*dtor)(void*)) {

    gc->nitems++;
    gc->maxptr = ((uintptr_t)ptr) + size > gc->maxptr ?
        ((uintptr_t)ptr) + size : gc->maxptr;
    gc->minptr = ((uintptr_t)ptr) < gc->minptr ?
        ((uintptr_t)ptr) : gc->minptr;

    if (tgc_resize_more(gc)) {
        tgc_add_ptr(gc, ptr, size, flags, dtor);
        if (!gc->paused && gc->nitems > gc->mitems) {
            tgc_run(gc);
        }
        return ptr;
    } else {
        gc->nitems--;
        free(ptr);
        return NULL;
    }
}

void *tgc_alloc(tgc_t *gc, size_t size) {
    return tgc_alloc_opt(gc, size, 0, NULL);
}

void *tgc_alloc_opt(tgc_t *gc, size_t size, int flags, void(*dtor)(void*)) {
    void *ptr = malloc(size);
    if (ptr != NULL) {
        ptr = tgc_add(gc, ptr, size, flags, dtor);
    }
    return ptr;
}