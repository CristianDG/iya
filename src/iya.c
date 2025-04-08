
#define DG_CONTAINER_IMPLEMENTATION
#define DG_ALLOC_IMPLEMENTATION
#define DG_MATRIX_IMPLEMENTATION
// #define DG_SOFTWARE_RENDERER_IMPLEMENTATION
#define DG_CANVAS_RENDERER_IMPLEMENTATION
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

  typedef struct {
    Node_Location loc;
  } Draw_Node_Command;

  typedef struct {
    Node_Location node_loc, child_loc;
  } Draw_Connection_Command;

  typedef struct {
    Make_Dynamic_Array_Type(Draw_Node_Command) node_commands;
    Make_Dynamic_Array_Type(Draw_Connection_Command) connection_commands;
    // Make_Dynamic_Array_Type(u32) layer_nodes;
    Make_Dynamic_Array_Type(Layer) layers;
  } Draw_Dag_Command_Buffer;

  Draw_Dag_Command_Buffer cmd_buffer = {};

  WithScratch(scratch)
  {

    u32 current_layer_number  = 0;
    u32 current_node_location = 0;

    SLICE_FOREACH_IDX(i, nodes) {
      DG_DAG *node = SLICE_AT(nodes, i);

      if (node->layer == 0) {
        continue;
      }

      if (node->layer != current_layer_number) {
        dynamic_array_push(&cmd_buffer.layers, ((Layer){ }), scratch.arena);
        current_node_location = 0;
        current_layer_number = node->layer;
      }

      if (node->layer == current_layer_number) {
        dynamic_array_push(&SLICE_AT_WRAP(cmd_buffer.layers, -1), ((Node_Location){ .node=node, .pos=current_node_location }), scratch.arena);
        current_node_location += 1;
      }


    }

    make_dynamic_array(&cmd_buffer.node_commands, scratch.arena, nodes.len);

    for (usize node_idx = 0; node_idx < nodes.len; ++node_idx) {
      DG_DAG *node = SLICE_AT(nodes, node_idx);

      u32 layer_idx = node->layer - 1;

      Node_Location node_loc = {};
      SLICE_FOREACH_IDX(layer_node_idx, SLICE_AT(cmd_buffer.layers, layer_idx)) {
        Node_Location loc = SLICE_AT(SLICE_AT(cmd_buffer.layers, layer_idx), layer_node_idx);

        if (loc.node == node) {
          node_loc = loc;
          break;
        }
      }
      DG_ASSERT(node_loc.node != 0);

      dynamic_array_push(&cmd_buffer.node_commands, ((Draw_Node_Command) { node_loc }), scratch.arena);

      if (node->children.len > 0) {

        SLICE_FOREACH_IDX(child_idx, node->children) {
          DAG_Connection connection = SLICE_AT(node->children, child_idx);
          DG_DAG *child = connection.child;
          u32 child_layer_idx = child->layer - 1;

          Node_Location child_loc = {};
          SLICE_FOREACH_IDX(layer_node_idx, SLICE_AT(cmd_buffer.layers, child_layer_idx)) {
            Node_Location loc = SLICE_AT(SLICE_AT(cmd_buffer.layers, child_layer_idx), layer_node_idx);

            if (loc.node == child) {
              child_loc = loc;
              break;
            }
          }
          DG_ASSERT(child_loc.node != 0);

          dynamic_array_push(&cmd_buffer.connection_commands, ((Draw_Connection_Command) { .node_loc=node_loc, .child_loc=child_loc }), scratch.arena);
        }

      }

    }

    {

      u32 padding_y = 10;
      u32 padding_x = 10;

      u32 node_radius = 10;
      u32 node_padding_y = padding_y + node_radius;
      u32 node_padding_x = padding_x + node_radius;

      u32 horizontal_space_available = canvas.width - (node_padding_x * 2);
      u32 vertical_space_available = canvas.height  - (node_padding_y * 2);

      ITERATE_SLICE(Draw_Connection_Command, cmd_iter, cmd_buffer.connection_commands)
      {
        u32 x1 = 0;
        u32 y1 = 0;
        {
          u32 pos_x = 0;
          u32 pos_y = 0;

          u32 layer_idx = cmd_iter.item.node_loc.node->layer-1;
          u32 layer_nodes_number = SLICE_AT(cmd_buffer.layers, layer_idx).len;
          u32 layers_number = cmd_buffer.layers.len;
          // x pos
          if (cmd_buffer.layers.len > 1) {
            u32 horizontal_node_spacing = horizontal_space_available / (layers_number - 1);
            pos_x = (horizontal_node_spacing * layer_idx) + node_padding_x;
          } else {
            pos_x = (horizontal_space_available / 2) + node_padding_x;
          }

          // y pos
          if (layer_nodes_number > 1) {
            u32 vertical_node_spacing = vertical_space_available / (layer_nodes_number - 1);
            pos_y = (vertical_node_spacing * cmd_iter.item.node_loc.pos) + node_padding_y;
          } else {
            pos_y = (vertical_space_available / 2) + node_padding_y;
          }

          x1 = pos_x;
          y1 = pos_y;
        }

        u32 x2 = 0;
        u32 y2 = 0;
        {
          u32 pos_x = 0;
          u32 pos_y = 0;

          u32 layer_idx = cmd_iter.item.child_loc.node->layer-1;
          u32 layer_nodes_number = SLICE_AT(cmd_buffer.layers, layer_idx).len;
          u32 layers_number = cmd_buffer.layers.len;
          // x pos
          if (cmd_buffer.layers.len > 1) {
            u32 horizontal_node_spacing = horizontal_space_available / (layers_number - 1);
            pos_x = (horizontal_node_spacing * layer_idx) + node_padding_x;
          } else {
            pos_x = (horizontal_space_available / 2) + node_padding_x;
          }

          // y pos
          if (layer_nodes_number > 1) {
            u32 vertical_node_spacing = vertical_space_available / (layer_nodes_number - 1);
            pos_y = (vertical_node_spacing * cmd_iter.item.child_loc.pos) + node_padding_y;
          } else {
            pos_y = (vertical_space_available / 2) + node_padding_y;
          }

          x2 = pos_x;
          y2 = pos_y;
        }

        dg_draw_line(canvas, x1, y1, x2, y2, 2, (DG_Color){ 1, 1, 1, 1 });
      }

      // draw the nodes
      for (usize cmd_idx = 0; cmd_idx < cmd_buffer.node_commands.len; ++cmd_idx){
        Draw_Node_Command cmd = cmd_buffer.node_commands.data[cmd_idx];

        u32 layer_idx = cmd.loc.node->layer-1;
        u32 layer_nodes_number = SLICE_AT(cmd_buffer.layers, layer_idx).len;
        u32 layers_number = cmd_buffer.layers.len;

        u32 node_x = 0;
        u32 node_y = 0;


        // x pos
        if (cmd_buffer.layers.len > 1) {
          u32 horizontal_node_spacing = horizontal_space_available / (layers_number - 1);
          node_x = (horizontal_node_spacing * layer_idx) + node_padding_y;
        } else {
          node_x = (horizontal_space_available / 2) + node_padding_y;
        }

        // y pos
        if (layer_nodes_number > 1) {
          u32 vertical_node_spacing = vertical_space_available / (layer_nodes_number - 1);
          node_y = (vertical_node_spacing * cmd.loc.pos) + node_padding_y;
        } else {
          node_y = (vertical_space_available / 2) + node_padding_y;
        }

        dg_draw_circle(canvas, node_x, node_y, node_radius, (DG_Color){1, 1, 1, 1});
      }
    }
  }
}

typedef struct {
  f32 angle;
  u32 length;
  i32 origin_x;
  i32 origin_y;
  f32 velocity;
  f32 acceleration;
} Pendulum;

typedef struct {
  DG_Canvas canvas;
  f32 dt;
  Pendulum pendulum;
  // TODO: keyboard/mouse state
} App_State;

global_variable App_State global_app_state = {};

int start(void)
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
          node->value = tanhf(node->value + node->bias);
        }

        for (usize i = 0; i < node->children.len; ++i) {
          DAG_Connection *connection = &SLICE_AT(node->children, i);
          connection->child->value += node->value * connection->weight;
        }
      }

      { // debug printing
        // DG_LOG("layer: %d value: %f", node->layer, node->value);
      }

    }

    u32 canvas_width = 640;
    u32 canvas_height = 480;
    // void *canvas_memory = arena_alloc(&permanent_arena, sizeof(u32) * canvas_width * canvas_height);
    // DG_Canvas canvas = {.pixels = canvas_memory, .width = canvas_width, .height = canvas_height };
    global_app_state.canvas = dg_create_canvas(canvas_width, canvas_height);
    dg_fill_canvas(global_app_state.canvas, u32_to_color(0xFF000000));
    // dg_draw_circle(canvas, 20, 60, 20, (DG_Color){ .2, .4, .6, 1 });
    // dg_draw_circle(canvas, 10, 20, 20, (DG_Color){ .2, .4, .6, 1 });
    draw_dag(global_app_state.canvas, dag_nodes);
    console_log_canvas(global_app_state.canvas.width, global_app_state.canvas.height, global_app_state.canvas.pixels);

  }

  global_app_state.pendulum = (Pendulum){
    .length = 150,
    .angle = 179,
    .origin_x = global_app_state.canvas.width / 2,
    .origin_y = global_app_state.canvas.height / 2,
  };

  return 69;
}

static DG_Color WHITE = {.r = 1, .g = 1, .b = 1, .a = 1};

void update_pendulum(Pendulum *p, f32 dt) {

  const f32 gravity = 9.8;
  f32 acceleration = -1 * gravity * sin(p->angle);

  p->velocity += acceleration * dt;
  // air resistence
  p->velocity *= .99;
  p->angle += p->velocity;

}

void draw_pendulum(DG_Canvas canvas, Pendulum p) {

  u32 arm_start_x = p.origin_x;
  u32 arm_start_y = p.origin_y;

  // sin cos ou cos sin???
  u32 arm_end_x = arm_start_x + (i32)(sinf(p.angle) * (f32)p.length);
  u32 arm_end_y = arm_start_y + (i32)(cosf(p.angle) * (f32)p.length);

  dg_draw_line(
    canvas,
    arm_start_x, arm_start_y,
    arm_end_x, arm_end_y,
    5, WHITE);

  dg_draw_circle(canvas, arm_end_x, arm_end_y, 20, WHITE);

}

void step(f64 dt) {
  if (dt > 1) { return; }

  bool dragging = false;

  global_app_state.dt = dt;
  dg_fill_canvas(global_app_state.canvas, u32_to_color(0xFF000000));

  if (!dragging) {
    update_pendulum(&global_app_state.pendulum, global_app_state.dt);
  }

  draw_pendulum(global_app_state.canvas, global_app_state.pendulum);

  draw();
}

int fds(void)
{
  return 420;
}

