
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


ML_Model *init_model(Arena *a)
{
  ML_Model *res = arena_alloc(a, sizeof(*res));
  make_slice(a, &res->inputs, 10);
  make_slice(a, &res->outputs, 10);

  return res;
}

int main(void)
{
  {
    usize size = 2 * MEGABYTE;
    u8 *mem = malloc(size);
    permanent_arena = arena_init_buffer(mem, size);
  }

  ML_Model *model = init_model(&permanent_arena);

  return 69;
}

int fds(void)
{
  return 420;
}

