#include "common.h"

struct SceneState {
  memoryArena arena;
  uint32_t fooBar;
};

static void Entry(common_context *ctx, void **data);
static void Exit(common_context *ctx, void *data);
static void Render(common_context *ctx, void *data);
static void Update(common_context *ctx, void *data);

struct Scene s_TerrainOnly = {
  .EntryFunction = Entry, 
  .ExitFunction = Exit, 
  .RenderFunction = Render, 
  .UpdateFunction = Update
};

static void Entry(common_context *ctx, void **data) {
  if (*data) {
    printf("Expecting an pointer to a uninitialized void pointer\n");
    return;
  }

  *data = MemAlloc(sizeof(struct SceneState));
  struct SceneState *state = (struct SceneState *)*data;

  state->arena.size = Megabytes(2);
  state->arena.used = 0;
  state->arena.memory = MemAlloc(state->arena.size);

  printf("Hello\n");
}

static void Exit(common_context *ctx, void *data) {
  if (!data) {
    return;
  }

  struct SceneState *state = (struct SceneState *)data;
  MemFree(state->arena.memory);
  MemFree(data);

}

static void Render(common_context *ctx, void *data) {

}

static void Update(common_context *ctx, void *data) {

}
