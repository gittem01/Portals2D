#include "helpers.h"

int main(void)
{
    WindowPainter* wh = new WindowPainter(nullptr);
    Camera* cam = new Camera(glm::vec2(0, 0), wh);
    cam->zoom = 0.6f;
    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, 0.0f));

    ContactListener cl;
    world->SetContactListener(&cl);

    DebugDrawer* drawer = new DebugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_aabbBit);
    
    pWorld = new PortalWorld(world, drawer);

    TestPlayer t(pWorld, wh);

    mouseJointHandler mjh(world, wh, drawer);

    testCase1(pWorld);

    bool done = false;
    int frame = 0;
    int totalIter = 1;
    long sleepTime = 20; // millisecond
    int vsyncFps = 60;

    while (!done)
    {
        frame++;

        done = wh->looper();

        if (t.pBody->size() > 1){
            b2Vec2 p1 = t.pBody->at(0)->body->GetPosition();
            b2Vec2 p2 = t.pBody->at(1)->body->GetPosition();

            glm::vec2 diff = glm::vec2(abs(p2.x - p1.x), abs(p2.y - p1.y)) / 2.0f + 5.0f;
            glm::vec2 mid = glm::vec2(p2.x + p1.x, p2.y + p1.y) / 2.0f;
            
            glm::vec2 posDiff = mid - cam->pos;

            cam->pos += posDiff / 50.0f;
            float reqZoom = sqrt(abs(cam->defaultXSides.x / diff.x));
            if (reqZoom < 0.65f){
                cam->zoom += (reqZoom - cam->zoom) / 10.0f;
            }
            else{
                cam->zoom += (0.65f - cam->zoom) / 50.0f;
            }
        }
        else{
            b2Vec2 bp = t.pBody->at(0)->body->GetPosition();
            glm::vec2 diff = glm::vec2(bp.x, bp.y) - wh->cam->pos;
            cam->pos += diff / 50.0f;
            cam->zoom += (0.65f - cam->zoom) / 50.0f;
        }

        cam->update();

        keyHandler(wh);

        if (!isPaused || tick) {
            glClear(GL_COLOR_BUFFER_BIT);

            for (int i = 0; i < totalIter; i++) {
                mjh.mouseHandler(frame, totalIter);

                world->Step(1.0f / (vsyncFps * totalIter), 8, 3);

                pWorld->portalUpdate();
                
                glfwPollEvents();
                t.update(1.0f / (vsyncFps * totalIter), totalIter);
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
