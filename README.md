# Graphical Sandbox

 This is a repo that will contain all my future graphical programming endeavours currently only host a primitive grass rendering pipeline.
 
# Grass Rendering
![til](./assets/git/BELH.gif)

# Future Additions planned

 Other large particle system like swarms of bats, fireflies, and maybe even fluid sims.

 Dynamic skyboxes, maybe even a realistic sun position.

 Lighting

 Clouds

# Building
  Building this project is as simple as running CMake which on linux is trival to set up and on windows a pain in the behind.
  But I digress. Below are the provided build scripts that should work on a good amount of systems.

 ```
   cmake -S . -B build
   cmake --build build
   ./graphical_sandbox
 ```
 

  Please note that this was not tested on any other system then a Linux Mint Distro so the likely hood of this working
  on a separate Operating system while greater then non-zero could also still be zero.
  
  Make sure the machine that you run this on supports Opengl 4.3 or higher and has a compatible C standard library.
  
  (Yes, I know. Quite the demanding requirements.)

  Furthermore while the CMakeList does include a Raylib fetch meaning you don't have to fully go through the hassle of installing that.

  (If you have CMake 3.11 or newer, Git, and an Internet connection)

  Raylib has a define that needs to be set in the config.h for custom-frame-control which to my knowledge can't be set from inside CMake.
  So you may get extranous errors from weird frame-rates to even bad input polling.

  Which means after you run the "cmake -S . -B build" step you should go into the config.h for raylib and set the custom-frame-control define.
  Or if you happen to have a precompiled package of Raylib that just so happened to be compiled for Opengl 4.3 or higher and custom-frame-control set
  the build is not only faster for it but it will get rid of the otherwise extraneous errors.


