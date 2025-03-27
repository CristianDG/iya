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
extern void console_log_number(u64 number);

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

// TODO: include platform independent file

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

#include "iya.c"

