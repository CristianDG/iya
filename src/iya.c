
#define DG_CONTAINER_IMPLEMENTATION
#define DG_ALLOC_IMPLEMENTATION
#define DG_MATRIX_IMPLEMENTATION
#include "cdg_base.c"

Arena permanent_arena = {};

int main(void)
{
  {
    usize size = 2 * KILOBYTE;
    u8 *mem = malloc(size);
    permanent_arena = arena_init_buffer(mem, size);
  }


  return 69;
}

int fds(void) {
  return 420;
}

