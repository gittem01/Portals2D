#include <WindowHandler.h>
#include <Camera.h>

WindowHandler::WindowHandler(Camera* cam)
{
    this->cam = cam;
    this->massInit();
}

void WindowHandler::handleMouseData()
{
    this->mouseData[5] = 0;
    this->trackpadData[0] = 0;
    this->trackpadData[1] = 0;
    for (int i = 2; i < 5; i++)
    {
        if (this->mouseData[i] == 2)
        {
            this->mouseData[i] = 1;
        }
        if (releaseQueue[i - 2])
        {
            mouseData[i] = 0;
            releaseQueue[i - 2] = false;
        }
    }
    if (lastMousePos[0] == mouseData[0] && lastMousePos[1] == mouseData[1])
    {
        mouseData[6] = 0;
    }
    else
    {
        mouseData[6] = 1;
    }
}

void WindowHandler::handleKeyData()
{
    for (int i : newPressIndices)
    {
        keyData[i] = 1;
    }
    newPressIndices.clear();
}

bool WindowHandler::looper()
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    handleMouseData();
    handleKeyData();

    glfwPollEvents();

    bool done = glfwWindowShouldClose(window);

    return done;
}

void WindowHandler::massInit()
{
    if (!glfwInit())
    {
        printf("!glfwInit()\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(windowSizes.x, windowSizes.y, "2D Vulkan", NULL, NULL);
    if (!window)
    {
        printf("Could not create window\n");
        glfwTerminate();
        exit(1);
    }

    int x1, y1, x2, y2;
    glfwGetWindowSize(window, &x1, &y1);
    glfwGetFramebufferSize(window, &x2, &y2);

    dpiScaling = std::round(x2 / x1);

    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, WindowHandler::mouseEventCallback);
    glfwSetMouseButtonCallback(window, WindowHandler::buttonEventCallback);
    glfwSetScrollCallback(window, WindowHandler::scrollEventCallback);
    glfwSetWindowFocusCallback(window, WindowHandler::glfwWindowFocusCallback);
    glfwSetKeyCallback(window, WindowHandler::glfwKeyEventCallback);
    glfwSetWindowSizeCallback(window, WindowHandler::windowSizeEventCallback);

    mouseData[0] = windowSizes.x / 2.0f;
    mouseData[1] = windowSizes.y / 2.0f;
}

void WindowHandler::mouseEventCallback(GLFWwindow* window, double xpos, double ypos)
{
    WindowHandler* thisClass = (WindowHandler*)glfwGetWindowUserPointer(window);

    if (thisClass->cursorDisabled)
    {
        double x0, y0;
        x0 = xpos; y0 = ypos;
        if (xpos > thisClass->windowSizes.x) xpos = thisClass->windowSizes.x;
        else if (xpos < 0) xpos = 0;

        if (ypos > thisClass->windowSizes.y) ypos = thisClass->windowSizes.y;
        else if (ypos < 0) ypos = 0;

        if (x0 != xpos || y0 != ypos)
        {
            glfwSetCursorPos(window, xpos, ypos);
        }
    }

    thisClass->mouseData[0] = (int)xpos;
    thisClass->mouseData[1] = (int)ypos;
}

void WindowHandler::buttonEventCallback(GLFWwindow* window, int button, int action, int mods)
{
    WindowHandler* thisClass = (WindowHandler*)glfwGetWindowUserPointer(window);
    if (action)
    {
        thisClass->mouseData[button + 2] = 2;
    }
    else
    {
        thisClass->releaseQueue[button] = true;
    }
}

void WindowHandler::scrollEventCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    WindowHandler* thisClass = (WindowHandler*)glfwGetWindowUserPointer(window);
    thisClass->mouseData[5] = yoffset;

    thisClass->trackpadData[0] = xoffset;
    thisClass->trackpadData[1] = yoffset;
}

void WindowHandler::glfwKeyEventCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    WindowHandler* thisClass = (WindowHandler*)glfwGetWindowUserPointer(window);
    if (action == 1)
    {
        thisClass->keyData[key] = 2;
        thisClass->newPressIndices.push_back(key);
    }
    else if (!action)
    {
        thisClass->keyData[key] = 0;
    }
}

void WindowHandler::glfwWindowFocusCallback(GLFWwindow* window, int isFocused)
{
    WindowHandler* thisClass = (WindowHandler*)glfwGetWindowUserPointer(window);
    if (!isFocused)
    {
        thisClass->handleMouseData();
    }
}

void WindowHandler::windowSizeEventCallback(GLFWwindow* window, int width, int height)
{
    WindowHandler* thisClass = (WindowHandler*)glfwGetWindowUserPointer(window);

    thisClass->windowSizes.x = width;
    thisClass->windowSizes.y = height;

    if (thisClass->cam)
    {
        float ratio = (float)height / width;

        thisClass->cam->defaultYSides = glm::vec2(-thisClass->cam->baseX * ratio * 0.5f, thisClass->cam->baseX * ratio * 0.5f);

        thisClass->cam->xSides = thisClass->cam->defaultXSides / (thisClass->cam->zoom * thisClass->cam->zoom) + thisClass->cam->pos.x;
        thisClass->cam->ySides = thisClass->cam->defaultYSides / (thisClass->cam->zoom * thisClass->cam->zoom) + thisClass->cam->pos.y;
        thisClass->cam->updateOrtho();
    }
}