# Graphical Sandbox

 This is a repo that will contain all my future graphical programming endeavours currently only host a primitive grass rendering pipeline.
 
# Grass Rendering
[Grass Demo Example](assets/git/BELH.gif)


# Future Additions planned

 Other large particle system like swarms of bats, fireflies, and maybe even fluid sims.

 Dynamic skyboxes, maybe even a realistic sun position.

 Lighting

 Clouds

# Building

 Currently the only platform that this application supports is Linux as I haven't quite gotten around to adding in 
 CMake and making sure the includes are not platform specific.

 The build script would look as follow with a caveat mentioned below.

 '''
  gcc --debug  -o Test.out src/main.c `pkg-config --cflags --libs raylib` -lm
 '''

 Depending on your windowing service you might need to include an other library for example on my system the build script looks like this.

 '''

  gcc --debug  -o Test.out src/main.c `pkg-config --cflags --libs raylib x11` -lm

  or 

  gcc --debug  -o Test.out src/main.c `pkg-config --cflags --libs raylib glew` -lm

 '''

 You might even see in the main.c file the build script I use just pasted in there as a comment for convenience. 
 The reason the build script is either X11 or glew is just because glew will includes X11 implicitly and I used it during a transitional period between sdl and raylib.


