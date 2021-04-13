#include "WindowPainter.h"
#include <box2d/box2d.h>

WindowPainter::WindowPainter(Camera* cam) {
    this->cam = cam;
    this->massInit();
}

void WindowPainter::clearMouseData() {
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

bool WindowPainter::looper() {
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    if (width != lastWindowSizes.x || height != lastWindowSizes.y){
        glViewport(0, 0, width, height);
    }
    lastWindowSizes.x = width; lastWindowSizes.y = height;


    this->clearMouseData();
    lastMousePos[0] = mouseData[0];
    lastMousePos[1] = mouseData[1];

    bool done = false;    
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0) {
        if (event.type == SDL_QUIT) {
            done = 1;
        }
        else if (event.type == SDL_MOUSEMOTION){
            mouseData[0] = event.motion.x;
            mouseData[1] = event.motion.y;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN){
            mouseData[event.button.button + 1] = 2;
        }
        else if (event.type == SDL_MOUSEBUTTONUP){
            releaseQueue[event.button.button - 1] = true;
        }
        else if (event.type == SDL_MOUSEWHEEL){
            mouseData[5] = event.wheel.y;
        }
    }

    return done;
}


void WindowPainter::massInit() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("window", 0, 0, lastWindowSizes.x, lastWindowSizes.y,
     SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext ctx =  SDL_GL_CreateContext(window);
    
    glViewport(0, 0, lastWindowSizes.x, lastWindowSizes.y);
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}