
#ifndef JAM_COMMON
#define JAM_COMMON

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#define TAU 6.283185f

#define radToDeg(rad) ((rad * 360) / TAU)
#define degToRad(deg) ((deg * TAU) / 180)

#define MAX_READ_EVENTS 64
#define MAX_PARTICLE 10

#define SToMS(value) (value * 1000)
#define MSToNS(value) (value * 1000000)

#define Kilobytes(number) ((number) * 1024ull)
#define Megabytes(number) (Kilobytes(number) * 1024ull)
#define Gigabytes(number) (Megabytes(number) * 1024ull)

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

typedef Vector2 v2;
typedef Vector3 v3;
typedef Vector4 v4;

typedef struct memoryArena {
 uint32_t size;
 uint32_t used;

 void *memory;
} memoryArena;

static inline void *pushSize(memoryArena *arena, size_t size) {
  uint8_t *result = 0;
  if (arena->used + size < arena->size && size > 0) {
    result = (uint8_t *)arena->memory + arena->used;

    arena->used += size;
  }
  return (void *)(result);
}


#include "scene.h"

#endif
