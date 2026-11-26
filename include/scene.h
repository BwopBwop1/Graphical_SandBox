#ifndef SCENE_CORE
#define SCENE_CORE

typedef struct Scene Scene;
typedef struct SceneList SceneList;
typedef struct common_context common_context;

// TODO: A better scheme for Scenes might be each Scene will Describe itself so that name collision
// is limited only to the struct Scene name instead of the four functions needed 
// for operation.
struct Scene {
  void (*EntryFunction)(common_context *, void **);
  void (*ExitFunction)(common_context *, void *);
  void (*UpdateFunction)(common_context *, void *);
  void (*RenderFunction)(common_context *, void *);

  void *data;
};

struct SceneList {
  Scene *activeScene;
  Scene *buf;

  uint32_t size;
  uint32_t count;
};

struct common_context {
  float dt;
  struct SceneList *list;
};

// TODO: Idiot Proof this, somehow.
static inline void SetActiveScene(common_context *ctx, Scene *newScene) {

  Scene *activeScene = ctx->list->activeScene;

  if (activeScene) {
    if (activeScene->ExitFunction) {
      activeScene->ExitFunction(ctx, activeScene->data);
    }
  }

  if (newScene) {
    if (newScene->EntryFunction) {
      newScene->EntryFunction(ctx, &newScene->data);
      ctx->list->activeScene = newScene;
    }
  }
}

#endif
