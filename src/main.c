
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


#include "common.h"
#include "scenes.h"


typedef struct event {
 int32_t type;
 int32_t scancode;
 uint16_t modstate;
} event;

//int32_t connect_to_listener(int32_t *fd);
//int32_t check_and_get_event(int32_t fd);

float GetRandomFloat(float from, float to) {
    return from + (to-from)*(float)GetRandomValue(0, INT_MAX) / INT_MAX;
}

void main() {

  //int32_t file_descriptor = 0;
  //connect_to_listener(&file_descriptor);
  
  memoryArena arena;
  arena.size = Megabytes(2);
  arena.used = 0;
  arena.memory = malloc(arena.size);
  
  SceneList ListOfScenes = {0};
  ListOfScenes.size = 16;
  ListOfScenes.buf = pushSize(&arena, sizeof(SceneList) * ListOfScenes.size);
  ListOfScenes.count = 2;

  ListOfScenes.buf[0] = s_grassOnly;
  ListOfScenes.buf[1] = s_screenSaver;
  
  SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_TOPMOST);
  InitWindow(960, 540, "Window");
  SetTargetFPS(60);

  SetTraceLogLevel(LOG_ALL);

  int32_t c_monitor = GetCurrentMonitor();
  float dt = 0;
  double last = GetTime();
  double now = GetTime();
  int32_t refresh_rate = GetMonitorRefreshRate(c_monitor);

  common_context ctx = {0};
  ctx.dt = dt;
  ctx.list = &ListOfScenes;
  SetActiveScene(&ctx, &ListOfScenes.buf[0]);

  SetWindowSize(GetMonitorWidth(c_monitor), GetMonitorHeight(c_monitor));
  SetWindowPosition(0, 0);
  // WARNING: A bug, with either glfw or raylib, with the way window flags are set.
  // if the Mouse passthrough is done at window initialization it doesn't seem to work.
  // NOTE: The implementation difference being that window initialization does the flag setting
  // through glfwWindowHint and SetWindowState does it's flag setting through glfwSetWindowAttrib.
  SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_MOUSE_PASSTHROUGH);
  printf("Check Point Seven\n");

  while (!WindowShouldClose()) {
    ctx.dt = dt;

    if (ListOfScenes.activeScene) {
      if (ListOfScenes.activeScene->UpdateFunction) {
        ListOfScenes.activeScene->UpdateFunction(&ctx, ListOfScenes.activeScene->data);
      }
    }
    
    BeginDrawing();

    if (ListOfScenes.activeScene) {
      if (ListOfScenes.activeScene->RenderFunction) {
        ListOfScenes.activeScene->RenderFunction(&ctx, ListOfScenes.activeScene->data);
      }
    }

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


/*
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
*/
