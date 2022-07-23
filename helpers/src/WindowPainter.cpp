#include "WindowPainter.h"
#include <box2d/box2d.h>
#include <Renderer.h>

WindowPainter::WindowPainter(Camera* cam) {
    this->cam = cam;
    this->massInit();
}

void WindowPainter::handleMouseData() {
    this->mouseData[5] = 0;
    this->trackpadData[0] = 0;
    this->trackpadData[1] = 0;
    for (int i = 2; i < 5; i++) {
        if (this->mouseData[i] == 2) {
            this->mouseData[i] = 1;
        }
        if (releaseQueue[i - 2]){
            mouseData[i] = 0;
            releaseQueue[i - 2] = false;
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // for macOS
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(windowSizes.x, windowSizes.y, "2D physics playground", 0, 0);
    if (!window)
    {
        printf("Window creation error\n");
        glfwTerminate();
    }

    int x1, y1, x2, y2;
    glfwGetWindowSize(window, &x1, &y1);
    glfwGetFramebufferSize(window, &x2, &y2);

    dpiScaling = x2 / x1;

    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, WindowPainter::mouseEventCallback);
    glfwSetMouseButtonCallback(window, WindowPainter::buttonEventCallback);
    glfwSetScrollCallback(window, WindowPainter::scrollEventCallback);
    glfwSetWindowFocusCallback(window, WindowPainter::glfwWindowFocusCallback);
    glfwSetKeyCallback(window, WindowPainter::glfwKeyEventCallback);
    glfwSetWindowSizeCallback(window, WindowPainter::windowSizeEventCallback);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("Failed to initialize GLAD\n");
        std::exit(-1);
    }

    glViewport(0, 0, (int)(windowSizes.x * dpiScaling), (int)(windowSizes.y * dpiScaling));
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glfwWindowHint(GLFW_SAMPLES, 4);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mouseData[0] = windowSizes.x / 2.0f;
    mouseData[1] = windowSizes.y / 2.0f;

    //disableCursor();
}

void WindowPainter::disableCursor(){
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    cursorDisabled = true;
}

void WindowPainter::enableCursor(){
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    cursorDisabled = false;
}

void WindowPainter::mouseEventCallback(GLFWwindow* window, double xpos, double ypos)
{
    WindowPainter* thisClass = (WindowPainter*)glfwGetWindowUserPointer(window);
    if (thisClass->cursorDisabled){
        double x0, y0;
        x0 = xpos; y0 = ypos;
        if (xpos > thisClass->windowSizes.x) xpos = thisClass->windowSizes.x;
        else if (xpos < 0) xpos = 0;

        if (ypos > thisClass->windowSizes.y) ypos = thisClass->windowSizes.y;
        else if (ypos < 0) ypos = 0;

        if (x0 != xpos || y0 != ypos){
            glfwSetCursorPos(window, xpos, ypos);
        }
    }

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
    thisClass->mouseData[5] = yoffset;

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

    if (key == GLFW_KEY_ESCAPE)
        thisClass->enableCursor();
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

    float ratio = (float)height / width;

    thisClass->cam->defaultYSides = glm::vec2(-thisClass->cam->baseX * ratio * 0.5f, thisClass->cam->baseX * ratio * 0.5f);
}