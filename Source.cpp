#include "helpers.h"
#include <vector>


int main(void)
{
    WindowPainter* wh = new WindowPainter(nullptr);
    Camera* cam = new Camera(glm::vec2(0, 0), wh);
    cam->zoom = 0.6f;
    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, -9.81f));

    ContactListener cl;
    world->SetContactListener(&cl);

    DestrucionListener dl;
    world->SetDestructionListener(&dl);

    DebugDrawer* drawer = new DebugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(0);
    
    mouseJointHandler mjh(world, wh, drawer);

    testCase1(world);

    for (Portal* p : Portal::portals){
        p->drawer = drawer;
    }

    bool done = false;
    int frame = 0;
    int totalIter = 10;
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

                Portal::portalUpdate(); 
                for (int i = 0; i < PortalBody::portalBodies.size(); i++){
                    PortalBody* body = PortalBody::portalBodies.at(i);
                    body->postHandle();
                }
                PortalBody::globalPostHandle(world);
            }

            mjh.drawMouseBody();
            //world->DebugDraw();
            drawer->drawWorld(world);
            for (PortalBody* body : PortalBody::portalBodies){
                body->drawBodies();
            }
            for (Portal* p : Portal::portals) {
                p->draw();
            }

            if (wh->keyData[GLFW_KEY_S] == 2) PortalBody::drawReleases ^= 1;

            glfwSwapInterval(1);
            glfwSwapBuffers(wh->window);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
    }

    return 0;
}
