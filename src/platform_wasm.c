void *memset_stub(void *src, char val, long long size);

#define DG_MEMSET(ptr, val, size) 0
#define DG_MEMCPY(dst, src, size) 0
#define DG_LOG_ERROR(args...)
#define DG_LOG(args...)

#define CDG_MATH_H
#define DG_ALLOC_IMPLEMENTATION
#include "cdg_base.c"

int main(void) {
  return 0;
}
