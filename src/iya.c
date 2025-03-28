
#define DG_CONTAINER_IMPLEMENTATION
#define DG_ALLOC_IMPLEMENTATION
#define DG_MATRIX_IMPLEMENTATION
#include "cdg_base.c"


Arena permanent_arena = {};

static u32 _scratch_idx = 0;
#define SCRATCH_SIZE 2
Arena scratch_arena[SCRATCH_SIZE] = {};
#define GetScratch() &scratch_arena[_scratch_idx++ % SCRATCH_SIZE]


#define TempGuard(arena) \
  for (Temporary_Arena_Memory __t_a_m = temp_arena_memory_begin(arena) \
  ; __t_a_m.arena \
  ; temp_arena_memory_end(&__t_a_m))

#define WithScratch(name) \
  for (Temporary_Arena_Memory name = temp_arena_memory_begin(GetScratch()) \
  ; name.arena \
  ; temp_arena_memory_end(&name))

typedef struct {
  // TODO: backing arena
  DG_DAG nodes;
} ML_Model;

ML_Model *init_model(Arena *a)
{

  ML_Model *result = arena_alloc(a, sizeof(*result));

  const u32 inputs = 4;
  const u32 outputs = 1;

  DG_DAG *output = 0;
  for (u32 i = 0; i < inputs; ++i) {
    DG_DAG *child = dag_add_child(a, &result->nodes, i, 1);
    if (i == 2) {
      DG_DAG *child2 = dag_add_child(a, child, 22, 1);
      dag_connect_child(a, child2, output, 1);
    } else {
      if (output) {
        dag_connect_child(a, child, output, 1);
      } else {
        output = dag_add_child(a, child, 69, 1);
      }
    }
  }

  return result;
}

#define TRAVERSE_MODEL_FN(name) void name(f32 *value, u32 layer)
typedef TRAVERSE_MODEL_FN(traverse_model_fn);

void traverse_model(ML_Model *model, traverse_model_fn *fn)
{
  WithScratch(scratch)
  {
    Make_Dynamic_Array_Type(DG_DAG *) nodes;
    make_dynamic_array(&nodes, scratch.arena, 32);

    Make_Dynamic_Array_Type(DG_DAG *) nodes_to_traverse;
    make_dynamic_array(&nodes_to_traverse, scratch.arena, 32);

    DG_DAG *node = &model->nodes;
    do {
      for (u32 i = 0; i < node->children.len; ++i) {
        DG_DAG *child = SLICE_AT(node->children, i).child;
        if (!child->visited) {
          child->visited = true;
          dynamic_array_push(&nodes_to_traverse, child, scratch.arena);
          dynamic_array_push(&nodes, child, scratch.arena);
        }
      }
      dynamic_array_pop(&nodes_to_traverse, &node);
    } while (nodes_to_traverse.len);

    for (u32 i = 0; i < nodes.len; ++i) {
      DG_DAG *child = SLICE_AT(nodes, i);
      child->visited = 0;

      fn(&child->value, child->layer);
    }

  }
}

TRAVERSE_MODEL_FN(traverse_print) {
  console_log_number(*value);
  console_log_number(layer);
  console_log_number(420);
}

int main(void)
{
  {
    usize size = 10 * KILOBYTE;
    permanent_arena = arena_init_buffer(malloc(size), size);

    scratch_arena[0] = arena_init_buffer(malloc(size), size);
    scratch_arena[1] = arena_init_buffer(malloc(size), size);
  }

  WithScratch(guard) {
    Arena *scratch = guard.arena;

    ML_Model *model = init_model(scratch);
    traverse_model(model, traverse_print);
  }


  return 69;
}

int fds(void)
{
  return 420;
}

