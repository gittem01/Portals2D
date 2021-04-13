#include "debugDrawer.h"
#include "polygon.h"
#include "WindowPainter.h"
#include "ContactListener.h"

b2MouseJoint* mouseJoint = NULL;
float frequencyHz = 5.0f;
float dampingRatio = 0.7f;

b2Body* groundBody;

void mouseJointHandler(glm::vec2 mp, b2World* world){
    b2Vec2 target = b2Vec2(mp.x, mp.y);

    b2Body* clickedBody = NULL;
    for (b2Body* body = world->GetBodyList(); body; body = body->GetNext()){
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

    Camera* cam = new Camera(glm::vec2(0, 0), wh->mouseData, wh->window);

    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, 0.0f));
    b2BodyDef bodyDef;
    groundBody = world->CreateBody(&bodyDef);


    ContactListener cl;

    world->SetContactListener(&cl);
    world->SetAllowSleeping(false);

    (new polygon(world, b2Vec2(0.0f, 16.05f)))->createBox(b2Vec2(16, 0.05), b2_staticBody);
    (new polygon(world, b2Vec2(-16.05f, 0.0f)))->createBox(b2Vec2(0.05, 16), b2_staticBody);
    (new polygon(world, b2Vec2(0.0f, -16.05f)))->createBox(b2Vec2(16, 0.05), b2_staticBody);
    (new polygon(world, b2Vec2(16.05f, 0.0f)))->createBox(b2Vec2(0.05f, 16), b2_staticBody);

    for (int i=0; i<100; i++){
        polygon* poly = new polygon(world, b2Vec2(getRand() * 32, getRand() * 32));
        poly->createBox(b2Vec2((getRand() + 0.8f) / 2.0f, (getRand() + 0.8f) / 2.0f), b2_dynamicBody);
    }

    debugDrawer* drawer = new debugDrawer();

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

        if (wh->mouseData[4] == 2) {
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
            mouseJoint->SetTarget(b2Vec2(mp.x, mp.y));
        }

        if (wh->mouseData[2] == 2){
            mouseJointHandler(mp, world);
        }
        else if (wh->mouseData[2] == 0 && mouseJoint){
            world->DestroyJoint(mouseJoint);
            mouseJoint = NULL;
        }

        world->Step(1.0f/60.0f, 10, 10);

        world->DebugDraw();
            
        SDL_GL_SetSwapInterval(1);
        SDL_GL_SwapWindow(wh->window);
    }

    return 0;
}