cmake_minimum_required(VERSION 3.10)
project(graphical_sandbox LANGUAGES C)

set(CMAKE_C_STANDARD 17)

find_package(raylib QUIET)

if (NOT raylib_FOUND)
  message(STATUS "Raylib not found. Fetching Raylib...")

  message(WARNING "A Raylib that is fetched can not have custom frame time set which may cause errors like input not being polled correctly.")
  message(WARNING "If you or a loved one is suffering from this. A preprepared raylib package will run better make sure to compile with opengl 430 or above.")
  message(WARNING "And to set the flag in the config.h header of raylib for custom frame control.")

  include(FetchContent)
  set(FETCHCONTENT_QUIET FALSE)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games
  set(OPENGL_VERSION "4.3" CACHE STRING "OpenGL version" FORCE) # The opengl version has to be 4.3 or greater due to the use of compute shaders.

  FetchContent_Declare(
      raylib
      GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
      GIT_TAG "master"
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
  )

  FetchContent_MakeAvailable(raylib)
else()
  message(STATUS "Found installed Raylib!")
endif()


# Adding our source files
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/include/") 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}")

# Declaring our executable
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

