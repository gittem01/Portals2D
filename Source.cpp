#include "helpers.h"

int main(void)
{
    WindowPainter* wh = new WindowPainter(NULL);
    Camera* cam = new Camera(glm::vec2(0, 0), wh->mouseData, wh->window);
    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, -15.0f));
    groundBody = world->CreateBody(&bodyDef);

    ContactListener cl;
    world->SetContactListener(&cl);

    debugDrawer* drawer = new debugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit);

    testCase2(world);

    bool done = false;
    int frame = 0;
    while (!done)
    {
        glClear( GL_COLOR_BUFFER_BIT );

        done = wh->looper();

        cam->update();

        glm::vec2 mp = cam->getMouseCoords();
        mouseHandler(world, mp, wh);

        keyHandler(wh);

        if (!isPaused || tick) world->Step(1.0f / 60.0f, 8, 3);

        for (Portal* p : Portal::portals) {
            p->creation();
        }
        for (Portal* p : Portal::portals) {
            p->destruction();
        }

        world->DebugDraw();
        for (Portal* p : Portal::portals) {
            p->draw();
        }

        printBodyCount(world);

        glfwSwapInterval(1);
        glfwSwapBuffers(wh->window);
    }

    return 0;
}