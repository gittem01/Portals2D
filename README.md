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
 - Clicking LEFT_MOUSE when objects selected pins selected objects
 - Clicking MOUSE_RIGHT 2 times creates an edge
# Keymap
 - Clicking f when draging a body fixes the joint
 - Clicking s toggles on or off the release parts of bodies

## Required additions
 - Time machine (need to be able to look back in time to fix bugs)

