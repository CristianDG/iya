#define DG_CRASH() __builtin_trap()

#define CDG_MATH_H
#include "cdg_base.c"

#define DG_MEMSET(ptr, val, size) 0
#define DG_MEMCPY(dst, src, size) 0
#define DG_LOG_ERROR(args...)
#define DG_LOG(args...)


#define DG_CONTAINER_IMPLEMENTATION
#define DG_ALLOC_IMPLEMENTATION
#define DG_MATRIX_IMPLEMENTATION
#include "cdg_base.c"

// implementation from https://surma.dev/things/c-to-webassembly/
extern u8 __heap_base;
u8 *bump_pointer = &__heap_base;

void* malloc(usize n) {
  u8 *r = bump_pointer;
  bump_pointer += n;
  return r;
}

void free(void* p) {
  // lol
}

Arena permanent_arena = {};

extern int main(void)
{
  {
    usize size = 2 * KILOBYTE;
    u8 *mem = malloc(size);
    permanent_arena = arena_init_buffer(mem, size);
  }

  DG_CRASH();

  return 69;
}

extern int fds(void) {
  return 420;
}
