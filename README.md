# 2D Portals

## Features :
 - Reverse portal connection (rotation of objects gets reversed when going through reverse connections and appearence of object gets mirrored)
 - Multiple portal connection (a portal can be connected to more than one portal)
 - Global ray which can go through portals, including multi-connected portals
 
<img width="1440" alt="Screenshot 2022-08-01 at 21 03 44" src="https://user-images.githubusercontent.com/29559574/182213026-71d62466-5203-465f-93ae-358832e951a2.png">

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
