
#include "common.h"

typedef struct grass_chunk {
  int32_t gpuposID;
  int32_t gpudisplacementID;
} grass_chunk;
typedef struct grass_list {
 int32_t chunk_size;
 int32_t chunk_count;
 int32_t side_length;
 grass_chunk *chunks;
} grass_list;

struct SceneState {
  Camera camera;

  int32_t computeShader;

  Shader grassLOD0Shader;
  Shader grassLOD1Shader;
  int32_t grassLOD0VAO;
  int32_t grassLOD1VAO;
  int32_t grassLOD0_vertex_count;
  int32_t grassLOD1_vertex_count;
  int32_t num_of_particles;

  grass_list glist;

  memoryArena arena;
};

static void Entry(common_context *ctx, void **data);
static void Exit(common_context *ctx, void *data);
static void Render(common_context *ctx, void *data);
static void Update(common_context *ctx, void *data);

struct Scene s_grassOnly = {
  .EntryFunction = Entry, 
  .ExitFunction = Exit, 
  .RenderFunction = Render, 
  .UpdateFunction = Update
};

static void GetVerticeRange(v2 *x, v2 *y, v2 *z, float *vertices, uint32_t count);


static void Entry(common_context *ctx, void **data) {

  *data = MemAlloc(sizeof(struct SceneState));
  struct SceneState *state = (struct SceneState *)*data;

  state->arena.size = Megabytes(2);
  state->arena.used = 0;
  state->arena.memory = MemAlloc(state->arena.size);

  char *shaderCode = LoadFileText("assets/shaders/example_compute.glsl");
  int32_t computeData = rlLoadShader(shaderCode, RL_COMPUTE_SHADER);
  state->computeShader = rlLoadShaderProgramCompute(computeData);
  UnloadFileText(shaderCode);

  state->grassLOD0Shader = LoadShader(("assets/shaders/grass_lod0_vert.glsl"), ("assets/shaders/grass_lod0_frag.glsl"));
  state->grassLOD1Shader = LoadShader(("assets/shaders/grass_lod1_vert.glsl"), ("assets/shaders/grass_lod1_frag.glsl"));

  int32_t num_of_work_groups = 1024;
  state->num_of_particles = num_of_work_groups * 9;

  state->glist;
  {
    grass_list zeroList = {0};
    state->glist = zeroList;
  }
  state->glist.chunk_size = 8;
  state->glist.side_length = 4;
  state->glist.chunk_count = state->glist.side_length * state->glist.side_length;
  state->glist.chunks = pushSize(&state->arena, state->glist.chunk_count * sizeof(grass_chunk));

  for (uint32_t i = 0; i < state->glist.chunk_count; i++) {
    state->glist.chunks[i].gpuposID = rlLoadShaderBuffer(state->num_of_particles*sizeof(v4), 0, RL_DYNAMIC_COPY);
  }

  int32_t grid_size = (int32_t)(ceil(sqrt(state->num_of_particles)));

  state->grassLOD0VAO = rlLoadVertexArray();
  state->grassLOD0_vertex_count = 0;
  {
    Model grass = LoadModel("assets/models/grass_Lod0.obj");
    Mesh grass_mesh = grass.meshes[0];
    state->grassLOD0_vertex_count = grass_mesh.vertexCount;

    rlEnableVertexArray(state->grassLOD0VAO);

    rlEnableVertexAttribute(0);
    rlLoadVertexBuffer(grass_mesh.vertices, state->grassLOD0_vertex_count * 3 * sizeof(float), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);

    v2 x_dim, y_dim, z_dim = {0};
    GetVerticeRange(&x_dim, &y_dim, &z_dim, grass_mesh.vertices, state->grassLOD0_vertex_count);

    v2 *buffer = MemAlloc(state->grassLOD0_vertex_count * sizeof(v2));
    uint32_t index = 0;
    for (uint32_t i = 0; i < state->grassLOD0_vertex_count; i++) {
      float x = grass_mesh.vertices[index];
      float y = grass_mesh.vertices[index + 1];

      buffer[i].x = Remap(x, x_dim.x, x_dim.y, 0.0f, 1.0f);
      buffer[i].y = Remap(y, y_dim.x, y_dim.y, 0.0f, 1.0f);

      index += 3;
    }

    rlEnableVertexAttribute(1);
    rlLoadVertexBuffer(buffer, state->grassLOD0_vertex_count * sizeof(v2), false);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, 0, 0);
    
    rlDisableVertexArray();

    MemFree(buffer);
    UnloadModel(grass);
  }

  state->grassLOD1VAO = rlLoadVertexArray();
  state->grassLOD1_vertex_count = 0;
  {
    Model grass = LoadModel("assets/models/grass_Lod1.obj");
    Mesh grass_mesh = grass.meshes[0];
    state->grassLOD1_vertex_count = grass_mesh.vertexCount;

    rlEnableVertexArray(state->grassLOD1VAO);

    rlEnableVertexAttribute(0);
    rlLoadVertexBuffer(grass_mesh.vertices, state->grassLOD1_vertex_count * 3 * sizeof(float), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);

    v2 x_dim, y_dim, z_dim = {0};
    GetVerticeRange(&x_dim, &y_dim, &z_dim, grass_mesh.vertices, state->grassLOD1_vertex_count);

    v2 *buffer = MemAlloc(state->grassLOD1_vertex_count * sizeof(v2));
    uint32_t index = 0;
    for (uint32_t i = 0; i < state->grassLOD1_vertex_count; i++) {
      float x = grass_mesh.vertices[index];
      float y = grass_mesh.vertices[index + 1];

      buffer[i].x = Remap(x, x_dim.x, x_dim.y, 0.0f, 1.0f);
      buffer[i].y = Remap(y, y_dim.x, y_dim.y, 0.0f, 1.0f);

      index += 3;
    }

    rlEnableVertexAttribute(1);
    rlLoadVertexBuffer(buffer, state->grassLOD1_vertex_count * sizeof(v2), false);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, 0, 0);

    rlDisableVertexArray();

    MemFree(buffer);
    UnloadModel(grass);
  }

  state->camera;
  {
    Camera zeroCamera = {0};
    state->camera = zeroCamera;
  }
  state->camera.position = (Vector3){0.0f, 0.0f, -1.2f};
  state->camera.target = (Vector3){0.0f, 0.0f, 0.0f};

  state->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  state->camera.fovy = 60.0f;
  state->camera.projection = CAMERA_PERSPECTIVE;

  {

    rlEnableShader(state->computeShader);
    for (uint32_t i = 0; i < state->glist.chunk_count; i++) {
      float x = (float)(i % (state->glist.side_length));
      float y = (float)(i / (state->glist.side_length));

      v2 chunk_pos = {x, y};

      rlSetUniform(0, &ctx->dt, SHADER_UNIFORM_FLOAT, 1);
      rlSetUniform(1, &grid_size, SHADER_UNIFORM_INT, 1);
      rlSetUniform(2, &state->glist.chunk_size, SHADER_UNIFORM_INT, 1);
      rlSetUniform(3, &state->glist.side_length, SHADER_UNIFORM_INT, 1);
      rlSetUniform(4, &chunk_pos, SHADER_UNIFORM_VEC2, 1);

      rlBindShaderBuffer(state->glist.chunks[i].gpuposID, 0);

      rlComputeShaderDispatch(state->num_of_particles / 1024, 1, 1);
    }
    rlDisableShader();

  }

}

static void Exit(common_context *ctx, void *data) {
  if (!data) {
    return;
  }

  struct SceneState *state = (struct SceneState *)data;
  MemFree(state->arena.memory);
  MemFree(data);
}

static void Update(common_context *ctx, void *data) {
  if (!data) {
    return;
  }

  struct SceneState *state = (struct SceneState *)data;

  PollInputEvents();
  if (IsKeyPressed(KEY_W)) {
    state->camera.position.z += 1.0f;
  }
  if (IsKeyPressed(KEY_S)) {
    state->camera.position.z -= 1.0f;
  }
  if (IsKeyPressed(KEY_A)) {
    state->camera.position.x -= 1.0f;
  }
  if (IsKeyPressed(KEY_D)) {
    state->camera.position.x += 1.0f;
  }
  if (IsKeyPressed(KEY_SPACE)) {
    state->camera.position.y += 1.0f;
  }
  if (IsKeyPressed(KEY_LEFT_SHIFT)) {
    state->camera.position.y -= 1.0f;
  }

}

static void Render(common_context *ctx, void *data) {

  if (!data) {
    return;
  }

  struct SceneState *state = (struct SceneState *)data;

  ClearBackground((Color){0, 0, 0, 0});
  BeginMode3D(state->camera);
  rlDisableBackfaceCulling();

  Matrix projection = rlGetMatrixProjection();
  Matrix view = GetCameraMatrix(state->camera);

  for (uint32_t i = 0; i < state->glist.chunk_count; i++) {
    float rX = ((float)(i % (state->glist.side_length)) - (state->glist.side_length / 2)) + 0.5;
    float rY = ((float)(i / (state->glist.side_length)) - (state->glist.side_length / 2)) + 0.5;

    float time = (float)GetTime();
    float scale = 0.5;

    v3 rChunk_pos = {rX * state->glist.chunk_size, 0.0, rY * state->glist.chunk_size};
    v3 rCamera_pos = state->camera.position;
    float chunk_distance = Vector3Distance(rCamera_pos, rChunk_pos);

    uint32_t vertexCount = 0;
    float lod_distance = state->glist.chunk_size;
    if (chunk_distance < lod_distance * 2.0f) {
      rlEnableShader(state->grassLOD0Shader.id);
      rlEnableVertexArray(state->grassLOD0VAO);

      SetShaderValueMatrix(state->grassLOD0Shader, 0, projection);
      SetShaderValueMatrix(state->grassLOD0Shader, 1, view);

      vertexCount = state->grassLOD0_vertex_count;

      rlSetUniform(3, &time, SHADER_UNIFORM_FLOAT, 1);
      rlSetUniform(4, &scale, SHADER_UNIFORM_FLOAT, 1);

    } else if (chunk_distance < lod_distance * 3.0f) {
      rlEnableShader(state->grassLOD0Shader.id);
      rlEnableVertexArray(state->grassLOD1VAO);

      SetShaderValueMatrix(state->grassLOD0Shader, 0, projection);
      SetShaderValueMatrix(state->grassLOD0Shader, 1, view);

      vertexCount = state->grassLOD1_vertex_count;

      rlSetUniform(3, &time, SHADER_UNIFORM_FLOAT, 1);
      rlSetUniform(4, &scale, SHADER_UNIFORM_FLOAT, 1);
    } else if (chunk_distance < lod_distance * 4.0f) {
      rlEnableShader(state->grassLOD1Shader.id);
      rlEnableVertexArray(state->grassLOD1VAO);

      SetShaderValueMatrix(state->grassLOD1Shader, 0, projection);
      SetShaderValueMatrix(state->grassLOD1Shader, 1, view);

      vertexCount = state->grassLOD1_vertex_count;
    }

    float x = (float)(i % (state->glist.side_length));
    float y = (float)(i / (state->glist.side_length));

    v2 chunk_pos = {x, y};

    rlSetUniform(2, &chunk_pos, SHADER_UNIFORM_VEC2, 1);

    rlBindShaderBuffer(state->glist.chunks[i].gpuposID, 0);
    rlDrawVertexArrayInstanced(0, vertexCount, state->num_of_particles);

    rlDisableVertexArray();
    rlDisableShader();

  }

  rlEnableBackfaceCulling();
  EndMode3D();

}

static void GetVerticeRange(v2 *x, v2 *y, v2 *z, float *vertices, uint32_t count) {

  v2 x_dim = {0};
  v2 y_dim = {0};
  v2 z_dim = {0};

  uint32_t index = 0;
  for (uint32_t i = 0; i < count; i++) {
    float x = vertices[index];
    float y = vertices[index + 1];
    float z = vertices[index + 2];
    
    if (i == 0) {
      x_dim = (v2){x, x};
      y_dim = (v2){y, y};
      z_dim = (v2){z, z};
      continue;
    }

    if (x < x_dim.x) { x_dim.x = x; }
    if (x > x_dim.y) { x_dim.y = x; }
    if (y < y_dim.x) { y_dim.x = y; }
    if (y > y_dim.y) { y_dim.y = y; }
    if (z < z_dim.x) { z_dim.x = z; }
    if (z > z_dim.y) { z_dim.y = z; }

    // Advance by 3 because we are looking at a buffer of floats not bytes
    // index += sizeof(v3); -Bwop
    index += 3;   
  }
  
  *x = x_dim;
  *y = y_dim;
  *z = z_dim;
}
