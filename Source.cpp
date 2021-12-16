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

    DebugDrawer* drawer = new DebugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_jointBit);
    
    mouseJointHandler mjh(world, wh, drawer);

    testCase1(world);

    for (Portal* p : Portal::portals){
        p->drawer = drawer;
    }

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

                Portal::portalUpdate(); 
            }

            mjh.drawMouseBody();
            world->DebugDraw();
            drawer->drawWorld(world);
            for (Portal* p : Portal::portals) {
                p->draw();
            }
            
            // Portal* p = *(++Portal::portals.begin());
            // printf("%d , %d__%d , %d__%d\n",  p->prepareFixtures.size(),
            //                             p->collidingFixtures[0].size(), p->collidingFixtures[1].size(),
            //                             p->releaseFixtures[0].size(), p->releaseFixtures[1].size());

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