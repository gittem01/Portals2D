#include "helpers.h"

int main(void)
{
    WindowPainter* wh = new WindowPainter(nullptr);
    Camera* cam = new Camera(glm::vec2(0, 0), wh);
    cam->zoom = 0.55f;
    wh->cam = cam;

    DebugDrawer* drawer = new DebugDrawer();
    ContactListener cl;
    pWorld = new PortalWorld(drawer);
    pWorld->SetContactListener(&cl);

    pWorld->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_centerOfMassBit);
    
    mouseJointHandler mjh(pWorld, wh, drawer);

    Renderer* renderer = new Renderer(pWorld);

    testCase1(pWorld, renderer);

    // manual random coloring for now
    for (Portal* p : pWorld->portals){
        b2Color c = b2Color(1, getRand() + 0.5f, getRand() + 0.5f, 1.0f);
        renderer->addPortal(p, c);
    }

    bool done = false;
    int frame = 0;
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

                pWorld->PortalStep(1.0f / (vsyncFps * totalIter), 8, 3);
            }

            //pWorld->DebugDraw();
            drawer->drawWorld(pWorld);
            renderer->render();

            mjh.drawMouseBody();

            glfwSwapInterval(1);
            glfwSwapBuffers(wh->window);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
    }

    return 0;
}
