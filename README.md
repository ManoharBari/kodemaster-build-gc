# Build Your Own Garbage Collector

A challenge to build a working garbage collector in C from scratch!

## Getting Started

```bash
# Build the project
make

# Run your program
./your_program
```

## Files

- `main.c` - Your demo/test program
- `tgc.h` - Header file with type definitions and function declarations
- `tgc.c` - Implementation file (you'll write the actual GC code here)
- `Makefile` - Build configuration

## Challenge Structure

Follow the challenge steps to implement:

1. **Hash table** for tracking allocations
2. **Allocation functions** (`tgc_alloc`, `tgc_calloc`, `tgc_realloc`, `tgc_free`)
3. **Mark phase** - stack scanning and pointer marking
4. **Sweep phase** - freeing unreachable memory
5. **Destructors and flags** for advanced features

Good luck! 🎉
