#include "Portal.h"
#include <math.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <GLFW/glfw3.h>

std::set<Portal*> Portal::portals;

bool isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) >= t;
}

float calcAngle2(b2Vec2 vec) {
    float angle = atan2(vec.y, vec.x);
    if (angle < 0) angle += b2_pi * 2.0f;

    return angle;
}


float vecAngle(b2Vec2 v1, b2Vec2 v2){
    return acos(b2Dot(v1, v2));
}

void rotateVec(b2Vec2* vec, float angle){
    float x = cos(angle) * vec->x - sin(angle) * vec->y;
    float y = sin(angle) * vec->x + cos(angle) * vec->y;
    vec->x = x;
    vec->y = y;
}

void normalize(b2Vec2* vec){
    float length = sqrt(vec->x * vec->x + vec->y * vec->y);
    if (length == 0) { return; }
    
    vec->x /= length;
    vec->y /= length;
}


Portal::Portal(b2Vec2 pos, b2Vec2 dir, float size, b2World* world){
    this->world = world;
    this->pos = pos;
    this->dir = dir;
    normalize(&this->dir);
    this->size = size;
    calculatePoints();
    createPortalBody(world);
    
    this->color = b2Color(0.0f, 0.3f, 1.0f, 1.0f);

    portals.insert(this);
}

Portal::~Portal() {
    
}

void Portal::calculatePoints(){
    this->angle = calcAngle2(this->dir) + b2_pi / 2.0f;

    points[0].x = pos.x + cos(angle) * size;
    points[0].y = pos.y + sin(angle) * size;

    points[1].x = pos.x - cos(angle) * size;
    points[1].y = pos.y - sin(angle) * size;
}

void Portal::createPortalBody(b2World* world){
    b2BodyDef bd;
    bodyData* data = (bodyData*)malloc(sizeof(bodyData));
    *data = { PORTAL, this };
    bd.userData.pointer = (uintptr_t)data;

    bd.type = b2_staticBody;
    body = world->CreateBody(&bd);

    b2EdgeShape shape;
    shape.SetTwoSided(points[0], points[1]);

    b2FixtureDef midPortal;
    midPortal.shape = &shape;
    midPortal.isSensor = false;

    midFixture = body->CreateFixture(&midPortal);

    b2Vec2 pVec = points[1] - points[0];
    normalize(&pVec);
    pVec = b2Vec2(pVec.x * 0.1f, pVec.y * 0.1f);

    shape.SetTwoSided(points[0], points[0]);
    yFix[0] = body->CreateFixture(&shape, 0.0f);
    
    shape.SetTwoSided(points[1], points[1]);
    yFix[1] = body->CreateFixture(&shape, 0.0f);

    float widthMul = 10.0f;
    float dirMult = 2.0f;
    b2Vec2 p1 = points[0] - b2Vec2(pVec.x * widthMul, pVec.y * widthMul);
    b2Vec2 p2 = points[1] + b2Vec2(pVec.x * widthMul, pVec.y * widthMul);

    b2Vec2 p3 = p1 + b2Vec2(dir.x * dirMult, dir.y * dirMult);
    b2Vec2 p4 = p2 + b2Vec2(dir.x * dirMult, dir.y * dirMult);

    p1 += b2Vec2(dir.x * dirMult * -1, dir.y * dirMult);
    p2 += b2Vec2(dir.x * dirMult * -1, dir.y * dirMult);
    
    b2Vec2 polyPoints1[4] = { p1, p2, p3, p4 };

    b2PolygonShape polyShape;
    polyShape.Set(polyPoints1, 4);
    collisionSensor = body->CreateFixture(&polyShape, 0.0f);
    collisionSensor->SetSensor(true);
}

void Portal::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (!bData || bData->type != PORTAL_BODY) return;

    PortalBody* pBody = (PortalBody*)bData->data;

    for (int i = 0; i < 2; i++){
        if  (fix1 == collisionSensor){
            prepareBegins[i].push_back(fix2);
            return;
        }

        if (fix1 == midFixture){
            collideBegins.push_back(fix2);
        }
    }
}

void Portal::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (!bData || bData->type != PORTAL_BODY) return;

    PortalBody* pBody = (PortalBody*)bData->data;

    for (int i = 0; i < 2; i++){
        if (fix1 == collisionSensor){
            prepareEnds[i].push_back(fix2);
            return;
        }

        if (fix1 == midFixture){
            collideEnds.push_back(fix2);
        }
    }    
}

void Portal::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    if (fix1 == midFixture){
        contact->SetEnabled(false);
    }
}

void Portal::postHandle(){
    
}

void Portal::connectBodies(b2Body* body1, b2Body* body2) {
    b2PrismaticJointDef prismDef;
    prismDef.Initialize(body1, body2, b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, 0.0f));
    prismDef.collideConnected = true;
    prismDef.maxMotorForce = 0.0f;

    body1->GetWorld()->CreateJoint(&prismDef);

    b2Vec2 dirClone1 = connections.at(0)->portal->dir;
    b2Vec2 dirClone2 = this->dir;

    float mult = 100000.0f;

    b2PulleyJointDef pulleyDef;

    b2Vec2 anchor1 = body1->GetPosition();
    b2Vec2 anchor2 = body2->GetPosition();
    b2Vec2 groundAnchor1(dirClone1.x * mult, dirClone1.y * mult);
    b2Vec2 groundAnchor2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1, body2, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    world->CreateJoint(&pulleyDef);

    rotateVec(&dirClone1, b2_pi / 2.0f);
    rotateVec(&dirClone2, b2_pi / 2.0f);

    anchor1 = body1->GetPosition();
    anchor2 = body2->GetPosition();
    groundAnchor1 = b2Vec2(dirClone1.x * mult, dirClone1.y * mult);
    groundAnchor2 = b2Vec2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1, body2, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    world->CreateJoint(&pulleyDef);
}


void Portal::draw(){
    glLineWidth(2.0f);
	glColor4f(color.r, color.g, color.b, color.a);
	glBegin(GL_LINES);
	glVertex2d(points[0].x, points[0].y);
	glVertex2d(points[1].x, points[1].y);
	glEnd();
}

void Portal::connect(Portal* portal2){

    portalConnection* c = (portalConnection*)malloc(sizeof(portalConnection));
    c->portal = portal2;
    c->side1 = 1;
    c->side2 = 1;

    connections.push_back(c);
    this->color = b2Color(1.0f, (float)rand() / RAND_MAX, ((float)rand()) / RAND_MAX, 1.0f);
}