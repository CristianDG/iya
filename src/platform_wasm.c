/*
 * TODO:
 * - drawing functions
 *   - NOTE: olhar olive.c
 *   - draw circle
 *   - draw square
 *   - draw line
 *
 * */

#define CDG_MATH_H
#include "cdg_base.c"

extern void console_error(char *str);
extern void console_log(char *str);

#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>

void *memset(void *ptr, u8 val, usize size) {

  u8 *cpy = ptr;
  for (usize i = 0; i < size; ++i) {
    *cpy++ = val;
  }

  return ptr;
}

void *memcpy(void *dest, const void *src, usize size) {
  void *ptr = dest;

  u8 *dest_cpy = dest;
  const u8 *src_cpy = src;

  for (usize i = 0; i < size; ++i) {
    *dest_cpy++ = *src_cpy++;
  }

  return ptr;
}

#define DG_MEMSET(ptr, val, size) memset(ptr, val, size)
#define DG_MEMCPY(dst, src, size) memcpy(dst, src, size)

#define DG_LOG_IMPL(fn, buf_identifier, args...) DG_STATEMENT({ \
  char buf_identifier[KILOBYTE] = {}; \
  stbsp_sprintf(buf_identifier, args); \
  fn(buf_identifier); \
})

#define DG_LOG_ERROR(args...) DG_LOG_IMPL(console_error, GLUE(buf, identifier), args)
#define DG_LOG(args...) DG_LOG_IMPL(console_log, GLUE(buf, identifier), args)

#include "cdg_base.c"

extern void console_log_canvas(u32 width, u32 height, void *pixels);

extern f64 tanh(f64 val);
static inline f32 tanhf(f32 val) { return (f32)tanh((f64) val); }

extern f64 cos(f64 val);
static inline f32 cosf(f32 val) { return (f32)cos((f64) val); }

extern f64 sin(f64 val);
static inline f32 sinf(f32 val) { return (f32)sin((f64) val); }

// implementation from https://surma.dev/things/c-to-webassembly/
extern u8 __heap_end;
extern u8 __heap_base;
u8 *max_memory = &__heap_end;
u8 *bump_pointer = &__heap_base;

void* malloc(usize n) {
  u8 *r = bump_pointer;
  bump_pointer += n;
  DG_ASSERT(bump_pointer <= max_memory);
  return r;
}

void free(void* p) {
  // lol
}

extern void canvas_set_dimensions(u32 width, u32 height);
extern void canvas_set_color(u8 r, u8 g, u8 b, u8 a);
extern void canvas_draw_circle(i32 cx, i32 cy, i32 r);
extern void canvas_draw_rect(i32 x, i32 y, i32 width, i32 height);
extern void canvas_draw_line(i32 x1, i32 y1, i32 x2, i32 y2, u32 thickness);
extern void canvas_draw();
extern void get_mouse_position(u32 *x, u32 *y);

void draw() {
  canvas_draw();
}

#include "iya.c"

