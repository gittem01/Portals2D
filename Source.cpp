#include "debugDrawer.h"
#include "Shape.h"
#include "WindowPainter.h"
#include "ContactListener.h"
#include "Portal.h"

b2MouseJoint* mouseJoint = NULL;
float frequencyHz = 5.0f;
float dampingRatio = 0.7f;

void createEdge(b2Vec2 p1, b2Vec2 p2, b2World* world) {
    b2BodyDef bd;
    bd.type = b2_staticBody;
    b2Body* edgeBody = world->CreateBody(&bd);

    b2EdgeShape shape;
    shape.SetTwoSided(p1, p2);

    b2FixtureDef fixDef;
    fixDef.shape = &shape;

    edgeBody->CreateFixture(&fixDef);
}

void createCircle(b2Vec2 pos, float r, b2World* world) {
    b2BodyDef bd;
    bd.position = pos;
    bd.type = b2_dynamicBody;
    b2Body* circleBody = world->CreateBody(&bd);

    b2CircleShape circleShape;
    circleShape.m_radius = r;

    b2FixtureDef fixDef;
    fixDef.shape = &circleShape;
    fixDef.density = 1.0f;

    circleBody->CreateFixture(&fixDef);
}

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

    Camera* cam = new Camera(glm::vec2(0, 0), wh->mouseData, wh->window);

    wh->cam = cam;

    b2World* world = new b2World(b2Vec2(0.0f, -10.0f));

    ContactListener cl;

    world->SetContactListener(&cl);

    float xSize = 7.98f;
    float ySize = 4.48f;
    float width = 0.05f;
    float m = 0.9f;

    Portal* portal1 = new Portal(b2Vec2(0.0f, -ySize), b2Vec2(0.0f, 1.0f), ySize * m, world);
    Portal* portal2 = new Portal(b2Vec2(0.0f, ySize), b2Vec2(0.0f, -1.0f), ySize * m, world);
    Portal* portal3 = new Portal(b2Vec2(-xSize, 0.0f), b2Vec2(1.0f, 0.0f), ySize * m, world);
    Portal* portal4 = new Portal(b2Vec2(xSize, 0.0f), b2Vec2(-1.0f, 0.0f), ySize * m, world);

    portal1->connect(portal2);
    portal3->connect(portal4);

    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(+xSize, -ySize), world);
    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(-xSize, +ySize), world);
    createEdge(b2Vec2(-xSize, +ySize), b2Vec2(+xSize, +ySize), world);
    createEdge(b2Vec2(+xSize, +ySize), b2Vec2(+xSize, -ySize), world);

    debugDrawer* drawer = new debugDrawer();
    world->SetDebugDraw(drawer);
    drawer->SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit);

    for (int i = 0; i < 150; i++) {
        Shape* circle = new Shape(world, b2Vec2(getRand() * xSize * 2, getRand() * ySize * 2));
        circle->createCircle((getRand() + 1.5f) / 5.0f, b2_dynamicBody);
    }
    for (int i = 0; i < 150; i++) {
        Shape* poly = new Shape(world, b2Vec2(getRand() * xSize * 2, getRand() * ySize * 2));
        poly->createRect(b2Vec2(((getRand() + 1.5f) / 5.0f), (getRand() + 1.5f) / 5.0f), b2_dynamicBody);
    }

    glm::vec2* clicks[2] = { NULL, NULL };

    bool done = false;
    while (!done)
    {
        Portal::portals.size();
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

        world->Step(1.0f / 60.0f, 3, 8);

        for (Portal* p : Portal::portals) {
            p->update();
        }
        
        glfwSwapInterval(1);
        
        glfwSwapBuffers(wh->window);
    }

    return 0;
}