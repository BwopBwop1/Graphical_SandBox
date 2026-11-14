
#include "common.h"

struct saver_SceneState {
  memoryArena arena;
};

static void saver_EntryFunction(common_context *ctx, void **data) {

  *data = MemAlloc(sizeof(struct saver_SceneState));
  struct saver_SceneState *state = (struct saver_SceneState *)*data;

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

static void saver_ExitFunction(common_context *ctx, void *data) {

}

static void saver_UpdateFunction(common_context *ctx, void *data) {

}

static void saver_RenderFunction(common_context *ctx, void *data) {

}
