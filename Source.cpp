#include "helpers.h"


int main(void)
{
    WindowPainter* wh = new WindowPainter(NULL);

    Camera* cam = new Camera(glm::vec2(0, 0), wh->mouseData, wh->window);

    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, -10.0f));
    groundBody = world->CreateBody(&bodyDef);

    ContactListener cl;

    world->SetContactListener(&cl);

    debugDrawer* drawer = new debugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit);

    testCase1(world);

    glm::vec2* clicks[2] = { NULL, NULL };

    bool done = false;
    int frame = 0;
    while (!done)
    {
        glClear( GL_COLOR_BUFFER_BIT );

        done = wh->looper();

        cam->update();

        glm::vec2 mp = cam->getMouseCoords();

        if (wh->mouseData[3] == 2 && !wh->mouseData[2]) {
            clicks[0] = clicks[1];
            clicks[1] = new glm::vec2(mp.x, mp.y);
        }
        if (clicks[0]) {
            b2BodyDef bd;
            b2Body* ground = world->CreateBody(&bd);

            b2EdgeShape shape;
            shape.SetTwoSided(b2Vec2(clicks[0]->x, clicks[0]->y), b2Vec2(clicks[1]->x, clicks[1]->y));
            
            ground->CreateFixture(&shape, 0.0f);
            clicks[0] = NULL;
        }
        
        if (mouseJoint){
            bool jointFound = false;
            for (b2Joint* joint = world->GetJointList(); joint; joint = joint->GetNext()) {
                if (joint == mouseJoint) {
                    mouseJoint->SetTarget(b2Vec2(mp.x, mp.y));
                    jointFound = true;
                    break;
                }
            }
            if (!jointFound) mouseJoint = NULL;
        }

        if (wh->mouseData[2] == 2){
            mouseJointHandler(mp, world);
        }
        else if (wh->mouseData[2] == 0 && mouseJoint){
            for (b2Joint* joint = world->GetJointList(); joint; joint = joint->GetNext()){
                if (joint == mouseJoint) {
                    world->DestroyJoint(mouseJoint);
                    break;
                }
            }
            mouseJoint = NULL;
        }
        else if (wh->mouseData[2] == 1 && wh->mouseData[3] == 2){
            mouseJoint = NULL;
        }

        world->DebugDraw();
        for (Portal* p : Portal::portals) {
            p->draw();
        }

        world->Step(1.0f / 60.0f, 8, 3);
        
        for (Portal* p : Portal::portals) {
            p->creation();
        }
        for (Portal* p : Portal::portals) {
            p->destruction();
        }

        // num of bodies decreasing after some time. TODO.
        // not the biggest issue currently
        // Note: pulling a body out of a portal without removing
        // the mouse button increases num of bodies permanently

        int n = world->GetBodyCount();
        int b = 0;
        for (Portal* p: Portal::portals){
            b += p->correspondingBodies.size();
        }
        n -= b/2;
        //printf("Body count: %d, Frame: %d\n", n, ++frame);

        glfwSwapInterval(1);
        
        glfwSwapBuffers(wh->window);
    }

    return 0;
}