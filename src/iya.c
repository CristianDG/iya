
#define DG_CONTAINER_IMPLEMENTATION
#define DG_ALLOC_IMPLEMENTATION
#define DG_MATRIX_IMPLEMENTATION
#include "cdg_base.c"

#define Slice_Of_Slice(type) Make_Slice_Type(Make_Slice_Type(type))

typedef struct {
  u32 a;
  u32 b;
} Connection;

typedef struct {
  Make_Slice_Type(f32) inputs;
  Make_Slice_Type(f32) outputs;
  Slice_Of_Slice(f32) ws;
  Slice_Of_Slice(f32) bs;
  Slice_Of_Slice(Connection) connections;
} ML_Model;

Arena permanent_arena = {};

int main(void)
{
  {
    usize size = 2 * MEGABYTE;
    u8 *mem = malloc(size);
    permanent_arena = arena_init_buffer(mem, size);
  }

  return 69;
}

int fds(void) {
  return 420;
}

