
#include "common.h"

struct SceneState {
  memoryArena arena;
};

static void Entry(common_context *ctx, void **data);
static void Exit(common_context *ctx, void *data);
static void Render(common_context *ctx, void *data);
static void Update(common_context *ctx, void *data);

struct Scene s_screenSaver = {
  .EntryFunction = Entry, 
  .ExitFunction = Exit, 
  .RenderFunction = Render, 
  .UpdateFunction = Update
};

static void Entry(common_context *ctx, void **data) {

  *data = MemAlloc(sizeof(struct SceneState));
  struct SceneState *state = (struct SceneState *)*data;

  state->arena.size = Megabytes(2);
  state->arena.used = 0;
  state->arena.memory = MemAlloc(state->arena.size);

  char *text_file = LoadFileText("assets/txts/gone_away_messages.txt");
  int32_t total_messages = 0;
  char **temp_messages = TextSplit(text_file, '\n', &total_messages);
  void **messages = pushSize(&state->arena, sizeof(void*) * total_messages - 1);

  for (uint32_t i = 0; i < total_messages - 1; i++) {
    char *cur_txt = temp_messages[i];
    uint32_t message_length = TextLength(cur_txt);

    if (message_length == 0) {
     continue;
    }   

    messages[i] = pushSize(&state->arena, message_length);
    TextCopy(messages[i], cur_txt);
    
  }
  UnloadFileText(text_file);

}

static void Exit(common_context *ctx, void *data) {

}

static void Update(common_context *ctx, void *data) {

}

static void Render(common_context *ctx, void *data) {

}
