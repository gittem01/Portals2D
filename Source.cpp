#include "helpers.h"

int main(void)
{
    WindowPainter* wh = new WindowPainter(nullptr);
    Camera* cam = new Camera(glm::vec2(0, 0), wh);
    cam->zoom = 0.55f;
    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, 0.0f));

    ContactListener cl;
    world->SetContactListener(&cl);

    DebugDrawer* drawer = new DebugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_aabbBit);
    
    pWorld = new PortalWorld(world, drawer);

    mouseJointHandler mjh(world, wh, drawer);

    testCase3(pWorld);

    bool done = false;
    int frame = 0;
    int totalIter = 5;
    long sleepTime = 20; // millisecond
    int vsyncFps = 60;

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
