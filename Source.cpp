#include "helpers.h"

int main(void)
{
    WindowPainter* wh = new WindowPainter(NULL);
    Camera* cam = new Camera(glm::vec2(0, 0), wh);
    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, -30.0f));
    world->SetAllowSleeping(false); // required for mouse joint to work properly

    ContactListener cl;
    world->SetContactListener(&cl);

    DestrucionListener dl;
    world->SetDestructionListener(&dl);

    debugDrawer* drawer = new debugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit);
    
    mouseJointHandler mjh(world, wh);

    testCase2(world);

    bool done = false;
    int frame = 0;
    int totalIter = 10.0f;
    while (!done)
    {
        glClear( GL_COLOR_BUFFER_BIT );

        done = wh->looper();

        cam->update();

        keyHandler(wh);

        if (!isPaused || tick) {
            for (int i = 0; i < totalIter; i++) {
                mjh.mouseHandler();

                world->Step(1.0f / (60.0f * totalIter), 8, 3);

                for (Portal* p : Portal::portals) {
                    p->creation();
                }
                for (Portal* p : Portal::portals) {
                    p->destruction();
                }
            }

            world->DebugDraw();
            for (Portal* p : Portal::portals) {
                p->draw();
            }
            
            printBodyCount(world);
            
            glfwSwapInterval(1);
            glfwSwapBuffers(wh->window);
        }
    }

    return 0;
}