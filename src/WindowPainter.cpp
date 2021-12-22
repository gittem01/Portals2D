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
        keyData[i] = 1;
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

    window = glfwCreateWindow(windowSizes.x, windowSizes.y, "2D physics playground", 0, 0);
    if (!window)
    {
        glfwTerminate();
        printf("Window creation error\n");
        std::exit(-1);
    }

    int x1, y1, x2, y2;
    glfwGetWindowSize(window, &x1, &y1);
    glfwGetFramebufferSize(window, &x2, &y2);

    dpiScaling = x2 / x1;

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(this->window, WindowPainter::mouseEventCallback);
    glfwSetMouseButtonCallback(this->window, WindowPainter::buttonEventCallback);
    glfwSetScrollCallback(this->window, WindowPainter::scrollEventCallback);
    glfwSetWindowFocusCallback(this->window, WindowPainter::glfwWindowFocusCallback);
    glfwSetKeyCallback(this->window, WindowPainter::glfwKeyEventCallback);
    glfwSetWindowSizeCallback(this->window, WindowPainter::windowSizeEventCallback);
    glfwSetWindowUserPointer(window, this);

    glViewport(0, 0, (int)(windowSizes.x * dpiScaling), (int)(windowSizes.y * dpiScaling));
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glLoadIdentity();
    glfwWindowHint(GLFW_SAMPLES, 8);
    
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
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
    thisClass->trackpadData[0] = xoffset;
    thisClass->trackpadData[1] = yoffset;
}

void WindowPainter::glfwKeyEventCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
    WindowPainter* thisClass = (WindowPainter*)glfwGetWindowUserPointer(window);
    if (action == 1) {
        thisClass->keyData[key] = 2;
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

    glViewport(0, 0, (int)(width * thisClass->dpiScaling), (int)(height * thisClass->dpiScaling));
    thisClass->windowSizes.x = width;
    thisClass->windowSizes.y = height;
}