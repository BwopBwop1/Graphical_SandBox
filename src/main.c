
// gcc complains about sdl unless done this way probably a user error.
// ffs. -Byte
// gcc -o Test.out main.c `pkg-config --cflags --libs sdl3 stb` -lm
//
// gcc -o Test.out main.c `pkg-config --cflags --libs sdl3 sdl3-ttf` -lm
// gcc --debug  -o Test.out src/main.c `pkg-config --cflags --libs raylib glew` -lm

/*
*  TODO: List of things until application is feature complete-ish.
*  Performant Grass Shader. - 0%
*  Sky box for hiding screen. - 0%
*  Text popups - 0%
*  Global input. - 40%
*  Decent-ish Lighting - 0%
*  Clouds - 0%
*  Water / reflections. - 0%
*  Trees. - 0%
*/

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>

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

typedef struct event {
 int32_t type;
 int32_t scancode;
 uint16_t modstate;
} event;

typedef struct memoryArena {
 uint32_t size;
 uint32_t used;

 void *memory;
} memoryArena;

int32_t connect_to_listener(int32_t *fd);
int32_t check_and_get_event(int32_t fd);
void *pushSize(memoryArena *arena, size_t size);


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

float GetRandomFloat(float from, float to) {
    return from + (to-from)*(float)GetRandomValue(0, INT_MAX) / INT_MAX;
}

void GetVerticeRange(v2 *x, v2 *y, v2 *z, float *vertices, uint32_t count) {

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



void main() {
  memoryArena arena;

  int32_t file_descriptor = 0;
  connect_to_listener(&file_descriptor);
  
  arena.size = Megabytes(2);
  arena.used = 0;
  arena.memory = malloc(arena.size);
  

  char *text_file = LoadFileText("assets/txts/gone_away_messages.txt");
  int32_t total_messages = 0;
  char **temp_messages = TextSplit(text_file, '\n', &total_messages);
  void **messages = pushSize(&arena, sizeof(void*) * total_messages - 1);

  for (uint32_t i = 0; i < total_messages - 1; i++) {
    char *cur_txt = temp_messages[i];
    uint32_t message_length = TextLength(cur_txt);

    if (message_length == 0) {
     continue;
    }   

    messages[i] = pushSize(&arena, message_length);
    TextCopy(messages[i], cur_txt);
    
  }
  UnloadFileText(text_file);
  
  SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_TOPMOST);
  InitWindow(960, 540, "Window");
  SetTargetFPS(60);

  char *shaderCode = LoadFileText("assets/shaders/example_compute.glsl");
  int32_t computeData = rlLoadShader(shaderCode, RL_COMPUTE_SHADER);
  int32_t computeShader = rlLoadShaderProgramCompute(computeData);
  UnloadFileText(shaderCode);

  Shader grassLOD0Shader = LoadShader(("assets/shaders/grass_lod0_vert.glsl"), ("assets/shaders/grass_lod0_frag.glsl"));
  Shader grassLOD1Shader = LoadShader(("assets/shaders/grass_lod1_vert.glsl"), ("assets/shaders/grass_lod1_frag.glsl"));

  int32_t num_of_work_groups = 1024;
  int32_t num_of_particles = num_of_work_groups * 9;

  grass_list glist = {0};
  glist.chunk_size = 8;
  glist.side_length = 4;
  glist.chunk_count = glist.side_length * glist.side_length;
  glist.chunks = pushSize(&arena, glist.chunk_count * sizeof(grass_chunk));

  // NOTE: Is there any reason to care about the usage flag?
  
  for (uint32_t i = 0; i < glist.chunk_count; i++) {
    glist.chunks[i].gpuposID = rlLoadShaderBuffer(num_of_particles*sizeof(v4), 0, RL_DYNAMIC_COPY);
  }

  SetTraceLogLevel(LOG_ALL);


  int32_t grid_size = (int32_t)(ceil(sqrt(num_of_particles)));


  int grassLOD0VAO = rlLoadVertexArray();
  uint32_t grassLOD0_vertex_count = 0;
  {
    Model grass = LoadModel("assets/models/grass_Lod0.obj");
    Mesh grass_mesh = grass.meshes[0];
    grassLOD0_vertex_count = grass_mesh.vertexCount;

    rlEnableVertexArray(grassLOD0VAO);

    rlEnableVertexAttribute(0);
    rlLoadVertexBuffer(grass_mesh.vertices, grassLOD0_vertex_count * 3 * sizeof(float), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);

    v2 x_dim, y_dim, z_dim = {0};
    GetVerticeRange(&x_dim, &y_dim, &z_dim, grass_mesh.vertices, grassLOD0_vertex_count);

    v2 *buffer = MemAlloc(grassLOD0_vertex_count * sizeof(v2));
    uint32_t index = 0;
    for (uint32_t i = 0; i < grassLOD0_vertex_count; i++) {
      float x = grass_mesh.vertices[index];
      float y = grass_mesh.vertices[index + 1];

      buffer[i].x = Remap(x, x_dim.x, x_dim.y, 0.0f, 1.0f);
      buffer[i].y = Remap(y, y_dim.x, y_dim.y, 0.0f, 1.0f);

      index += 3;
    }

    rlEnableVertexAttribute(1);
    rlLoadVertexBuffer(buffer, grassLOD0_vertex_count * sizeof(v2), false);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, 0, 0);
    

    rlDisableVertexArray();

    MemFree(buffer);
    UnloadModel(grass);
  }

  int grassLOD1VAO = rlLoadVertexArray();
  uint32_t grassLOD1_vertex_count = 0;
  {
    Model grass = LoadModel("assets/models/grass_Lod1.obj");
    Mesh grass_mesh = grass.meshes[0];
    grassLOD1_vertex_count = grass_mesh.vertexCount;

    rlEnableVertexArray(grassLOD1VAO);

    rlEnableVertexAttribute(0);
    rlLoadVertexBuffer(grass_mesh.vertices, grassLOD1_vertex_count * 3 * sizeof(float), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);

    v2 x_dim, y_dim, z_dim = {0};
    GetVerticeRange(&x_dim, &y_dim, &z_dim, grass_mesh.vertices, grassLOD1_vertex_count);

    v2 *buffer = MemAlloc(grassLOD1_vertex_count * sizeof(v2));
    uint32_t index = 0;
    for (uint32_t i = 0; i < grassLOD1_vertex_count; i++) {
      float x = grass_mesh.vertices[index];
      float y = grass_mesh.vertices[index + 1];

      buffer[i].x = Remap(x, x_dim.x, x_dim.y, 0.0f, 1.0f);
      buffer[i].y = Remap(y, y_dim.x, y_dim.y, 0.0f, 1.0f);

      index += 3;
    }

    rlEnableVertexAttribute(1);
    rlLoadVertexBuffer(buffer, grassLOD1_vertex_count * sizeof(v2), false);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, 0, 0);

    rlDisableVertexArray();

    MemFree(buffer);
    UnloadModel(grass);
  }


  // WARNING: A bug, with either glfw or raylib, with the way window flags are set.
  // if the Mouse passthrough is done at window initialization it doesn't seem to work.
  // NOTE: The implementation difference being that window initialization does the flag setting
  // through glfwWindowHint and SetWindowState does it's flag setting through glfwSetWindowAttrib.

  Camera camera = {0};
  camera.position = (Vector3){0.0f, 0.0f, -1.2f};
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};

  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 60.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  int32_t compute_buffer_ran = 0;

  float dt = 0;

  {
    printf("Running the compute shader\n");

    rlEnableShader(computeShader);
    for (uint32_t i = 0; i < glist.chunk_count; i++) {
      float x = (float)(i % (glist.side_length));
      float y = (float)(i / (glist.side_length));

      v2 chunk_pos = {x, y};

      rlSetUniform(0, &dt, SHADER_UNIFORM_FLOAT, 1);
      rlSetUniform(1, &grid_size, SHADER_UNIFORM_INT, 1);
      rlSetUniform(2, &glist.chunk_size, SHADER_UNIFORM_INT, 1);
      rlSetUniform(3, &glist.side_length, SHADER_UNIFORM_INT, 1);
      rlSetUniform(4, &chunk_pos, SHADER_UNIFORM_VEC2, 1);

      rlBindShaderBuffer(glist.chunks[i].gpuposID, 0);

      rlComputeShaderDispatch(num_of_particles / 1024, 1, 1);
    }
    rlDisableShader();

    compute_buffer_ran = 1;
  }

  int32_t c_monitor = GetCurrentMonitor();

  SetWindowSize(GetMonitorWidth(c_monitor), GetMonitorHeight(c_monitor));
  SetWindowPosition(0, 0);
  SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_MOUSE_PASSTHROUGH);

  double last = GetTime();
  double now = GetTime();

  int32_t refresh_rate = GetMonitorRefreshRate(c_monitor);

  while (!WindowShouldClose()) {

    PollInputEvents();
    if (IsKeyPressed(KEY_W)) {
      camera.position.z += 1.0f;
    }
    if (IsKeyPressed(KEY_S)) {
      camera.position.z -= 1.0f;
    }
    if (IsKeyPressed(KEY_A)) {
      camera.position.x -= 1.0f;
    }
    if (IsKeyPressed(KEY_D)) {
      camera.position.x += 1.0f;
    }
    if (IsKeyPressed(KEY_SPACE)) {
      camera.position.y += 1.0f;
    }
    if (IsKeyPressed(KEY_LEFT_SHIFT)) {
      camera.position.y -= 1.0f;
    }
    
    BeginDrawing();
    ClearBackground((Color){0, 0, 0, 0});
    BeginMode3D(camera);
    if (compute_buffer_ran) {

      rlDisableBackfaceCulling();

      Matrix projection = rlGetMatrixProjection();
      Matrix view = GetCameraMatrix(camera);

      for (uint32_t i = 0; i < glist.chunk_count; i++) {
        float rX = ((float)(i % (glist.side_length)) - (glist.side_length / 2)) + 0.5;
        float rY = ((float)(i / (glist.side_length)) - (glist.side_length / 2)) + 0.5;

        float time = (float)GetTime();
        float scale = 0.5;

        v3 rChunk_pos = {rX * glist.chunk_size, 0.0, rY * glist.chunk_size};
        v3 rCamera_pos = camera.position;
        float chunk_distance = Vector3Distance(rCamera_pos, rChunk_pos);

        uint32_t vertexCount = 0;
        float lod_distance = glist.chunk_size;
        if (chunk_distance < lod_distance * 2.0f) {
          rlEnableShader(grassLOD0Shader.id);
          rlEnableVertexArray(grassLOD0VAO);

          SetShaderValueMatrix(grassLOD0Shader, 0, projection);
          SetShaderValueMatrix(grassLOD0Shader, 1, view);

          vertexCount = grassLOD0_vertex_count;

          rlSetUniform(3, &time, SHADER_UNIFORM_FLOAT, 1);
          rlSetUniform(4, &scale, SHADER_UNIFORM_FLOAT, 1);
        } else if (chunk_distance < lod_distance * 3.0f) {
          rlEnableShader(grassLOD0Shader.id);
          rlEnableVertexArray(grassLOD1VAO);

          SetShaderValueMatrix(grassLOD0Shader, 0, projection);
          SetShaderValueMatrix(grassLOD0Shader, 1, view);

          vertexCount = grassLOD1_vertex_count;

          rlSetUniform(3, &time, SHADER_UNIFORM_FLOAT, 1);
          rlSetUniform(4, &scale, SHADER_UNIFORM_FLOAT, 1);
        } else if (chunk_distance < lod_distance * 4.0f) {
          rlEnableShader(grassLOD1Shader.id);
          rlEnableVertexArray(grassLOD1VAO);

          SetShaderValueMatrix(grassLOD1Shader, 0, projection);
          SetShaderValueMatrix(grassLOD1Shader, 1, view);

          vertexCount = grassLOD1_vertex_count;
        }

        

        float x = (float)(i % (glist.side_length));
        float y = (float)(i / (glist.side_length));

        v2 chunk_pos = {x, y};

        rlSetUniform(2, &chunk_pos, SHADER_UNIFORM_VEC2, 1);

        rlBindShaderBuffer(glist.chunks[i].gpuposID, 0);
        rlDrawVertexArrayInstanced(0, vertexCount, num_of_particles);

        rlDisableVertexArray();
        rlDisableShader();
      }
      rlEnableBackfaceCulling();

      /*
      for (uint32_t i = 0; i < glist.chunk_count; i++) { 

        float rX = ((float)(i % (glist.side_length)) - (glist.side_length / 2)) + 0.5;
        float rY = ((float)(i / (glist.side_length)) - (glist.side_length / 2)) + 0.5;

        v3 rChunk_pos = {rX * glist.chunk_size, 0.0, rY * glist.chunk_size};
        v3 rCamera_pos = camera.position;
        float chunk_distance = Vector3Distance(rCamera_pos, rChunk_pos);

        if (chunk_distance < glist.chunk_size) {
          DrawSphere(rChunk_pos, 0.5f, GREEN);
        }

        if (chunk_distance > glist.chunk_size) {
          DrawSphere(rChunk_pos, 0.5f, YELLOW);
        }

        if (chunk_distance > glist.chunk_size + glist.chunk_size) {
          DrawSphere(rChunk_pos, 0.5f, RED);
        }

      }
      */

      //rlDrawVertexArrayElementsInstanced(0, ArrayCount(indices), indices, num_of_particles);
      

    }

    EndMode3D();

    EndDrawing();

    SwapScreenBuffer();
    
    now = GetTime();

    double updateDrawTime = now - last;
    double waitTime = (1.0f / (float)refresh_rate) - updateDrawTime;
    if (waitTime > 0.0) {
      WaitTime(waitTime);
      now = GetTime();
      dt = (float)(now - last);
    }

    last = now;
  }

  free(arena.memory);

  CloseWindow();
}

void *pushSize(memoryArena *arena, size_t size) {
  uint8_t *result = 0;
  if (arena->used + size < arena->size && size > 0) {
    result = (uint8_t *)arena->memory + arena->used;

    arena->used += size;
  }
  return (void *)(result);
}

int32_t connect_to_listener(int32_t *fd) {
  const char *path = "/run/user/1000/keyboardListener";

  struct sockaddr_un sockaddr = { .sun_family = AF_UNIX };
  strcpy(sockaddr.sun_path, path);

  int32_t socket_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (socket_fd == -1) {
    printf("Unable to open socket\n");
    printf("errno value %s\n", strerror(errno));
    return 1;
  }

  int32_t connect_result = connect(socket_fd, 
                                  (struct sockaddr *)&sockaddr, 
                                  offsetof(struct sockaddr_un, sun_path) + strlen(sockaddr.sun_path) + 1);

  if (connect_result == -1) {
    printf("Unable to connect to socket\n");
    printf("errno value %s\n", strerror(errno));
    close(socket_fd);
  return 1;
  }

  int setflag_result = fcntl(socket_fd, F_SETFL, O_NONBLOCK);
  if (setflag_result == -1) {
    printf("Couldn't set socket to non blocking\n");
    printf("errno value %s\n", strerror(errno));
    close(socket_fd);
    return 1;
  } 

  if (socket_fd != 0) {
    *fd = socket_fd;
  }

  return 0;
}

int32_t check_and_get_event(int32_t fd) {

  if (fd <= 0) {
    return 0;
  }

  struct event read_events[MAX_READ_EVENTS];
  int32_t read_events_size = sizeof(struct event) * MAX_READ_EVENTS;

  ssize_t bytes_read = read(fd, read_events, read_events_size);

  if (bytes_read == -1) {
    return 0;
  }

  if (bytes_read < sizeof(struct event)) {
    return 0;
  }

  int32_t num_of_events = bytes_read / sizeof(struct event);

  return 1;
}
