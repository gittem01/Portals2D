# 2D Portals
<img width="988" alt="p1" src="https://user-images.githubusercontent.com/29559574/150926012-42992263-ba23-4932-bf00-b5e6fbea701b.png">

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
