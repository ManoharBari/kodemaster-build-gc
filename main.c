#include "tgc.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>

/* ================= FORWARD DECLARATIONS ================= */


static void tgc_mark(tgc_t *gc);
void tgc_sweep(tgc_t *gc);


/* ======================================================== */

static size_t tgc_hash(void *ptr)
{
    uintptr_t ad = (uintptr_t)ptr;
    return (size_t)((13 * ad) ^ (ad >> 15));
}

static size_t tgc_probe(tgc_t *gc, size_t i, size_t h)
{
    long v = (long)i - ((long)h - 1);
    if (v < 0)
    {
        v = (long)gc->nslots + v;
    }
    return (size_t)v;
}

static void tgc_add_ptr(
    tgc_t *gc, void *ptr, size_t size,
    int flags, void (*dtor)(void *))
{
    tgc_ptr_t item, tmp;
    size_t h, p, i, j;

    i = tgc_hash(ptr) % gc->nslots;
    j = 0;

    item.ptr = ptr;
    item.flags = flags;
    item.size = size;
    item.hash = i + 1;
    item.dtor = dtor;

    while (1)
    {
        h = gc->items[i].hash;
        if (h == 0)
        {
            gc->items[i] = item;
            return;
        }
        if (gc->items[i].ptr == item.ptr)
        {
            return;
        }
        p = tgc_probe(gc, i, h);
        if (j >= p)
        {
            tmp = gc->items[i];
            gc->items[i] = item;
            item = tmp;
            j = p;
        }
        i = (i + 1) % gc->nslots;
        j++;
    }
}

static tgc_ptr_t *tgc_get_ptr(tgc_t *gc, void *ptr)
{
    size_t i, j, h;
    i = tgc_hash(ptr) % gc->nslots;
    j = 0;
    while (1)
    {
        h = gc->items[i].hash;
        if (h == 0 || j > tgc_probe(gc, i, h))
        {
            return NULL;
        }
        if (gc->items[i].ptr == ptr)
        {
            return &gc->items[i];
        }
        i = (i + 1) % gc->nslots;
        j++;
    }
}

static void tgc_rem_ptr(tgc_t *gc, void *ptr)
{
    size_t i, j, h, nj, nh;

    if (gc->nitems == 0)
        return;

    i = tgc_hash(ptr) % gc->nslots;
    j = 0;

    while (1)
    {
        h = gc->items[i].hash;
        if (h == 0 || j > tgc_probe(gc, i, h))
            return;

        if (gc->items[i].ptr == ptr)
        {
            memset(&gc->items[i], 0, sizeof(tgc_ptr_t));
            j = i;
            while (1)
            {
                nj = (j + 1) % gc->nslots;
                nh = gc->items[nj].hash;
                if (nh != 0 && tgc_probe(gc, nj, nh) > 0)
                {
                    memcpy(&gc->items[j], &gc->items[nj], sizeof(tgc_ptr_t));
                    memset(&gc->items[nj], 0, sizeof(tgc_ptr_t));
                    j = nj;
                }
                else
                {
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

/* ===================== GC CORE ===================== */

void tgc_start(tgc_t *gc, void *stk)
{
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

void tgc_stop(tgc_t *gc)
{
    tgc_sweep(gc);
    free(gc->items);
    free(gc->frees);
}

void tgc_pause(tgc_t *gc)
{
    gc->paused = 1;
}

void tgc_resume(tgc_t *gc)
{
    gc->paused = 0;
}

void tgc_run(tgc_t *gc)
{
    tgc_mark(gc);
    tgc_sweep(gc);
}

/* ===================== MARK PHASE ===================== */

static void tgc_mark_ptr(tgc_t *gc, void *ptr)
{
    size_t i, j, h, k;

    if ((uintptr_t)ptr < gc->minptr ||
        (uintptr_t)ptr > gc->maxptr)
        return;

    i = tgc_hash(ptr) % gc->nslots;
    j = 0;

    while (1)
    {
        h = gc->items[i].hash;
        if (h == 0 || j > tgc_probe(gc, i, h))
            return;

        if (ptr == gc->items[i].ptr)
        {
            if (gc->items[i].flags & TGC_MARK)
                return;

            gc->items[i].flags |= TGC_MARK;

            if (gc->items[i].flags & TGC_LEAF)
                return;

            for (k = 0; k < gc->items[i].size / sizeof(void *); k++)
                tgc_mark_ptr(gc, ((void **)gc->items[i].ptr)[k]);
            return;
        }
        i = (i + 1) % gc->nslots;
        j++;
    }
}

static void tgc_mark_stack(tgc_t *gc)
{
    void *stk, *bot, *top, *p;
    bot = gc->bottom;
    top = &stk;

    if (bot < top)
    {
        for (p = top; p >= bot; p = (char *)p - sizeof(void *))
            tgc_mark_ptr(gc, *((void **)p));
    }
    else
    {
        for (p = top; p <= bot; p = (char *)p + sizeof(void *))
            tgc_mark_ptr(gc, *((void **)p));
    }
}

static void tgc_mark(tgc_t *gc)
{
    size_t i, k;
    jmp_buf env;

    if (gc->nitems == 0)
        return;

    for (i = 0; i < gc->nslots; i++)
    {
        if (gc->items[i].hash == 0)
            continue;

        if (gc->items[i].flags & TGC_MARK)
            continue;

        if (gc->items[i].flags & TGC_ROOT)
        {
            gc->items[i].flags |= TGC_MARK;
            if (!(gc->items[i].flags & TGC_LEAF))
            {
                for (k = 0; k < gc->items[i].size / sizeof(void *); k++)
                    tgc_mark_ptr(gc, ((void **)gc->items[i].ptr)[k]);
            }
        }
    }

    memset(&env, 0, sizeof(env));
    setjmp(env);

    tgc_mark_stack(gc);
}

/* ===================== SWEEP PHASE ===================== */

void tgc_sweep(tgc_t *gc)
{
    size_t i, j, k, nj, nh;

    if (gc->nitems == 0)
        return;

    gc->nfrees = 0;

    for (i = 0; i < gc->nslots; i++)
    {
        if (gc->items[i].hash &&
            !(gc->items[i].flags & (TGC_MARK | TGC_ROOT)))
            gc->nfrees++;
    }

    gc->frees = realloc(gc->frees, sizeof(tgc_ptr_t) * gc->nfrees);
    if (!gc->frees)
        return;

    i = 0;
    k = 0;

    while (i < gc->nslots)
    {
        if (gc->items[i].hash &&
            !(gc->items[i].flags & (TGC_MARK | TGC_ROOT)))
        {
            gc->frees[k++] = gc->items[i];
            memset(&gc->items[i], 0, sizeof(tgc_ptr_t));

            j = i;
            while (1)
            {
                nj = (j + 1) % gc->nslots;
                nh = gc->items[nj].hash;
                if (nh && tgc_probe(gc, nj, nh) > 0)
                {
                    memcpy(&gc->items[j], &gc->items[nj], sizeof(tgc_ptr_t));
                    memset(&gc->items[nj], 0, sizeof(tgc_ptr_t));
                    j = nj;
                }
                else
                    break;
            }
            gc->nitems--;
        }
        else
            i++;
    }

    for (i = 0; i < gc->nslots; i++)
        gc->items[i].flags &= ~TGC_MARK;

    for (i = 0; i < gc->nfrees; i++)
    {
        if (gc->frees[i].dtor)
            gc->frees[i].dtor(gc->frees[i].ptr);
        free(gc->frees[i].ptr);
    }

    free(gc->frees);
    gc->frees = NULL;
    gc->nfrees = 0;
}
