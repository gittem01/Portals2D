#include "debugDrawer.h"
#include "Shape.h"
#include "WindowPainter.h"
#include "ContactListener.h"
#include "Portal.h"

b2MouseJoint* mouseJoint = NULL;
float frequencyHz = 5.0f;
float dampingRatio = 0.7f;

b2BodyDef bodyDef;
b2Body* groundBody;

b2Body* createEdge(b2Vec2 p1, b2Vec2 p2, b2World* world, b2BodyType type) {
    b2BodyDef bd;
    bd.type = type;
    b2Body* edgeBody = world->CreateBody(&bd);

    b2EdgeShape shape;
    shape.SetTwoSided(p1, p2);

    b2FixtureDef fixDef;
    fixDef.shape = &shape;

    edgeBody->CreateFixture(&fixDef);

    return edgeBody;
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

void testCase1(b2World* world){
    float xSize = 7.98f;
    float ySize = 4.48f;
    float width = 0.05f;
    float m = 1.0f;

    /*Portal* portal1 = new Portal(b2Vec2(0.0f, -ySize), b2Vec2(0.0f, 1.0f), ySize * m, world);
    Portal* portal2 = new Portal(b2Vec2(0.0f, ySize), b2Vec2(0.0f, -1.0f), ySize * m, world);
    Portal* portal3 = new Portal(b2Vec2(-xSize, 0.0f), b2Vec2(1.0f, 0.0f), ySize * m, world);
    Portal* portal4 = new Portal(b2Vec2(xSize, 0.0f), b2Vec2(-1.0f, 0.0f), ySize * m, world);

    portal1->connect(portal2);
    portal3->connect(portal4);*/

    b2Vec2 center = b2Vec2(0.0f, 2.0f);
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

    for (int i = 0; i < 1; i++) {
        Shape* circle = new Shape(world, b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f));
        circle->createCircle((getRand() + 2.0f) / 5.0f, b2_dynamicBody);
    }
    for (int i = 0; i < 1; i++) {
        Shape* poly = new Shape(world, b2Vec2(getRand() * xSize * 1.9f, getRand() * ySize * 1.9f));
        poly->createRect(b2Vec2(((getRand() + 2.0f) / 5.0f), (getRand() + 2.0f) / 5.0f), b2_dynamicBody);
    }
}
