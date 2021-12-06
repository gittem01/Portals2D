#include "debugDrawer.h"
#include "mouseJointHandler.h"
#include "ContactListener.h"
#include "DestructionListener.h"
#include "Portal.h"

#include <chrono>
#include <thread>


bool isPaused = false;
bool tick = false;

b2Body* createObody(b2World* world, b2Vec2 bodyPos=b2Vec2(0, 0), float degree=270.0f, float thickness=0.3f, float rOut = 1.0f) {
    b2BodyDef bodyDef;
    bodyDef.position = bodyPos;
    bodyDef.type = b2_dynamicBody;
    bodyDef.angularDamping = 0.0f;

    b2Body* body = world->CreateBody(&bodyDef);

    b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2) * 4);

    degree /= 180 / glm::pi<float>();
    float n = 20;
    float incr = degree / n;

    for (float currD = 0.0f; currD < degree;) {
        b2FixtureDef fDef;
        b2PolygonShape shape;

        float nextD = currD + incr;

        float x1 = sin(currD) * rOut;
        float y1 = cos(currD) * rOut;

        float x2 = sin(currD) * (rOut - thickness);
        float y2 = cos(currD) * (rOut - thickness);

        float x3 = sin(nextD) * rOut;
        float y3 = cos(nextD) * rOut;

        float x4 = sin(nextD) * (rOut - thickness);
        float y4 = cos(nextD) * (rOut - thickness);

        vertices[0].x = x1; vertices[0].y = y1;
        vertices[1].x = x2; vertices[1].y = y2;
        vertices[2].x = x3; vertices[2].y = y3;
        vertices[3].x = x4; vertices[3].y = y4;

        shape.Set(vertices, 4);
        fDef.shape = &shape;
        fDef.density = 0.2f;

        body->CreateFixture(&fDef);

        currD = nextD;
    }

    free(vertices);

    return body;
}

b2Body* createWbody(b2World* world, b2Vec2 bodyPos=b2Vec2(0, 0), float degree=270.0f) {
    b2BodyDef bodyDef;
    bodyDef.position = bodyPos;
    bodyDef.type = b2_dynamicBody;
    bodyDef.angularDamping = 0.0f;

    b2Body* body = world->CreateBody(&bodyDef);

    b2FixtureDef fDef;
    b2CircleShape shape;
    
    degree /= 180 / glm::pi<float>();
    float n = 20;
    float incr = degree / n;
    float bigCircleR = 1.0f;
    float circleR = 0.15f;

    for (float currD = 0.0f; currD < degree;) {
        b2FixtureDef fDef;
        b2CircleShape cShape;

        float nextD = currD + incr;

        float x = sin(currD) * (bigCircleR - circleR);
        float y = cos(currD) * (bigCircleR - circleR);

        cShape.m_p = b2Vec2(x, y);
        cShape.m_radius = circleR;

        fDef.shape = &cShape;
        fDef.density = 0.2f;

        body->CreateFixture(&fDef);

        currD = nextD;
    }

    return body;
}

b2Body* createEdge(b2Vec2 p1, b2Vec2 p2, b2World* world, b2BodyType type) {
    b2BodyDef bd;
    bd.type = type;
    b2Body* edgeBody = world->CreateBody(&bd);

    b2EdgeShape shape;
    shape.SetTwoSided(p1, p2);

    b2FixtureDef fixDef;
    fixDef.shape = &shape;
    bodyData data = { OTHER, NULL };
    fixDef.userData.pointer = (uintptr_t)&data;

    edgeBody->CreateFixture(&fixDef);

    return edgeBody;
}

void keyHandler(WindowPainter* wh) {
    tick = false;
    if (wh->keyData[GLFW_KEY_P] == 2) {
        isPaused = !isPaused;
    }
    else if (wh->keyData[GLFW_KEY_O] == 2) {
        isPaused = true;
        tick = true;
    }
}

float getRand(){
    return ((float)rand()) / RAND_MAX - 0.5f;
}

void testCase1(b2World* world){
    float yPos = -4.0f;
    float portalSize = 3.0f;

    Portal* portal1 = new Portal(b2Vec2(-5.0f, yPos), b2Vec2(+1.0f, +0.0f), portalSize, world);
    Portal* portal2 = new Portal(b2Vec2(+5.0f, yPos), b2Vec2(-1.0f, +0.0f), portalSize, world);

    //Portal* portal5 = new Portal(b2Vec2(+9.0f, yPos - portalSize), b2Vec2(0.0f, 1.0f), portalSize, world);
    //Portal* portal6 = new Portal(b2Vec2(-9.0f, yPos - portalSize), b2Vec2(0.0f, 1.0f), portalSize, world);

    portal1->connect(portal2);

    //portal5->connect(portal6);

    createEdge(b2Vec2(-100.0f, yPos - portalSize), b2Vec2(+100.0f, yPos - portalSize), world, b2_staticBody);
    
    createObody(world, b2Vec2(0.0f, 0.0f));
    createWbody(world, b2Vec2(0.0f, -3.0f));
    PortalBody* b1 = new PortalBody(createObody(world, b2Vec2(0.0f, 3.0f)), world);
    PortalBody* b2 = new PortalBody(createWbody(world, b2Vec2(0.0f, 6.0f)), world);

    b1->bodyColor = b2Vec3(0.0f, 1.0f, 1.0f);
}