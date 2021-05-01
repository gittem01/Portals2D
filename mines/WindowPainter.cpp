#include "WindowPainter.h"
#include <box2d/box2d.h>

WindowPainter::WindowPainter(Camera* cam) {
    this->cam = cam;
    this->massInit();
}

void WindowPainter::handleMouseData() {
    this->mouseData[5] = 0;
    for (int i = 2; i < 5; i++) {
        if (this->mouseData[i] == 2) {
            this->mouseData[i] = 1;
        }
        if (releaseQueue[i-2]){
            mouseData[i] = 0;
            releaseQueue[i-2] = false;
        }
    }
    if (lastMousePos[0] == mouseData[0] && lastMousePos[1] == mouseData[1]) {
        mouseData[6] = 0;
    }
    else {
        mouseData[6] = 1;
    }
}

void WindowPainter::handleKeyData() {
    for (int i : newPressIndices) {
        keyData[i] = 2;
    }
    newPressIndices.clear();
}

bool WindowPainter::looper() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    handleMouseData();
    handleKeyData();
    
    glfwPollEvents();

    bool done = glfwWindowShouldClose(window);  

    return done;
}


void WindowPainter::massInit() {
    if (!glfwInit()) {
        printf("GLFW init error\n");
        std::exit(-1);
    }

    window = glfwCreateWindow(windowSizes.x, windowSizes.y, "Window", 0, 0);
    if (!window)
    {
        glfwTerminate();
        printf("Window creation error\n");
        std::exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(this->window, WindowPainter::mouseEventCallback);
    glfwSetMouseButtonCallback(this->window, WindowPainter::buttonEventCallback);
    glfwSetScrollCallback(this->window, WindowPainter::scrollEventCallback);
    glfwSetWindowFocusCallback(this->window, WindowPainter::glfwWindowFocusCallback);
    glfwSetKeyCallback(this->window, WindowPainter::glfwKeyEventCallback);
    glfwSetWindowSizeCallback(this->window, WindowPainter::windowSizeEventCallback);
    glfwSetWindowUserPointer(window, this);

    glViewport(0, 0, windowSizes.x, windowSizes.y);
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void WindowPainter::mouseEventCallback(GLFWwindow* window, double xpos, double ypos)
{
    WindowPainter* thisClass = (WindowPainter*)glfwGetWindowUserPointer(window);
    thisClass->mouseData[0] = (int)xpos;
    thisClass->mouseData[1] = (int)ypos;
}

void WindowPainter::buttonEventCallback(GLFWwindow* window, int button, int action, int mods) {
    WindowPainter* thisClass = (WindowPainter*)glfwGetWindowUserPointer(window);
    if (action){
        thisClass->mouseData[button + 2] = 2;
    }
    else {
        thisClass->releaseQueue[button] = true;
    }
}

void WindowPainter::scrollEventCallback(GLFWwindow* window, double xoffset, double yoffset) {
    WindowPainter* thisClass = (WindowPainter*)glfwGetWindowUserPointer(window);
    thisClass->mouseData[5] = (int)yoffset;
}

void WindowPainter::glfwKeyEventCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
    WindowPainter* thisClass = (WindowPainter*)glfwGetWindowUserPointer(window);
    if (action == 1) {
        thisClass->keyData[key] = action;
        thisClass->newPressIndices.insert(key);
    }
    else if (!action) {
        thisClass->keyData[key] = 0;
    }
}

void WindowPainter::glfwWindowFocusCallback(GLFWwindow* window, int isFocused) {
    WindowPainter* thisClass = (WindowPainter*)glfwGetWindowUserPointer(window);
    if (!isFocused) {
        thisClass->handleMouseData();
    }
}

void WindowPainter::windowSizeEventCallback(GLFWwindow* window, int width, int height) {
    WindowPainter* thisClass = (WindowPainter*)glfwGetWindowUserPointer(window);
    glViewport(0, 0, width, height);
    thisClass->windowSizes.x = width;
    thisClass->windowSizes.y = height;
}