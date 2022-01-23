# 2D Portals

## Compile Requirements
 - cmake
 - C/C++ compiler such as clang or gcc for OSX and Linux
 - Visual Studio with C++ tools installed for Windows
## Compiling
 - ```mkdir build```
 - ```cd build```
 - ```cmake -DCMAKE_BUILD_TYPE=Release ..```
 - for Linux and OSX : ```make -j16```
 - for Windows : open newly created .sln file and compile the project

# Camera controls
 - CTRL + MOUSE_WHEEL changes size of the selection circle
 - Draging MIDDLE_MOUSE moves camera
 - Clicking RIGHT_MOUSE while there are selected bodies, pins selected objects
 - Clicking RIGHT_MOUSE 2 times creates an edge
# Keymap
 - Clicking f when draging a body fixes the joint
 - Clicking s toggles on or off the release parts of bodies
