#ifndef TGC_H
#define TGC_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>

enum {
    TGC_MARK = 0x01,
    TGC_ROOT = 0x02,
    TGC_LEAF = 0x04
};

typedef struct {
    void *ptr;
    int flags;
    size_t size, hash;
    void (*dtor)(void*);
} tgc_ptr_t;

typedef struct {
    void *bottom;
    int paused;
    uintptr_t minptr, maxptr;
    tgc_ptr_t *items, *frees;
    double loadfactor, sweepfactor;
    size_t nitems, nslots, mitems, nfrees;
} tgc_t;

void *tgc_alloc(tgc_t *gc, size_t size);
void *tgc_alloc_opt(tgc_t *gc, size_t size, int flags, void(*dtor)(void*));

void tgc_start(tgc_t *gc, void *stk);
void tgc_stop(tgc_t *gc);
void tgc_pause(tgc_t *gc);
void tgc_resume(tgc_t *gc);
void tgc_run(tgc_t *gc);

#endif