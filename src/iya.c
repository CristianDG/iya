
#define DG_CONTAINER_IMPLEMENTATION
#define DG_ALLOC_IMPLEMENTATION
#define DG_MATRIX_IMPLEMENTATION
#define DG_SOFTWARE_RENDERER_IMPLEMENTATION
#include "cdg_base.c"

// NOTE: platform independent but no `iya` dependent {{{

Arena permanent_arena = {};
u8 permanent_arena_buffer[2 * MEGABYTE];

static u32 _scratch_idx = 0;
#define SCRATCH_SIZE 2
Arena scratch_arena[SCRATCH_SIZE] = {};
#define GetScratch() temp_arena_memory_begin(&scratch_arena[_scratch_idx++ % SCRATCH_SIZE])
#define ReleaseScratch(guard) temp_arena_memory_end(&guard)

#define WithScratch(name) \
  for (Temp_Arena_Memory name = GetScratch() \
  ; name.arena \
  ; temp_arena_memory_end(&name))

// }}}

typedef struct {
  // TODO: backing arena
  DG_DAG nodes;
  u32 layers;
} ML_Model;

usize dag_slice_partition(DG_DAG **arr, usize low, usize high)
{
  DG_DAG *pivot = arr[high];
  int i = (low - 1);

  for (int j = low; j <= high - 1; j++) {
    if (arr[j]->layer <= pivot->layer) {
      i++;
      DG_SWAP(DG_DAG *, &arr[i], &arr[j]);
    }
  }
  DG_SWAP(DG_DAG *, &arr[i + 1], &arr[high]);
  return (i + 1);
}

void dag_slice_quicksort(DG_DAG **arr, usize low, usize high)
{
  if (low < high) {
    usize partition_index = dag_slice_partition(arr, low, high);

    dag_slice_quicksort(arr, low, partition_index - 1);
    dag_slice_quicksort(arr, partition_index + 1, high);
  }
}

ML_Model *init_model(Arena *a)
{
  ML_Model *result = arena_alloc(a, sizeof(*result));

  const u32 inputs = 4;
  const u32 outputs = 1;


  DG_DAG *output = 0;
  for (u32 i = 0; i < inputs; ++i) {

    const f32 weight = 1;
    const f32 bias = 1;

    DG_DAG *child = dag_add_child(a, &result->nodes, i, bias, weight);
    if (i == 2) {
      DG_DAG *child2 = dag_add_child(a, child, 22, bias, weight);
      dag_connect_child(a, child2, output, weight);
    } else {
      if (output) {
        dag_connect_child(a, child, output, weight);
      } else {
        output = dag_add_child(a, child, 69, bias, weight);
      }
    }
  }

  return result;
}

typedef Make_Slice_Type(DG_DAG *) Node_Slice;

Node_Slice dag_sort_nodes(Arena *a, DG_DAG *dag)
{
  Node_Slice result;

  WithScratch(scratch)
  {
    Make_Dynamic_Array_Type(DG_DAG *) nodes;
    make_dynamic_array(&nodes, scratch.arena, 32);

    Make_Dynamic_Array_Type(DG_DAG *) nodes_to_traverse;
    make_dynamic_array(&nodes_to_traverse, scratch.arena, 32);

    DG_DAG *node = dag;
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

    // NOTE: remove visited
    for (u32 i = 0; i < nodes.len; ++i) {
      DG_DAG *child = SLICE_AT(nodes, i);
      child->visited = false;
    }

    dag_slice_quicksort(nodes.data, 0, nodes.len - 1);

    make_slice(a, &result, nodes.len);
    // SPEED: será que aqui funciona um memcpy?
    for (u32 i = 0; i < nodes.len; ++i) {
      SLICE_AT(result, i) = SLICE_AT(nodes, i);
    }

  }
  return result;
}


void draw_dag(DG_Canvas canvas, Node_Slice nodes)
{

  typedef struct {
    DG_DAG *node;
    u32 pos;
  } Node_Location;
  typedef Make_Dynamic_Array_Type(Node_Location) Layer;

  Make_Dynamic_Array_Type(Layer) layers;
  WithScratch(scratch)
  {

    u32 current_layer_number  = 1;
    u32 current_node_location = 0;
    Layer current_layer = { };

    SLICE_FOREACH_IDX(i, nodes) {
      DG_DAG *node = SLICE_AT(nodes, i);

      if (node->layer == 0) {
        continue;
      }

      if (node->layer == current_layer_number) {
        dynamic_array_push(&current_layer, ((Node_Location){ .node=node, .pos=current_node_location }), scratch.arena);
        current_node_location += 1;
      } else {
        dynamic_array_push(&layers, current_layer, scratch.arena);
        current_layer = (Layer){ };
        current_layer_number = node->layer;
      }

    }

    typedef struct {
      Node_Location node;
    } Draw_Node_Command;

    typedef struct {
      Node_Location node, child;
    } Draw_Connection_Command;

    Make_Dynamic_Array_Type(Draw_Node_Command) draw_node_commands;
    make_dynamic_array(&draw_node_commands, scratch.arena, nodes.len);
    Make_Dynamic_Array_Type(Draw_Connection_Command) draw_connection_commands;

    for (usize node_idx = 0; node_idx < nodes.len; ++node_idx) {
      DG_DAG *node = SLICE_AT(nodes, node_idx);

      u32 layer_idx = node->layer - 1;

      Node_Location node_loc = {};
      SLICE_FOREACH_IDX(layer_node_idx, SLICE_AT(layers, layer_idx)) {
        Node_Location loc = SLICE_AT(SLICE_AT(layers, layer_idx), layer_node_idx);

        if (loc.node == node) {
          node_loc = loc;
          break;
        }
      }

      dynamic_array_push(&draw_node_commands, ((Draw_Node_Command) { node_loc }), scratch.arena);

      if (node->children.len > 0) {

        SLICE_FOREACH_IDX(child_idx, node->children) {
          DAG_Connection connection = SLICE_AT(node->children, child_idx);
          DG_DAG *child = connection.child;

          Node_Location child_loc = {};
          SLICE_FOREACH_IDX(layer_node_idx, SLICE_AT(layers, layer_idx)) {
            Node_Location loc = SLICE_AT(SLICE_AT(layers, layer_idx), layer_node_idx);

            if (loc.node == child) {
              child_loc = loc;
              break;
            }
          }

          dynamic_array_push(&draw_connection_commands, ((Draw_Connection_Command) { .node=node_loc, .child=child_loc }), scratch.arena);
        }

      }

    }


    // TODO: draw the connections
    DG_ASSERT(!"not implemented");

    Make_Iterator_Type(Draw_Connection_Command) cmd_iter = {};
    make_iterator(cmd_iter, draw_connection_commands);
    for (;!ITERATOR_TERMINATED(cmd_iter) ; ITERATOR_ADVANCE(&cmd_iter))
    {
      Draw_Connection_Command cmd = cmd_iter.item;
      // dg_draw_line
    }

    // TODO: draw the nodes
    SLICE_FOREACH_IDX(cmd_idx, draw_node_commands) {
      Draw_Node_Command cmd = draw_node_commands.data[cmd_idx];
      // dg_draw_circle
    }

  }
}


int main(void)
{
  {
    usize size = 10 * KILOBYTE;
    permanent_arena = arena_init_buffer(permanent_arena_buffer, ARRAY_LEN(permanent_arena_buffer));

    scratch_arena[0] = arena_init_buffer(malloc(size), size);
    scratch_arena[1] = arena_init_buffer(malloc(size), size);
  }

  WithScratch(guard) {
    Arena *scratch = guard.arena;

    ML_Model *model = init_model(scratch);

    Node_Slice dag_nodes = dag_sort_nodes(scratch, &model->nodes);

    SLICE_FOREACH_IDX(i, dag_nodes) {
      DG_DAG *node = SLICE_AT(dag_nodes, i);

      // NOTE: metadata pra depois
      model->layers = MAX(model->layers, node->layer);

      {
        if (node->layer != 1) {
          node->value = 0;
        }
      }

    }

    SLICE_FOREACH_IDX(i, dag_nodes) {

      DG_DAG *node = SLICE_AT(dag_nodes, i);

      { // applying the algorithm
        if (node->layer == 1) {
          node->value = node->value + node->bias;
        } else {
          node->value = ftanh(node->value + node->bias);
        }

        for (usize i = 0; i < node->children.len; ++i) {
          DAG_Connection *connection = &SLICE_AT(node->children, i);
          connection->child->value += node->value * connection->weight;
        }
      }

      { // debug printing
        DG_LOG("%f", node->value);
        DG_LOG("%d", node->layer);
        DG_LOG("%d", 420);
      }

    }

    u32 canvas_width = 256;
    u32 canvas_height = 256;
    void *canvas_memory = arena_alloc(&permanent_arena, sizeof(u32) * canvas_width * canvas_height);
    DG_Canvas canvas = {.pixels = canvas_memory, .width = canvas_width, .height = canvas_height };
    dg_fill_canvas(canvas, u32_to_color(0xFF000000));
    dg_draw_circle(canvas, 20, 60, 20, (DG_Color){ .2, .4, .6, 1 });
    dg_draw_circle(canvas, 10, 20, 20, (DG_Color){ .2, .4, .6, 1 });

    console_log_canvas(canvas.width, canvas.height, canvas.pixels);

  }

  return 69;
}

int fds(void)
{
  return 420;
}

