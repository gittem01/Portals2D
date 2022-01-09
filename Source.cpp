#include "helpers.h"

#define GL_SLIENCE_DEPRECATION

int main(void)
{
    WindowPainter* wh = new WindowPainter(nullptr);
    Camera* cam = new Camera(glm::vec2(0, 0), wh);
    cam->zoom = 0.6f;
    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, -9.81f));

    pWorld = new PortalWorld(world);

    ContactListener cl;
    world->SetContactListener(&cl);

    DestrucionListener dl;
    world->SetDestructionListener(&dl);

    DebugDrawer* drawer = new DebugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(0);
    
    mouseJointHandler mjh(world, wh, drawer);

    testCase1(pWorld);

    bool done = false;
    int frame = 0;
    int totalIter = 1;
    long sleepTime = 20; // millisecond

    const int vsyncFps = 60;
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

                pWorld->portalUpdate(); 
            }

            mjh.drawMouseBody();
            //world->DebugDraw();
            drawer->drawWorld(world);
            pWorld->drawUpdate();

            glfwSwapInterval(1);
            glfwSwapBuffers(wh->window);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
    }

    return 0;
}
