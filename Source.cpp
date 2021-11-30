#include "helpers.h"

int main(void)
{
    WindowPainter* wh = new WindowPainter(NULL);
    Camera* cam = new Camera(glm::vec2(0, 0), wh);
    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, -45.0f));

    ContactListener cl;
    world->SetContactListener(&cl);

    DestrucionListener dl;
    world->SetDestructionListener(&dl);

    debugDrawer* drawer = new debugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit);
    
    mouseJointHandler mjh(world, wh);

    testCase1(world);

    bool done = false;
    int frame = 0;
    int totalIter = 1;
    long sleepTime = 20; // millisecond

    const int vsyncFps = 120;
    while (!done)
    {
        frame++;

        done = wh->looper();

        cam->update();

        keyHandler(wh);

        if (!isPaused || tick) {
            glClear(GL_COLOR_BUFFER_BIT);

            for (int i = 0; i < totalIter; i++) {
                mjh.mouseHandler(frame, totalIter);

                world->Step(1.0f / (vsyncFps * totalIter), 8, 3);
            }

            mjh.drawMouseBody();
            world->DebugDraw();
            for (Portal* p : Portal::portals) {
                p->draw();
            }

            for (PortalBody* body : PortalBody::portalBodies){
                body->drawBodies();
            }
            
            glfwSwapInterval(1);
            glfwSwapBuffers(wh->window);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
    }

    return 0;
}