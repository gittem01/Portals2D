#include "debugDrawer.h"
#include "polygon.h"
#include "WindowPainter.h"
#include "ContactListener.h"
#include "Portal.h"

b2MouseJoint* mouseJoint = NULL;
float frequencyHz = 5.0f;
float dampingRatio = 0.7f;

void mouseJointHandler(glm::vec2 mp, b2World* world){
    b2Vec2 target = b2Vec2(mp.x, mp.y);

    b2Body* clickedBody = NULL;
    for (b2Body* body = world->GetBodyList(); body; body = body->GetNext()){
        if (body->GetType() != b2_dynamicBody) continue;
        for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext()){
            bool isIn = fixture->TestPoint(target);
            if (isIn){ 
                clickedBody = body; 
                goto endFor;
            }
        }
    }
    endFor:

    if (clickedBody){
        b2BodyDef bodyDef;
        b2Body* groundBody = world->CreateBody(&bodyDef);
        
        b2MouseJointDef jd;
        jd.bodyA = groundBody;
        jd.bodyB = clickedBody;
        jd.target = target;
        jd.maxForce = 1000.0f * clickedBody->GetMass();
        b2LinearStiffness(jd.stiffness, jd.damping, frequencyHz, dampingRatio, jd.bodyA, jd.bodyB);

        mouseJoint = (b2MouseJoint*)world->CreateJoint(&jd);
        clickedBody->SetAwake(true);
    }
}

float getRand(){
    return ((float)rand()) / RAND_MAX - 0.5f;
}

int main(void)
{
    WindowPainter* wh = new WindowPainter(NULL);

    Camera* cam = new Camera(glm::vec2(0, -5), wh->mouseData, wh->window);

    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, -10.0f));

    ContactListener cl;

    world->SetContactListener(&cl);
    world->SetContinuousPhysics(false);
    //world->SetAllowSleeping(false);

    float boxSize = 8.0f;
    float width = 0.05f;

    Portal* portal = new Portal(b2Vec2(0.0f, -boxSize), b2Vec2(0.0f, 1.0f), 6.0f, world);

    //Portal* portal2 = new Portal(b2Vec2(-boxSize + width * 2.0f * 0, 0.0f), b2Vec2(1.0f, 0.0f), 6.0f, world);
    //Portal* portal2 = new Portal(b2Vec2(0.0f, boxSize), b2Vec2(0.0f, -1.0f), 6.0f, world);
    Portal* portal2 = new Portal(b2Vec2(0.0f, boxSize), b2Vec2(-1.0f, -1.0f), 6.0f, world);
    portal->connect(portal2);

    /*(new polygon(world, b2Vec2(0.0f, boxSize + width)))->createBox(
        b2Vec2(boxSize + width*2, width), b2_staticBody, NULL);*/
    (new polygon(world, b2Vec2(-(boxSize + width), 0.0f)))->createBox(
        b2Vec2(width, boxSize), b2_staticBody, NULL);
    (new polygon(world, b2Vec2(0.0f, -(boxSize + width))))->createBox(
        b2Vec2(boxSize + width * 2, width), b2_staticBody, NULL);
    (new polygon(world, b2Vec2(boxSize + width, 0.0f)))->createBox(
        b2Vec2(width, boxSize), b2_staticBody, NULL);

    debugDrawer* drawer = new debugDrawer();

    polygon* poly = new polygon(world, b2Vec2(0.0f, -2.0f));
    poly->createBox(b2Vec2(0.6f, 0.5f), b2_dynamicBody, portal);
    //poly->body->SetAngularVelocity(9.0f);

    //polygon* thin = new polygon(world, b2Vec2(getRand() * boxSize, getRand() * boxSize));
    //thin->createBox(b2Vec2(0.01f, 3.0f), b2_dynamicBody, portal);

    for (int i = 0; i < 0; i++) {
        polygon* poly2 = new polygon(world, b2Vec2(getRand() * boxSize, getRand() * boxSize));
        poly2->createBox(b2Vec2((getRand() + 0.8f) / 2.0f, (getRand() + 0.8f) / 2.0f), b2_dynamicBody, portal);
    }

    world->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit);

    double timeStep = 0.0f;
    double t0;

    glm::vec2* clicks[2] = { NULL, NULL };

    bool done = false;
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
            for (b2Joint* joint = world->GetJointList(); joint; joint = joint->GetNext()) {
                if (joint == mouseJoint) mouseJoint->SetTarget(b2Vec2(mp.x, mp.y));
                break;
            }
        }

        if (wh->mouseData[2] == 2){
            mouseJointHandler(mp, world);
        }
        else if (wh->mouseData[2] == 0 && mouseJoint){
            for (b2Joint* joint = world->GetJointList(); joint; joint = joint->GetNext()){
                if (joint == mouseJoint) world->DestroyJoint(mouseJoint);
                break;
            }
            mouseJoint = NULL;

        }
        else if (wh->mouseData[2] == 1 && wh->mouseData[3] == 2){
            mouseJoint = NULL;
        }

        world->DebugDraw();
        portal->draw();
        portal2->draw();

        world->Step(1.0f / 60.0f, 10, 10);

        portal->update();
        portal2->update();

        glfwSwapInterval(1);

        glfwSwapBuffers(wh->window);
    }

    return 0;
}