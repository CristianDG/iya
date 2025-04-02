// TODO: DG_ALL_IMPLEMENTATION

// types.h {{{
#ifndef CDG_TYPES_H
#define CDG_TYPES_H


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* TODO:
 * - context cracking
 * - debugger trap
 */

#define DG_STATEMENT(x) do { x } while (0)

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
#define ABS(x) ((x) > 0 ? (x) : -(x))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define CLAMP_TOP MIN
#define CLAMP_BOTTOM MAX
#define CLAMP(val, min, max) (val < min ? min : (val > max ? max : val))

#define STR(x) #x
#define GLUE(a,b) a##b

#define ELEVENTH_ARGUMENT(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, ...) a11
#define DG_NARGS(...) ELEVENTH_ARGUMENT(dummy, ## __VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)


#if !defined(DG_STATIC_ASSERT) // {{{

#define DG_STATIC_ASSERT(args...) DG_STATIC_ASSERT_IMPL(DG_NARGS(args), args)
#define DG_STATIC_ASSERT_IMPL(n, args...) GLUE(DG_STATIC_ASSERT_, n)(args)
#define DG_STATIC_ASSERT_1(expr) _Static_assert(expr, "")
#define DG_STATIC_ASSERT_2(expr, msg) _Static_assert(expr, msg)

#endif // }}} DG_STATIC_ASSERT

#define is_power_of_two(x) ((x != 0) && ((x & (x - 1)) == 0))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uintptr;
typedef intptr_t intptr;

typedef size_t usize;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;

typedef u32 b32;

#define KILOBYTE 1024
#define MEGABYTE 1048576L

#define DG_REINTERPRET_CAST(type, val) \
  (*((type *)&(val)))

#define DG_OFFSET_OF(type, field) \
  ((uintptr)&(((type *) 0)->field))

#define DG_DYNAMIC_ACCESS(type, offset) \
  (((void *)(type))+offset)

#define DG_SWAP(type, a, b) DG_STATEMENT({ \
  type _tmp = *(a); \
  *(a) = *(b); \
  *(b) = _tmp; \
})


#ifndef DG_CRASH // {{{
#if defined(DG_PLATFORM_WASM)
#define DG_CRASH() __builtin_trap()
#else
#define DG_CRASH() (*((volatile int *)0) = 69)
#endif
#endif // }}} DG_CRASH

#if !defined(DG_ASSERT_EXPR) // {{{
// NOTE: esse assert funciona como expressão: bool assert(bool)
#define DG_ASSERT_EXPR(exp) ( \
  (exp) \
    ? (true) \
    : (DG_LOG_ERROR("%s,%d: assertion '%s' failed\n", __FILE__, __LINE__, STR(exp)), DG_CRASH(), false) \
)
#endif // DG_ASSERT_EXPR }}}

#if !defined(DG_ASSERT) // {{{
#define DG_ASSERT(exp) \
DG_STATEMENT({ \
  if ((exp) == false) { \
    DG_LOG_ERROR("%s,%d: assertion '%s' failed\n", __FILE__, __LINE__, STR(exp)); \
    DG_CRASH(); \
  } \
})
#endif // DG_ASSERT }}}

#endif // }}}

// arena allocator (+ allocator struct ...?) {{{
#ifndef CDG_ALLOC_C // alloc.c {{{
#define CDG_ALLOC_C

/* TODO:
 *  - GetScratch macro
 *  - ReleaseScratch macro
 *  - WithScratch macro usando for
 * */

#include <stddef.h>
// #include <string.h> // for memset

#define DEFAULT_ALIGNMENT 16

// TODO: WIP
typedef struct {
} Growing_Arena;

typedef struct {
  u8 *data;
  // TODO: add temp_count
  u32 size;
  u32 cursor;
} Arena;

// TODO: adicionar uma abstração que faça uso
typedef struct {
  void *(*alloc)(u64);
  void (*free)(void *);
} DG_Allocator;

typedef struct {
  Arena *arena;
  u32 cursor;
} Temp_Arena_Memory;


Arena arena_init_buffer(u8 *data, size_t size);
uintptr_t align_forward(uintptr_t ptr, size_t alignment);

void *_arena_alloc(Arena *arena, size_t size, size_t alignment);
void *_tracking_arena_alloc(Arena *arena, size_t size, size_t alignment, char *file, i32 line);

Temp_Arena_Memory temp_arena_memory_begin(Arena *a);
void temp_arena_memory_end(Temp_Arena_Memory *tmp_mem);

void arena_clear(Arena *arena);

#if defined(DG_ARENA_DEBUG) // {{{
#define arena_alloc(arena, size) _tracking_arena_alloc(arena, size, DEFAULT_ALIGNMENT, __FILE__, __LINE__)
#define arena_alloc_pass_loc(arena, size, file, line) _tracking_arena_alloc(arena, size, DEFAULT_ALIGNMENT, file, line)
// }}}
#else
// {{{
#define arena_alloc(arena, size) _arena_alloc(arena, size, DEFAULT_ALIGNMENT)
#define arena_alloc_pass_loc(arena, size, file, line) _arena_alloc(arena, size, DEFAULT_ALIGNMENT)
#endif // }}} DG_ARENA_DEBUG

#define TempGuard(arena) \
  for (Temp_Arena_Memory __t_a_m = temp_arena_memory_begin(arena) \
  ; __t_a_m.arena \
  ; temp_arena_memory_end(&__t_a_m))

#endif // CDG_ALLOC_C }}}
#if defined(DG_ALLOC_IMPLEMENTATION) // {{{

#ifndef DG_MEMSET
#include <string.h>
#define DG_MEMSET memset
#endif // DG_MEMSET

#ifndef DG_MEMCPY
#include <string.h>
#define DG_MEMCPY memcpy
#endif // DG_MEMCPY

#ifndef DG_LOG_ERROR
#include <stdio.h>
#define DG_LOG_ERROR(args...) fprintf(stderr, args)
#endif // DG_LOG_ERROR

#ifndef DG_LOG
#include <stdio.h>
#define DG_LOG(args...) fprintf(stdout, args)
#endif // DG_LOG

// TODO: make scratch arena

Arena arena_init_buffer(u8 *data, size_t size)
{
  return (Arena){
    .data = data,
    .size = size,
  };
}

// implementation from https://dylanfalconer.com/articles/the-arena-custom-memory-allocators
uintptr_t align_forward(uintptr_t ptr, size_t alignment)
{
  uintptr_t p, a, modulo;
  if (!is_power_of_two(alignment)) {
    return 0;
  }

  p = ptr;
  a = (uintptr_t)alignment;
  modulo = p & (a - 1);

  if (modulo) {
    p += a - modulo;
  }

  return p;
}

void *_arena_alloc(Arena *arena, size_t size, size_t alignment)
{
  uintptr_t curr_ptr = (uintptr_t)arena->data + (uintptr_t)arena->cursor;
  uintptr_t offset = align_forward(curr_ptr, alignment);
  offset -= (uintptr_t)arena->data;

  // NOTE: olhar se é melhor um if ou um assert aqui
  DG_ASSERT(offset + size < arena->size);

  void *ptr = (void *)arena->data + offset;
  arena->cursor = offset + size;
  return DG_MEMSET(ptr, 0, size);
}

// TODO: olhar https://youtu.be/443UNeGrFoM?si=DBJXmKB_z8W8Yrrf&t=3074
void *_tracking_arena_alloc(Arena *arena, size_t size, size_t alignment, char *file, i32 line)
{
  // TODO: registrar onde foram todas as alocações
  void *ptr = _arena_alloc(arena, size, alignment);
  if (ptr == 0) {
    DG_LOG_ERROR("%s:%d Could not allocate %zu bytes\n", file, line, size);
  }
  return ptr;
}

Temp_Arena_Memory temp_arena_memory_begin(Arena *a)
{
  return (Temp_Arena_Memory) {
    .arena = a,
    .cursor = a->cursor,
  };
}

void temp_arena_memory_end(Temp_Arena_Memory *tmp_mem)
{
  tmp_mem->arena->cursor = tmp_mem->cursor;
  tmp_mem->arena = 0;
}

// TODO:
void arena_clear(Arena *arena)
{
  arena->cursor = 0;
}

#endif // DG_ALLOC_IMPLEMENTATION }}}
// }}}

// dynamic array and other containers...? {{{
#ifndef CDG_CONTAINER_C  // {{{
#define CDG_CONTAINER_C
// TODO: include arena.c

// NOTE: implementation from https://nullprogram.com/blog/2023/10/05/

#define Make_Dynamic_Array_Type(type) \
struct { \
  type *data; \
  i32 len; \
  i32 cap; \
}

typedef Make_Dynamic_Array_Type(void) _Any_Dynamic_Array;

void _make_dynamic_array(_Any_Dynamic_Array *arr, u32 capacity, Arena *a, u32 item_size);
#define make_dynamic_array(arr, arena, capacity) _make_dynamic_array((_Any_Dynamic_Array *) arr, capacity, arena, sizeof((arr)->data))

#endif // }}} CDG_CONTAINER_C
#if defined(DG_CONTAINER_IMPLEMENTATION) // {{{

void _make_dynamic_array(_Any_Dynamic_Array *arr, u32 capacity, Arena *a, u32 item_size) {
  _Any_Dynamic_Array replica = {0};
  replica.cap = capacity;
  replica.data = arena_alloc(a, 2 * item_size * replica.cap);
  DG_MEMCPY(arr, &replica, sizeof(replica));
}

void dynamic_array_grow(_Any_Dynamic_Array *arr, Arena *a, u32 item_size) {
  _Any_Dynamic_Array replica = {0};
  DG_MEMCPY(&replica, arr, sizeof(replica));

  if (!replica.data) {
    // TODO: default capacity
    replica.cap = 1;
    replica.data = arena_alloc(a, 2 * item_size * replica.cap);
  } else if ((replica.data + replica.cap * item_size) == (a->data + a->cursor)) {
    // NOTE: se a última alocação da arena foi esse array,
    // então da para extender allocando um `cap` a mais
    arena_alloc(a, item_size * replica.cap);
  } else {
    void *data = arena_alloc(a, 2 * item_size * replica.cap);
    DG_MEMCPY(data, replica.data, item_size*replica.len);
    replica.data = data;
  }

  replica.cap *= 2;
  DG_MEMCPY(arr, &replica, sizeof(replica));
}

#define dynamic_array_push(arr, item, arena) DG_STATEMENT({ \
  if ((arr)->len >= (arr)->cap) { \
    dynamic_array_grow((_Any_Dynamic_Array*)(arr), (arena), sizeof(*(arr)->data)); /* NOLINT */ \
  } \
  (arr)->data[(arr)->len++] = (item); \
})


void _dynamic_array_pop(_Any_Dynamic_Array *arr, void *dst, u32 item_size) {
  memcpy(dst, arr->data, item_size);
  memcpy(arr->data, arr->data + ((arr->len * item_size) - (1 * item_size)), item_size);
  arr->len -= 1;
}
#define dynamic_array_pop(arr, item) _dynamic_array_pop((_Any_Dynamic_Array *) arr, (void *) item, sizeof(item))

void _dynamic_array_clear(_Any_Dynamic_Array *arr) {
  arr->len = 0;
}

#define dynamic_array_clear(arr) _dynamic_array_clear((_Any_Dynamic_Array *) (arr))

#define Make_Slice_Type(type) \
struct { \
  type *data; \
  i32 len; \
}

typedef Make_Slice_Type(void) _Any_Slice;

// NOTE: somente necessário se for usar DG_REINTERPRET_CAST
DG_STATIC_ASSERT(DG_OFFSET_OF(_Any_Slice, data) == DG_OFFSET_OF(_Any_Dynamic_Array, data));
DG_STATIC_ASSERT(DG_OFFSET_OF(_Any_Slice, len ) == DG_OFFSET_OF(_Any_Dynamic_Array, len ));

#define make_slice(arena, slice, len) dg_make_slice(arena, (_Any_Slice *)slice, len, sizeof(*(slice)->data))

void dg_make_slice(Arena *a, _Any_Slice *slice, u64 len, u64 item_size){
  _Any_Slice res = {0};

  void *data = arena_alloc(a, len * item_size);

  if (data) {
    res.len = len;
    res.data = data;
  }

  *slice = res;
}

#define SLICE_FOREACH_IDX(idx, slice) for (usize idx = 0; idx < (slice).len; ++idx )
#define SLICE_AT(slice, idx) (slice).data[idx]
#define SLICE_AT_WRAP(slice, idx) (slice).data[idx < 0 ? slice.len + idx : idx]

/*
FIXME:
fazer a implementação funcionar diretamente dentro de um for
Eu acretido que da pra fazer isso se passar o slice em `ITERATOR_ADVANCE`

exemplo:

```c
for (
  Make_Iterator_Type(Draw_Connection_Command) cmd_iter = {};
  !ITERATOR_TERMINATED(cmd_iter);
  ITERATOR_ADVANCE(&cmd_iter, draw_connection_commands);
) {
  Draw_Connection_Command cmd = cmd_iter.item;
  // dg_draw_line
}
```

*/
// iterator implementation {{{
#define Make_Iterator_Type(type) \
struct { \
  type *data; \
  i32 len; \
  u32 next_idx; \
  type item; \
}
typedef Make_Iterator_Type(u8) _Any_Iterator;

#define ITERATOR_TERMINATED(iterator) ((iterator).next_idx > (iterator).len)

static inline void _advance(_Any_Iterator *iterator, u8 item_size) {
  DG_MEMCPY(&iterator->item, (iterator->data + (iterator->next_idx * item_size)), item_size);
  iterator->next_idx++;
}

#define ITERATOR_ADVANCE(iterator) _advance((_Any_Iterator *) iterator, sizeof((iterator)->item))

#define make_iterator(name, slice) DG_STATEMENT({ \
  name.data = (slice).data; \
  name.len  = (slice).len; \
  ITERATOR_ADVANCE(&name); \
})
// }}}


struct dg_dag;

typedef struct {
  f32 weight;
  struct dg_dag *child;
} DAG_Connection;

typedef struct dg_dag { // TODO: mudar para a parte de definições
  Make_Dynamic_Array_Type(DAG_Connection) children;
  u32 layer; // NOTE: olhar se ajuda na organização topologica
  f32 value;
  f32 bias;
  b32 visited;
} DG_DAG;

#define DAG_CHILD_AT(node, idx) SLICE_AT((node).children, idx).child

void dag_connect_child(Arena *a, DG_DAG *parent, DG_DAG *child, f32 connection_weight) {
  child->layer = MAX(parent->layer + 1, child->layer);
  DAG_Connection connection = { .child = child, .weight = connection_weight };
  dynamic_array_push(&parent->children, connection, a);
}

DG_DAG *dag_add_child(Arena *a, DG_DAG *parent, f32 value, f32 bias, f32 connection_weight) {
  DG_DAG *child = arena_alloc(a, sizeof(*child));
  child->value = value;

  if (parent) {
    dag_connect_child(a, parent, child, connection_weight);
  }

  return child;
}

#endif // }}} defined(DG_CONTAINER_IMPLEMENTATION)
// }}}

// algorithms {{{
#ifndef DG_ALGORITHM_H // {{{
#define DG_ALGORITHM_H

// TODO: quicksort

#endif // }}} DG_ALGORITHM_H
#if defined(DG_ALGORITHM_IMPLEMENTATION) // {{{


#endif // }}} DG_ALGORITHM_H
// }}}

// Matrix types and operations {{{
#ifndef CDG_MATRIX_H // {{{
#define CDG_MATRIX_H

typedef struct {
  f32 *data;
  i32 rows;
  i32 cols;
} DG_Matrix_View;

#define MAT_AT(mat, row, col) (mat).data[row*(mat).cols + col]

#endif // }}} CDG_MATRIX_H
#if defined(DG_MATRIX_IMPLEMENTATION) // {{{

DG_Matrix_View matrix_alloc(Arena *a, u32 rows, u32 cols){
  DG_Matrix_View m = {
    .rows = rows,
    .cols = cols,
  };
  void* data = arena_alloc(a, sizeof(*m.data) * rows * cols);
  m.data = data;
  return m;
}

void matrix_copy(DG_Matrix_View dst, DG_Matrix_View src){
  DG_ASSERT(src.rows == dst.rows);
  DG_ASSERT(src.cols == dst.cols);

  DG_MEMCPY(dst.data, src.data, sizeof(*src.data) * src.rows * src.cols);
}

// NOTE: não sei como me sinto passando um valor por cópia mesmo sabendo que ele vai ser modificado
void matrix_fill(DG_Matrix_View m, f32 val) {
  for(u32 r = 0; r < m.rows; ++r) {
    for(u32 c = 0; c < m.cols; ++c) {
      MAT_AT(m, r, c) = val;
    }
  }
}

void matrix_sum_in_place(DG_Matrix_View dst, DG_Matrix_View a, DG_Matrix_View b) {
  DG_ASSERT(a.rows == b.rows);
  DG_ASSERT(a.cols == b.cols);

  for(u32 r = 0; r < dst.rows; ++r) {
    for(u32 c = 0; c < dst.cols; ++c) {
        MAT_AT(dst, r, c) = MAT_AT(a, r, c) + MAT_AT(b, r, c);
    }
  }
}

void matrix_dot_in_place(DG_Matrix_View dst, DG_Matrix_View a, DG_Matrix_View b) {
  DG_ASSERT(a.cols == b.rows);
  DG_ASSERT(dst.cols == b.cols);
  DG_ASSERT(dst.rows == a.rows);

  for(u32 r = 0; r < dst.rows; ++r) {
    for(u32 c = 0; c < dst.cols; ++c) {
      f32 val = 0;
      for (u32 k = 0; k < a.cols; ++k) {
         val += MAT_AT(a, r, k) * MAT_AT(b, k, c);
      }
      MAT_AT(dst, r, c) = val;
    }
  }
}

DG_Matrix_View matrix_dot(Arena *arena, DG_Matrix_View a, DG_Matrix_View b) {
  DG_Matrix_View res = matrix_alloc(arena, a.rows, b.cols);
  matrix_dot_in_place(res, a, b);
  return res;
}

void dg_matrix_print(DG_Matrix_View m, char *name) {
  DG_LOG("%s = [ \n", name);
  for(u32 r = 0; r < m.rows; ++r) {
    DG_LOG("    ");
    for(u32 c = 0; c < m.cols; ++c) {
      DG_LOG(" %f", MAT_AT(m, r, c));
    }
    DG_LOG("\n");
  }
  DG_LOG("]\n");
}
#define matrix_print(x) dg_matrix_print(x, STR(x))


#endif // }}} DG_MATRIX_IMPLEMENTATION
// }}}

// TODO: math {{{
#ifndef CDG_MATH_H
#define CDG_MATH_H

#include <math.h>
#include <stdlib.h>

f32 sigmoidf(f32 x) {
  return 1.f / (1.f + expf(-x));
}

f32 randf(){
  return (f32) rand() / (f32) RAND_MAX;
}

#endif // CDG_MATH_H
// }}}

// software renderer {{{
#ifndef CDG_SOFTWARE_RENDERER_H // {{{
#define CDG_SOFTWARE_RENDERER_H

typedef struct {
  f32 r, g, b, a;
} DG_Color;

typedef struct {
  u32 *pixels;
  usize width;
  usize height;
} DG_Canvas;

// trocar para i32...????
typedef struct {
  u32 x1, y1;
  u32 x2, y2;
} DG_Safe_Rect;

#define DG_GET_PIXEL(canvas, x, y) (canvas).pixels[(y) * (canvas).width + (x)]

static inline u32 color_to_u32(DG_Color color){
  u32 a = (u8)(color.a * 255) << (8 * 3);
  u32 r = (u8)(color.r * 255) << (8 * 2);
  u32 g = (u8)(color.g * 255) << (8 * 1);
  u32 b = (u8)(color.b * 255) << (8 * 0);

  return (a | r | g | b);
}

static inline DG_Color u32_to_color(u32 val) {
  DG_Color result = { 0 };

  u8 a = val >> (8 * 3);
  u8 r = val >> (8 * 2);
  u8 g = val >> (8 * 1);
  u8 b = val >> (8 * 0);

  result.r = r / 255.f;
  result.g = g / 255.f;
  result.b = b / 255.f;
  result.a = a / 255.f;

  return result;
}

#endif // }}} CDG_SOFTWARE_RENDERER_H
#if defined(DG_SOFTWARE_RENDERER_IMPLEMENTATION) // {{{

void dg_fill_canvas(DG_Canvas canvas, DG_Color color) {

  for (u32 y = 0; y <= canvas.height; ++y) {
    for (u32 x = 0; x <= canvas.width; ++x) {
      DG_GET_PIXEL(canvas, x, y) = color_to_u32(color);
    }
  }

}

DG_Safe_Rect
dg_normalize_rect(
  i32 x, i32 y,
  i32 width, i32 height,
  u32 canvas_width, u32 canvas_height
) {
  DG_Safe_Rect result = { 0 };

  result.x1 = CLAMP(x, 0, canvas_width-1);
  result.x2 = CLAMP(x + width, 0, canvas_width-1);

  result.y1 = CLAMP(y, 0, canvas_height-1);
  result.y2 = CLAMP(y + height, 0, canvas_height-1);

  return result;
}

void dg_draw_circle(DG_Canvas canvas, i32 cx, i32 cy, i32 r, DG_Color color) {
  DG_ASSERT(r > 0);

  DG_Safe_Rect rect = dg_normalize_rect(
    cx - r, cy - r, r * 2, r * 2,
    canvas.width, canvas.height
  );

  for (u32 y = rect.y1; y <= rect.y2; ++y) {
    for (u32 x = rect.x1; x <= rect.x2; ++x) {

      u32 square_distance_x = (x - cx) * (x - cx);
      u32 square_distance_y = (y - cy) * (y - cy);
      if ((square_distance_x + square_distance_y) <= (r * r)) {
        DG_GET_PIXEL(canvas, x, y) = color_to_u32(color);
      }

    }
  }

}

#endif // }}} DG_SOFTWARE_RENDERER_IMPLEMENTATION
#if defined(DG_CANVAS_RENDERER_IMPLEMENTATION) // {{{

DG_Canvas dg_create_canvas(u32 width, u32 height) {
  canvas_set_dimensions(width, height);
  return (DG_Canvas){ .width = width, .height = height };
}

void dg_fill_canvas(DG_Canvas canvas, DG_Color color){
  canvas_draw_rect(0, 0, canvas.width, canvas.height, color_to_u32(color));
}

void dg_draw_circle(DG_Canvas canvas, i32 cx, i32 cy, i32 r, DG_Color color){
  canvas_draw_circle(cx, cy, r, color_to_u32(color));
}

void dg_draw_line(DG_Canvas canvas, i32 cx, i32 cy, i32 r, DG_Color color){

}

#endif // }}} DG_CANVAS_RENDERER_IMPLEMENTATION

// }}}
