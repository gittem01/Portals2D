# 2D Portals

## Features :
 - Reverse portal connection (rotation of objects gets reversed when going through reverse connections and appearence of object gets mirrored)
 - Multiple portal connection (a portal can be connected to more than one portal)
 - Global ray which can go through portals, including multi-connected portals
 
<img width="1320" alt="Screen Shot 2022-06-19 at 23 05 48" src="https://user-images.githubusercontent.com/29559574/174498555-47fa286f-2503-4338-9232-5c98d25991d7.png">

## Compile Requirements
 - CMake
 - C/C++ compiler such as clang or gcc for macOS and Linux
 - Visual Studio with C++ tools installed for Windows
## Compiling
 - ```mkdir build```
 - ```cd build```
 - ```cmake -DCMAKE_BUILD_TYPE=Release ..```
 - For Linux and OSX : ```make -j16```
 - For Windows : open newly created .sln file and compile the project. Or just open the folder from Visual Studio then configure and compile inside the Visual Studio

# Camera controls
 - CTRL + MOUSE_WHEEL changes size of the selection circle
 - Draging MIDDLE_MOUSE moves camera
 - Clicking RIGHT_MOUSE while there are selected bodies, pins selected objects
 - Clicking RIGHT_MOUSE 2 times creates an edge
# Keymap
 - Clicking 'f' when draging a body fixes the joint
 - Clicking 's' toggles on or off the release parts of bodies
 - Clicking 'o' pauses the physics world and makes a step every time 'o' is pressed
 - Clicking 'p' pauses and continues physics world
