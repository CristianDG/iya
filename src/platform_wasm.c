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

#define DG_MEMSET(ptr, val, size) memset(ptr, val, size)
#define DG_MEMCPY(dst, src, size) 0 /*stub*/

#define DG_LOG_IMPL(fn, buf_identifier, args...) DG_STATEMENT({ \
  char buf_identifier[KILOBYTE] = {}; \
  stbsp_sprintf(buf_identifier, args); \
  fn(buf_identifier); \
})

#define DG_LOG_ERROR(args...) DG_LOG_IMPL(console_error, GLUE(buf, identifier), args)
#define DG_LOG(args...) DG_LOG_IMPL(console_log, GLUE(buf, identifier), args)

#define DG_CONTAINER_IMPLEMENTATION
#define DG_ALLOC_IMPLEMENTATION
#define DG_MATRIX_IMPLEMENTATION
#include "cdg_base.c"

// TODO: include platform independent file

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


  return 69;
}

extern int fds(void) {
  return 420;
}
