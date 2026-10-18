
// gcc complains about sdl unless done this way probably a user error.
// ffs. -Byte
// gcc -o Test.out main.c `pkg-config --cflags --libs sdl3 stb` -lm
//
// gcc -o Test.out main.c `pkg-config --cflags --libs sdl3 sdl3-ttf` -lm
// gcc --debug  -o Test.out main.c `pkg-config --cflags --libs sdl3 sdl3-ttf glew` -lm

// SDL has a standard library
#include <stdio.h>
#include <stdlib.h>
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

#define SDL_MAIN_USE_CALLBACKS 1
#include "SDL3/SDL.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"

#include <SDL3_ttf/SDL_ttf.h>

#include <GL/glew.h>

#define TAU 6.283185
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

typedef struct particle {
 float x, y;
 float dx, dy;
 float lifetime;
} particle;

// The head index is so that it will bias 
// towards looking at old particles first.
typedef struct particle_system {
 particle particles[MAX_PARTICLE];
 size_t count;
 size_t head;
 int32_t size;
} particle_system;

typedef struct v3 {
 float x, y, z;
} v3;

typedef struct v4 {
 float x, y, z, w;
} v4;

typedef struct CameraData {
 float left_right_angle;
 float up_down_angle;
 
 v3 position;
 v3 forward;
 v3 right;

 float FOV;
 float Aspect;
 float NearPlane;
 float FarPlane;
} CameraData;

typedef struct mat4 {
 union {
  /* This is superior row column major*/
  float data[16];
  v4 m[4];

  struct {
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
  };
 };
} mat4;

int32_t connect_to_listener(int32_t *fd);
int32_t check_and_get_event(int32_t fd, SDL_Window *win);

void str_zero(uint8_t *b1);
int32_t str_length(uint8_t *b1);
bool str_compare(uint8_t *b1, uint8_t *b2);
void str_copy_to(uint8_t *b1, uint32_t amount, uint8_t *b2);
uint8_t * str_copy_inplace(const uint8_t *b1, uint32_t amount, uint8_t *b2);
void str_total(uint8_t *file, uint8_t delimiter, uint32_t *total);
void str_cut(uint8_t *file, int32_t idx, int32_t *location, int32_t *size, uint8_t delimiter);

void generate_text_texture(SDL_Texture **tex, uint8_t *txt, SDL_Color fg, SDL_Color bg);
void draw_text(SDL_Renderer *renderer, float x, float y, float scale, SDL_Texture *tex);

Uint32 spawn_particle(void *userdata, SDL_TimerID id, Uint32 interval);
Uint32 change_text(void *userdata, SDL_TimerID id, Uint32 interval);

void *pushSize(memoryArena *arena, size_t size);

// Flipped internally for the sanity of the rest of us.
v3 subtraction(v3 a, v3 b);
v3 normalize(v3 in);
v3 cross(v3 a, v3 b);
float dot(v3 a, v3 b);
mat4 mat4_multiply_mat4(mat4 a, mat4 b);
mat4 mat4_mul_identiy(void);
mat4 mat4_transpose(mat4 in);
mat4 mat4_addToPosition(mat4 in, v4 b);
mat4 project_perspective(float aspect, float fov, float near, float far);
mat4 lookAt(v3 eye, v3 target, v3 up);
mat4 MatrixFromPosRot(v3 eye, float pitch, float yaw);
v4 mat4_transform_vector(mat4 in, v4 b);
v4 mat4_premultiply_vector(mat4 in, v4 b);
v4 mat4_postmultiply_vector(mat4 in, v4 b);

void pretty_print_mat4(mat4 in);

particle_system ps;
memoryArena arena;

bool GL_LoadShader(GLuint *uiShader, GLenum ShaderType, const GLchar *ShaderSrc);
bool GL_LoadProgram(GLuint *uiShader, GLenum uiVertShader, GLenum uiFragShader);
bool GL_PrettyPrint(void);

// Two textures to swap what text is drawn for fade in/out.
SDL_Texture *a_Texture = 0;
SDL_Texture *b_Texture = 0;

const char *font_path = ("assets/fonts/font.ttf");
const char *gone_away_path = ("assets/txts/gone_away_messages.txt");

const char *vert_path = ("assets/shaders/basic.vert");
const char *frag_path = ("assets/shaders/basic.frag");

void **messages = 0;
int32_t total_messages = 0;

SDL_Window *window = 0;
SDL_GLContext context = 0;

TTF_Font *font = 0;

double nowMS = 0;
double lastMS = 0;

int32_t dirty = 0;
float refreshRatePerSecond = 60;
float targetMS = 0;

int32_t file_descriptor = 0;

GLuint g_uiVAO;
GLuint g_uiVBO;
GLuint g_uiIBO;
GLuint g_uiMainProgram;

GLfloat VertexData[] = {
   0.5f,  0.5f, -0.5f,
   0.5f, -0.5f, -0.5f,
  -0.5f, -0.5f, -0.5f,
  -0.5f,  0.5f, -0.5f,
  -0.5f, -0.5f,  0.5f,
  -0.5f,  0.5f,  0.5f,
   0.5f, -0.5f,  0.5f,
   0.5f,  0.5f,  0.5f,
};

GLuint IndexData[] = {
 0, 1, 3, 3, 1, 2,
 3, 2, 5, 5, 2, 4,
 1, 6, 2, 2, 6, 4,
 5, 4, 7, 7, 4, 6,
 7, 6, 0, 0, 6, 1,
 7, 0, 5, 5, 0, 3,
};

CameraData localCamera = {0};

mat4 viewTransform = {0};
GLuint ViewTransformUBO;

// NOTE: I might want to put all the application state in a structure.
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {

  arena.size = Megabytes(2);
  arena.memory = malloc(arena.size);

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  window = SDL_CreateWindow("Test Application", 960, 540, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);
  if (window == NULL) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  context = SDL_GL_CreateContext(window);
  if (context == NULL) {
    SDL_Log("Couldn't create opengl context: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GL_SetSwapInterval(-1);

  glewExperimental = GL_TRUE;
  GLenum GlewError = glewInit();
  if (GlewError != GLEW_OK) {
    SDL_Log("I Couldn't glew it no more: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!TTF_Init()) {
    SDL_Log("Couldn't initialize SDL_ttf: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  font = TTF_OpenFontIO(SDL_IOFromFile(font_path, "r"), true, 96.0f);
  if (!font) {
    SDL_Log("Couldn't open font: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

 
  if (!SDL_SetWindowFullscreen(window, true)) {
    SDL_Log("Couldn't make window fullscreen: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
 
  lastMS = (SDL_GetTicks());

  int32_t displayID = SDL_GetPrimaryDisplay();
  const SDL_DisplayMode *displayMode = SDL_GetCurrentDisplayMode(displayID);
  refreshRatePerSecond = displayMode->refresh_rate;
  glViewport(0, 0, displayMode->w, displayMode->h);
  targetMS = 1000.0 / refreshRatePerSecond;

  
  connect_to_listener(&file_descriptor);

  SDL_IOStream *vertex_file = SDL_IOFromFile(vert_path, "r");
  int32_t vertex_file_size = SDL_GetIOSize(vertex_file);
  char *vertex_src = pushSize(&arena, vertex_file_size + 1); // + 1 is for null termination.
  SDL_ReadIO(vertex_file, vertex_src, vertex_file_size);

  SDL_IOStream *fragment_file = SDL_IOFromFile(frag_path, "r");
  int32_t fragment_file_size = SDL_GetIOSize(fragment_file);
  char *fragment_src = pushSize(&arena, fragment_file_size + 1); // + 1 is for null termination.
  SDL_ReadIO(fragment_file, fragment_src, fragment_file_size);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_STENCIL_TEST);

  GLuint uiVertShader;
  GLuint uiFragShader;
  if (!GL_LoadShader(&uiVertShader, GL_VERTEX_SHADER, vertex_src)) {
    SDL_Log("Couldn't load vert shader : %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!GL_LoadShader(&uiFragShader, GL_FRAGMENT_SHADER, fragment_src)) {
    SDL_Log("Couldn't load frag shader: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!GL_LoadProgram(&g_uiMainProgram, uiVertShader, uiFragShader)) {
    SDL_Log("Couldn't create main program: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  
  glDeleteShader(uiVertShader);
  glDeleteShader(uiFragShader);

  memset(arena.memory, 0, arena.used);
  arena.used = 0;

  GL_PrettyPrint();

  glGenVertexArrays(1, &g_uiVAO);
  glBindVertexArray(g_uiVAO);

  glGenBuffers(1, &g_uiVBO);
  glBindBuffer(GL_ARRAY_BUFFER, g_uiVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);

  glGenBuffers(1, &g_uiIBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_uiIBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexData), IndexData, GL_STATIC_DRAW);
  
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3 * sizeof(GLfloat)), 0);
  glEnableVertexAttribArray(0);
  
  glUseProgram(g_uiMainProgram);

  viewTransform = mat4_mul_identiy();

  localCamera.left_right_angle = 0.0;
  localCamera.up_down_angle = 0.0;

  localCamera.position = (v3){0.0f, 0.0f, 12.0f};
  localCamera.forward = (v3){0.0f, 0.0f, -1.0f};
  localCamera.right = (v3){1.0f, 0.0, 0.0f};

  localCamera.NearPlane = 0.1;
  localCamera.FarPlane = 100.0;
  localCamera.Aspect = ((float)displayMode->h / (float)displayMode->w);
  localCamera.FOV = 60;
  
  mat4 view = MatrixFromPosRot(localCamera.position, 0, degToRad(180));
  mat4 projection = project_perspective(localCamera.Aspect, 
                                        degToRad(localCamera.FOV), 
                                        localCamera.NearPlane, 
                                        localCamera.FarPlane);

  viewTransform = mat4_multiply_mat4(projection, view);

  pretty_print_mat4(view);
  pretty_print_mat4(projection);
  pretty_print_mat4(viewTransform);

  glGenBuffers(1, &ViewTransformUBO);
  glBindBuffer(GL_UNIFORM_BUFFER, ViewTransformUBO);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), &viewTransform.data, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, ViewTransformUBO);


  return SDL_APP_CONTINUE;  
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  switch (event->type) {
    case SDL_EVENT_KEY_DOWN: {
      // Quick Escape.
      if (event->key.key == SDLK_ESCAPE && !event->key.repeat) {
        return SDL_APP_SUCCESS;
      } 
    } break;
    case SDL_EVENT_KEY_UP: {
     // TODO: Handle key up events.
    } break;
    case SDL_EVENT_QUIT: {
      return SDL_APP_SUCCESS;
    } break;

    default: {
    } break;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  nowMS = SDL_GetTicks();
  float time_step = (float)(nowMS - lastMS) / 1000.0; // Seconds

  if (check_and_get_event(file_descriptor, window)) {

  } else {
  // TODO: Try and recoonect.
  }

  { // Draw A Cube.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(g_uiVAO);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
  }

  SDL_GL_SwapWindow(window);

  float stepMS = (float)(nowMS - lastMS);
  if (stepMS < targetMS) {
    SDL_Delay((targetMS - stepMS));
  }
  lastMS = nowMS;

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  // TODO: Actual Object handling.
  glDeleteProgram(g_uiMainProgram);
  glDeleteBuffers(1, &g_uiVBO);
  glDeleteBuffers(1, &g_uiIBO);
  glDeleteVertexArrays(1, &g_uiVAO);
  glDeleteBuffers(1, &ViewTransformUBO);

  free(arena.memory);
  
  SDL_GL_DestroyContext(context);
  SDL_DestroyTexture(a_Texture);
  SDL_DestroyWindow(window);

  if (font) {
    TTF_CloseFont(font);
  }
  TTF_Quit();

}

int32_t str_length(uint8_t *b1) {
  int32_t result = 0;
  for (const uint8_t *c = b1; *c; c++) {
    result++;
  }
  return result;
}

void str_zero(uint8_t *b1) {
  for (uint8_t *c = b1; *c; c++) { 
    *c = 0;
  }
}

bool str_compare(uint8_t *b1, uint8_t *b2) {

  if (str_length(b1) != str_length(b2)) {
    return false;
  }

  for (uint8_t *c = b1; *c; c++) {
    uint8_t *b = b2++;

    if (*b == *c) {
      continue;
    }

    return false;
  }

  return true;
}

void str_copy_to(uint8_t *b1, uint32_t amount, uint8_t *b2) {
  uint8_t *src = b1;
  uint8_t *dst = b2;

  for (uint32_t i = 0; i < amount; i++) {
    *dst++ = *src++;
  }
}

uint8_t * str_copy_inplace(const uint8_t *b1, uint32_t amount, uint8_t *b2) {
  const char *src = b1;
  char *dst = b2;

  for (int i = 0; i < amount; i++) {
    *dst++ = *src++;
  }
  return dst;
}

void str_total(uint8_t *file, uint8_t delimiter, uint32_t *total) {
  int32_t t = 0;
  for (uint8_t *c = file; *c; c++) { 
    uint8_t current_char = *c;

    if (current_char == delimiter) {
      t++;
    }
  }

  *total = t;
}

void str_cut(uint8_t *file, int32_t idx, int32_t *location, int32_t *size, uint8_t delimiter) {
  int32_t loc = 0;
  int32_t s = 0;

  int32_t index = 0;
  int32_t i = 0;

  for (uint8_t *c = file; *c; c++) { 
    uint8_t current_char = *c;

    if (current_char == delimiter) {
      s = i - loc;

      if (index == idx) {
        break;
      } 

      loc = i + 1;
      index++;
    }

    i++;
  }

  *location = loc;
  *size = s;
}

Uint32 change_text(void *userdata, SDL_TimerID id, Uint32 interval) {

  dirty = 1;

  return interval;
}

void *pushSize(memoryArena *arena, size_t size) {
  uint8_t *result = 0;
  if (arena->used + size < arena->size) {
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

int32_t check_and_get_event(int32_t fd, SDL_Window *win) {

  if (fd <= 0) {
    return 0;
  }

  struct event read_events[MAX_READ_EVENTS];
  int32_t read_events_size = sizeof(struct event) * MAX_READ_EVENTS;

  ssize_t bytes_read = read(fd, read_events, read_events_size);

  if (bytes_read == -1) {
    return 0;
  }

  uint32_t flags = SDL_GetWindowFlags(win);
  if (flags & SDL_WINDOW_INPUT_FOCUS) {
    return 0;
  }

  if (bytes_read < sizeof(struct event)) {
    return 0;
  }

  int32_t num_of_events = bytes_read / sizeof(struct event);
  for (uint32_t i = 0; i < num_of_events; i++) {
    SDL_Event ev = {0};

    ev.type = read_events[i].type;
    ev.key.timestamp = SDL_GetTicksNS();
    ev.key.scancode = read_events[i].scancode;
    ev.key.mod = read_events[i].modstate;
    ev.key.key = SDL_GetKeyFromScancode(ev.key.scancode, ev.key.mod, true);
    ev.key.repeat = 0;

    if (!SDL_PushEvent(&ev)) {
      SDL_Log("Push Event either failed or was filtered: %s", SDL_GetError());
    }

  }

  return 1;
}

bool GL_LoadShader(GLuint *uiShader, GLenum ShaderType, const GLchar *ShaderSrc) {
  GLuint _uiShader = *uiShader;

  _uiShader = glCreateShader(ShaderType);
  glShaderSource(_uiShader, 1, &ShaderSrc, NULL);
  glCompileShader(_uiShader);

  GLint iTestReturn;
  glGetShaderiv(_uiShader, GL_COMPILE_STATUS, &iTestReturn);
  if (iTestReturn == GL_FALSE) {
      GLchar p_cInfoLog[1024];
      int32_t iErrorLength;
      glGetShaderInfoLog(_uiShader, 1024, &iErrorLength, p_cInfoLog);
      SDL_Log("Couldn't initialize SDL: %s", p_cInfoLog);
      return false;
  }

  *uiShader = _uiShader;

  return true;
}

bool GL_LoadProgram(GLuint *uiShader, GLenum uiVertShader, GLenum uiFragShader) {
  GLuint _uiShader = *uiShader;

  _uiShader = glCreateProgram();
  glAttachShader(_uiShader, uiVertShader);
  glAttachShader(_uiShader, uiFragShader);
  glLinkProgram(_uiShader);

  GLint iTestReturn;
  glGetShaderiv(_uiShader, GL_LINK_STATUS, &iTestReturn);
  if (iTestReturn == GL_FALSE) {
      GLchar p_cInfoLog[1024];
      int32_t iErrorLength;
      glGetShaderInfoLog(_uiShader, 1024, &iErrorLength, p_cInfoLog);
      SDL_Log("Couldn't initialize SDL: %s", p_cInfoLog);
      return false;
  }

  *uiShader = _uiShader;

  return true;
}

mat4 mat4_mul_identiy(void) {
  mat4 result = {0};

  result.m00 = 1.0f;
  result.m11 = 1.0f;
  result.m22 = 1.0f;
  result.m33 = 1.0f;

  return result;
}

mat4 mat4_transpose(mat4 in) {
  mat4 result = {0};

  return result;
}

mat4 mat4_addToPosition(mat4 in, v4 b) {
  mat4 result = in;

  result.m03 += b.x;
  result.m13 += b.y;
  result.m23 += b.z;

  return result;
};

v4 mat4_premultiply_vector(mat4 in, v4 b) {
 v4 result = {0};

 result.x = ((in.m00 * b.x) + (in.m01 * b.y) + (in.m02 * b.z) + (in.m03 * b.w));
 result.y = ((in.m10 * b.x) + (in.m11 * b.y) + (in.m12 * b.z) + (in.m13 * b.w));
 result.z = ((in.m20 * b.x) + (in.m21 * b.y) + (in.m22 * b.z) + (in.m23 * b.w));
 result.w = ((in.m30 * b.x) + (in.m31 * b.y) + (in.m32 * b.z) + (in.m33 * b.w));
 
 return result;
}

v4 mat4_postmultiply_vector(mat4 in, v4 b) {
  v4 result = {0};
  
  result.x = ((in.m00 * b.x) + (in.m10 * b.y) + (in.m20 * b.z) + (in.m30 * b.w));
  result.y = ((in.m01 * b.x) + (in.m11 * b.y) + (in.m21 * b.z) + (in.m31 * b.w));
  result.z = ((in.m02 * b.x) + (in.m12 * b.y) + (in.m22 * b.z) + (in.m32 * b.w));
  result.w = ((in.m03 * b.x) + (in.m13 * b.y) + (in.m23 * b.z) + (in.m33 * b.w));

  return result;
}

mat4 project_perspective(float aspect, float fov, float near, float far) {
 mat4 result = {0};
 
 float h = ((cosf(0.5) * fov) / (sinf(0.5) * fov));
 float w = h * aspect;
 
 result.m00 = w;
 result.m11 = h;
 result.m22 = (far + near) / (near - far);
 result.m23 = -( 2.0f * far * near) / (far - near);
 result.m32 = -1.0f;

 return result;
}

mat4 mat4_multiply_mat4(mat4 a, mat4 b) {
  mat4 result = {0};

  result.m[0] = mat4_premultiply_vector(b, a.m[0]);
  result.m[1] = mat4_premultiply_vector(b, a.m[1]);
  result.m[2] = mat4_premultiply_vector(b, a.m[2]);
  result.m[3] = mat4_premultiply_vector(b, a.m[3]);

  return result;
}

mat4 lookAt(v3 eye, v3 target, v3 up) {
  mat4 result = {0};

  v3 zaxis = normalize(subtraction(target, eye));
  v3 xaxis = normalize(cross(up, zaxis)); 
  v3 yaxis = cross(zaxis, xaxis);

  result.m[0] = (v4){xaxis.x, xaxis.y, xaxis.z, 0.0f};
  result.m[1] = (v4){yaxis.x, yaxis.y, yaxis.z, 0.0f};
  result.m[2] = (v4){zaxis.x, zaxis.y, zaxis.z, 0.0f};
  result.m[3] = (v4){-dot(xaxis, eye), -dot(yaxis, eye), -dot(zaxis, eye), 1.0f};

  return result;
}

mat4 MatrixFromPosRot(v3 eye, float pitch, float yaw) {
  mat4 result;

  float cosPitch = cosf(pitch);
  float sinPitch = sinf(pitch);
  float cosYaw = cosf(yaw);
  float sinYaw = sinf(yaw);

  v3 xaxis = {cosYaw, 0, -sinYaw};
  v3 yaxis = {sinYaw * sinPitch, cosPitch, cosYaw * sinPitch};
  v3 zaxis = {sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw};

  result.m[0] = (v4){xaxis.x, xaxis.y, xaxis.z, 0.0f};
  result.m[1] = (v4){yaxis.x, yaxis.y, yaxis.z, 0.0f};
  result.m[2] = (v4){zaxis.x, zaxis.y, zaxis.z, 0.0f};
  result.m[3] = (v4){-dot(xaxis, eye), -dot(yaxis, eye), -dot(zaxis, eye), 1.0f};

  pretty_print_mat4(result);

  return result;
}

v3 normalize(v3 in) {
  v3 result = {0};

  if (in.x + in.y + in.z == 0) {
    return result;
  }

  float ratio = 1 / sqrt((in.x * in.x) + (in.y * in.y) + (in.z * in.z));

  result.x = fabsf(in.x) * ratio;
  result.y = fabsf(in.y) * ratio;
  result.z = fabsf(in.z) * ratio;

  return result;
}

// Flipped internally for the sanity of the rest of us.
v3 subtraction(v3 a, v3 b) {
 v3 result = {0};
 
 result.x = (b.x - a.x);
 result.y = (b.y - a.y);
 result.x = (b.z - a.z);

 return result;
}

v3 cross(v3 a, v3 b) {
  v3 result = {0};

  result.x = (a.y*b.z - a.z*b.y);
  result.y = (a.z*b.x - b.z*a.x);
  result.z = (a.x*b.y - a.y*b.x);

  return result;
}

float dot(v3 a, v3 b) {
  float result = a.x*b.x + a.y*b.y + a.z*b.z;
  
  return result;
}

void pretty_print_mat4(mat4 in) {
  printf("\n\n[%f] [%f] [%f] [%f]\n", in.m00, in.m01, in.m02, in.m03);
  printf("[%f] [%f] [%f] [%f]\n", in.m10, in.m11, in.m12, in.m13);
  printf("[%f] [%f] [%f] [%f]\n", in.m20, in.m21, in.m22, in.m23);
  printf("[%f] [%f] [%f] [%f]\n\n", in.m30, in.m31, in.m32, in.m33);
}

bool GL_PrettyPrint(void) {
  GLenum error = glGetError();
  switch (error) {
    case GL_NO_ERROR:
      SDL_Log("GL_NO_ERROR");
    break;
    case GL_INVALID_ENUM:
      SDL_Log("GL_INVALID_ENUM");
    break;
    case GL_INVALID_VALUE:
      SDL_Log("GL_INVALID_VALUE");
    break;
    case GL_INVALID_OPERATION:
      SDL_Log("GL_INVALID_OPERATION");
    break;
    case GL_OUT_OF_MEMORY:
      SDL_Log("GL_OUT_OF_MEMORY");
    break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      SDL_Log("GL_INVALID_FRAMEBUFFER_OPERATION");
    break;
    default:
      SDL_Log("Unknown");
    break;
  }

}
