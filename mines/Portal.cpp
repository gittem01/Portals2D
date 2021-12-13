#include "Portal.h"
#include <math.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <GLFW/glfw3.h>

std::set<Portal*> Portal::portals;

bool Portal::isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) >= t;
}

int Portal::getPointSide(b2Vec2 point){
    return isLeft(points[1], points[0], point, 0.0f);
}

float getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x));
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
    
    rcInp1.p1 = points[0];
    rcInp1.p2 = points[1];
    rcInp1.maxFraction = 1.0f;

    rcInp1.p1 = points[1];
    rcInp1.p2 = points[0];
    rcInp1.maxFraction = 1.0f;

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

    shape.SetTwoSided(points[0], points[0] - 0.0f*pVec);
    yFix[0] = body->CreateFixture(&shape, 0.0f);
    
    shape.SetTwoSided(points[1], points[1] + 0.0f*pVec);
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

int Portal::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (!bData || bData->type != PORTAL_BODY) return 0;

    PortalBody* pBody = (PortalBody*)bData->data;

    if (fix1 == midFixture){
        handleCollidingFixtures(contact, fix1, fix2, 0);
    }
}

int Portal::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (!bData || bData->type != PORTAL_BODY) return 0;

    PortalBody* pBody = (PortalBody*)bData->data;

    if (fix1 == midFixture){
        handleCollidingFixtures(contact, fix1, fix2, 1);
    }
}

int Portal::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    if (fix1 == midFixture){
        handleCollidingFixtures(contact, fix1, fix2, 2);
    }
    
    if (fix1 == midFixture && (collidingFixtures[0].find(fix2) != collidingFixtures[0].end() ||
        collidingFixtures[1].find(fix2) != collidingFixtures[1].end()))
    {
        contact->SetEnabled(false);
    }
}

bool Portal::rayCheck(b2Fixture* fix){
    b2RayCastOutput rcOutput;

    bool res1;
    bool res2;

    res1 = fix->RayCast(&rcOutput, rcInp1, b2_dynamicBody);
    res2 = fix->RayCast(&rcOutput, rcInp1, b2_dynamicBody);

    return res1 & res2;
}

// no operation : 0
// entered from side 0 : 1
// entered from side 1 : 2
// released from side 0 : 3
// released from side 1 : 4
int Portal::handleCollidingFixtures(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, int type){
    int ret = 0;

    b2WorldManifold manifold;
    contact->GetWorldManifold(&manifold);
    float angle = vecAngle(manifold.normal, dir);

    if (type != 1 && (angle < 0.01f || rayCheck(fix2) || (angle < (b2_pi + 0.01f) && angle > (b2_pi - 0.01f)))){
        if (angle < 0.01f){
            if (releaseFixtures[1].find(fix2) != releaseFixtures[1].end()){
                collidingFixtures[1].insert(fix2);
                releaseFixtures[1].erase(fix2);
                ret = 2;
            }
            else if (collidingFixtures[1].find(fix2) == collidingFixtures[1].end() && connections[0].size() > 0){
                collidingFixtures[0].insert(fix2);
                releaseFixtures[0].erase(fix2);
                ret = 1;
            }
        }
        else if (angle < (b2_pi + 0.01f) && angle > (b2_pi - 0.01f)){
            if (releaseFixtures[0].find(fix2) != releaseFixtures[0].end()){
                collidingFixtures[0].insert(fix2);
                releaseFixtures[0].erase(fix2);
                ret = 1;
            }
            else if (collidingFixtures[0].find(fix2) == collidingFixtures[0].end() && connections[1].size() > 0){
                collidingFixtures[1].insert(fix2);
                releaseFixtures[1].erase(fix2);
                ret = 2;
            }
        }
    }
    else{
        int side = getFixtureSide(fix2);
        if (side == 0){
            if (collidingFixtures[1].find(fix2) != collidingFixtures[1].end()){
                releaseFixtures[1].insert(fix2);
                ret = 4;
            }
        }
        else{
            if (collidingFixtures[0].find(fix2) != collidingFixtures[0].end()){
                releaseFixtures[0].insert(fix2);
                ret = 3;
            }
        }
        collidingFixtures[0].erase(fix2);
        collidingFixtures[1].erase(fix2);
    }

    return ret;
}

int Portal::getFixtureSide(b2Fixture* fix){
    int side = 1;

    if (fix->GetType() == b2Shape::Type::e_circle){
        b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
        b2Vec2 pos = fix->GetBody()->GetWorldPoint(shape->m_p);
        if (isLeft(points[0], points[1], pos, 0.0f)) side = 0;
    }
    else{
        b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();
        float totalDist = 0.0f;
        for (int i = 0; i < shape->m_count; i++){
            b2Vec2 pos = fix->GetBody()->GetWorldPoint(shape->m_vertices[i]);
            totalDist += getDist(points[0], points[1], pos);
        }
        if (totalDist >= 0.0f) side = 0;
    }

    return side;
}


void Portal::postHandle(){
    
}

void Portal::connectBodies(b2Body* body1, b2Body* body2) {
    b2PrismaticJointDef prismDef;
    prismDef.Initialize(body1, body2, b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, 0.0f));
    prismDef.collideConnected = true;
    prismDef.maxMotorForce = 0.0f;

    body1->GetWorld()->CreateJoint(&prismDef);

    b2Vec2 dirClone1 = connections[0].at(0)->portal2->dir;
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

void Portal::connect(Portal* portal2, int side1, int side2, int isReversed){
    portalConnection* c1 = (portalConnection*)malloc(sizeof(portalConnection));
    c1->portal1 = this;
    c1->portal2 = portal2;
    c1->side1 = side1;
    c1->side2 = side2;
    c1->isReversed = isReversed;

    this->connections[c1->side1].push_back(c1);

    portalConnection* c2 = (portalConnection*)malloc(sizeof(portalConnection));

    c2->portal1 = portal2;
    c2->portal2 = this;
    c2->side1 = side2;
    c2->side2 = side1;
    c2->isReversed = isReversed;

    portal2->connections[c2->side1].push_back(c2);

    this->color = b2Color(1.0f, (float)rand() / RAND_MAX, ((float)rand()) / RAND_MAX, 1.0f);
    portal2->color = b2Color(1.0f - this->color.r, this->color.g, 1.0f - this->color.b);
}