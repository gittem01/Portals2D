#include "debugDrawer.h"
#include "mouseJointHandler.h"
#include "ContactListener.h"
#include "DestructionListener.h"
#include "Portal.h"


bool isPaused = false;
bool tick = false;

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


void printBodyCount(b2World* world) {
    // num of bodies decreasing after some time. TODO.

    int n = world->GetBodyCount();
    int b = 0;
    for (Portal* p : Portal::portals) {
        b += p->correspondingBodies.size();
    }
    n -= b / 2;
    printf("Body count: %d\n", n);
}

float getRand(){
    return ((float)rand()) / RAND_MAX - 0.5f;
}

void testCase1(b2World* world){
    float xSize = 7.98f;
    float ySize = 4.48f;
    float width = 0.05f;
    float m = 1.0f;

    b2Vec2 center = b2Vec2(0.0f, 1.75f);
    int n = 5;
    float angle, radius = 1.5f;
    std::vector<b2Vec2> poly;
    std::vector<std::vector<b2Vec2>> edges;
    for (int i = 0; i < n+1; i++) {
        angle = (((i % n) * b2_pi * 2) / n);
        poly.push_back(center + b2Vec2(sin(angle) * radius, cos(angle) * radius));
    }
    edges.push_back(poly);

    edges.push_back({ b2Vec2(-xSize, +ySize * .5f), b2Vec2(-xSize + ySize * .5f, +ySize) });
    edges.push_back({ b2Vec2(+xSize, +ySize * .5f), b2Vec2(+xSize - ySize * .5f, +ySize) });
    edges.push_back({ b2Vec2(+xSize, -ySize * .5f), b2Vec2(+xSize - ySize * .5f, -ySize) });
    edges.push_back({ b2Vec2(-xSize*.5f, -ySize * .7f), b2Vec2(+xSize * .5f, -ySize * .7f) });

    for (int i = 0; i < edges.size(); i++) {
        for (int j = 0; j < edges.at(i).size() - 1; j++) {
            createEdge(edges.at(i).at(j), edges.at(i).at(j + 1), world, b2_staticBody);
        }
    }

    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(+xSize, -ySize), world, b2_staticBody);
    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(-xSize, +ySize), world, b2_staticBody);
    createEdge(b2Vec2(-xSize, +ySize), b2Vec2(+xSize, +ySize), world, b2_staticBody);
    createEdge(b2Vec2(+xSize, +ySize), b2Vec2(+xSize, -ySize), world, b2_staticBody);

    for (int i = 0; i < 5; i++) {
        Shape* circle = new Shape(world, b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f));
        circle->createCircle((getRand() + 1.0f) / 5.0f, b2_dynamicBody);
    }
    for (int i = 0; i < 15; i++) {
        Shape* poly = new Shape(world, b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f));
        poly->createRect(b2Vec2(((getRand() + 1.0f) / 5.0f), (getRand() + 1.0f) / 5.0f), b2_dynamicBody);
    }
}

void testCase2(b2World* world) {
    float xSize = 7.98f;
    float ySize = 4.48f;
    float width = 0.05f;
    float m = 1.0f;

    Portal* portal1 = new Portal(b2Vec2(0.0f, -ySize), b2Vec2(+0.0f, +1.0f), ySize * m, world);
    Portal* portal2 = new Portal(b2Vec2(0.0f, +ySize), b2Vec2(+0.0f, -1.0f), ySize * m, world);
    Portal* portal3 = new Portal(b2Vec2(-xSize, 0.0f), b2Vec2(+1.0f, +0.0f), ySize * m, world);
    Portal* portal4 = new Portal(b2Vec2(+xSize, 0.0f), b2Vec2(-1.0f, +0.0f), ySize * m, world);

    portal1->connect(portal2);
    portal3->connect(portal4);

    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(+xSize, -ySize), world, b2_staticBody);
    createEdge(b2Vec2(-xSize, -ySize), b2Vec2(-xSize, +ySize), world, b2_staticBody);
    createEdge(b2Vec2(-xSize, +ySize), b2Vec2(+xSize, +ySize), world, b2_staticBody);

    Shape* shape = new Shape(world, b2Vec2(xSize+width, 0.0f));
    shape->createRect(b2Vec2(width, ySize), b2_staticBody);

    float sizeM = 1.0f;
    float div = 2.25f;
    uint32_t numCircles = 25;
    uint32_t numPolygons = 75;
    for (int i = 0; i < numCircles; i++) {
        Shape* circle = new Shape(world, b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f));
        circle->createCircle((getRand() + sizeM) / div, b2_dynamicBody);
    }
    for (int i = 0; i < numPolygons; i++) {
        Shape* poly = new Shape(world, b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f));
        poly->createRect(b2Vec2(((getRand() + sizeM) / div), (getRand() + sizeM) / div), b2_dynamicBody);
    }

    for (int i = 0; i < 10; i++) {
        Shape* poly = new Shape(world, b2Vec2(getRand() * xSize * 1.9f, 10.0f));
        poly->createRect(b2Vec2(((getRand() + sizeM) / div), (getRand() + sizeM) / div), b2_dynamicBody);
        //poly->createCircle((getRand() + sizeM) / div, b2_dynamicBody);
    }
}