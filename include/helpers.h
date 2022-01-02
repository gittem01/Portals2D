#include "DebugDrawer.h"
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
    bodyDef.angularDamping = 0.1f;
    bodyDef.linearDamping = 0.1f;

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
    bodyDef.angularDamping = 0.1f;
    bodyDef.linearDamping = 0.1f;

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

void testCase2(b2World* world){
    Portal* portal1 = new Portal(b2Vec2(-7.0f, 0.0f), b2Vec2(+1.0f, 0.0f), 7.0f, world);
    Portal* portal2 = new Portal(b2Vec2(+7.0f, 0.0f), b2Vec2(-1.0f, 0.0f), 7.0f, world);
    Portal* portal3 = new Portal(b2Vec2(0.0f, -7.0f), b2Vec2(0.0f, +1.0f), 7.0f, world);
    Portal* portal4 = new Portal(b2Vec2(0.0f, +7.0f), b2Vec2(0.0f, -1.0f), 7.0f, world);

    portal1->connect(portal3);
    portal2->connect(portal4);

    //createEdge(b2Vec2(-3.0f, 0.0f), b2Vec2(+3.0f, 0.0f), world, b2_staticBody);

    PortalBody* b2 = new PortalBody(createWbody(world, b2Vec2(0.0f, 3.0f)), world);
    b2->bodyColor = b2Color(1, 0, 1, 0.5f);

    b2PolygonShape shape;
    shape.SetAsBox(1.0f, 0.4f);

    b2FixtureDef fDef;
    fDef.shape = &shape;
    fDef.density = 1.0f;

    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position = b2Vec2(0, 3);

    b2Body* body = world->CreateBody(&def);
    body->CreateFixture(&fDef);

    (new PortalBody(body, world))->bodyColor = b2Color(0.0f, 1.0f, 1.0f, 0.5f);
}

void testCase1(b2World* world){
    float yPos = -4.0f;
    float portalSize = 3.0f;

    Portal* portal1 = new Portal(b2Vec2(-9.0f, yPos), b2Vec2(+1.0f, +0.0f), portalSize, world);
    Portal* portal2 = new Portal(b2Vec2(-3.0f, yPos), b2Vec2(-1.0f, +0.0f), portalSize, world);
    Portal* portal3 = new Portal(b2Vec2(+6.0f, yPos - portalSize + 0.1f), b2Vec2(0.0f, +1.0f), portalSize, world);
    Portal* portal4 = new Portal(b2Vec2(+10.0f - 0.3f, 0.0f), b2Vec2(-1.0f, 0.0f), portalSize, world);
    Portal* portal5 = new Portal(b2Vec2(+6.0f, -3.0f), b2Vec2(0.0f, -1.0f), portalSize, world);

    portal1->connect(portal2);
    portal2->connect(portal1, 0, 1, true);
    //portal3->connect(portal2);
    portal4->connect(portal4);
    portal5->connect(portal3);

    createEdge(b2Vec2(-100.0f, yPos - portalSize), b2Vec2(+100.0f, yPos - portalSize), world, b2_staticBody);
        
    PortalBody* b1 = new PortalBody(createObody(world, b2Vec2(0.0f, 3.0f)), world);
    PortalBody* b2 = new PortalBody(createWbody(world, b2Vec2(0.0f, 6.0f)), world);

    b1->bodyColor = b2Color(0.0f, 1.0f, 1.0f, 0.5f);
    b2->bodyColor = b2Color(1.0f, 0.0f, 1.0f, 0.5f);

    b2PolygonShape shape;
    shape.SetAsBox(1.0f, 0.4f);

    b2FixtureDef fDef;
    fDef.shape = &shape;
    fDef.density = 1.0f;

    b2BodyDef def;
    def.type = b2_dynamicBody;

    //b2Body* body = world->CreateBody(&def);
    //body->CreateFixture(&fDef);

    //(new PortalBody(body, world))->bodyColor = b2Color(0.0f, 1.0f, 1.0f, 0.5f);

    def.position = b2Vec2(0.0f, -2.0f);
    def.linearDamping = 0.0f;
    b2Body* body2 = world->CreateBody(&def);
    body2->CreateFixture(&fDef);

    (new PortalBody(body2, world))->bodyColor = b2Color(0.0f, 0.0f, 1.0f, 0.5f);

    def.position = b2Vec2(0.0f, 3.0f);
    //b2Body* body3 = world->CreateBody(&def);
    //body3->CreateFixture(&fDef);

    def.position = b2Vec2(10.0f, 0.0f);
    def.type = b2_staticBody;

    shape.SetAsBox(0.2f, 10.0f);

    //b2Body* body4 = world->CreateBody(&def);
    //body4->CreateFixture(&fDef);
}