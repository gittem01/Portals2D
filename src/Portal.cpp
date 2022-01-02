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

float Portal::vecAngle(b2Vec2 v1, b2Vec2 v2){
    return acos(b2Dot(v1, v2) / (v1.Length() * v2.Length()));
}

float Portal::getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x));
}

float Portal::calcAngle2(b2Vec2 vec) {
    float angle = atan2(vec.y, vec.x);
    if (angle < 0) angle += b2_pi * 2.0f;

    return angle;
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

    rcInp2.p1 = points[1];
    rcInp2.p2 = points[0];
    rcInp2.maxFraction = 1.0f;

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

    bd.type = b2_kinematicBody;
    body = world->CreateBody(&bd);

    b2EdgeShape shape;
    shape.SetTwoSided(points[0], points[1]);

    b2FixtureDef midPortal;
    midPortal.shape = &shape;
    midPortal.isSensor = false;

    midFixture = body->CreateFixture(&midPortal);

    b2Vec2 pVec = points[1] - points[0];
    normalize(&pVec);

    shape.SetTwoSided(points[0], points[0]);
    yFix[0] = body->CreateFixture(&shape, 0.0f);
    
    shape.SetTwoSided(points[1], points[1]);
    yFix[1] = body->CreateFixture(&shape, 0.0f);
}

int Portal::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (!bData || bData->type != PORTAL_BODY) return 0;
    
    int ret = -1;

    PortalBody* pBody = (PortalBody*)bData->data;

    if (fix1 == midFixture){
        ret = handleCollidingFixtures(contact, fix1, fix2);
    }

    return ret;
}

int Portal::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (!bData || bData->type != PORTAL_BODY) return 0;

    int ret = -1;

    PortalBody* pBody = (PortalBody*)bData->data;

    if (fix1 == midFixture){
        int side = getFixtureSide(fix2);
        if (collidingFixtures[1 ^ side].find(fix2) != collidingFixtures[1 ^ side].end()){
            releaseFixtures[1 ^ side].insert(fix2);
            ret = 3 - side;
        }
        else if (collidingFixtures[side].find(fix2) != collidingFixtures[side].end()){
            ret = 4;
        }
        collidingFixtures[0].erase(fix2);
        collidingFixtures[1].erase(fix2);
    }

    return ret;
}

int Portal::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    int ret = -1;

    if (fix1 == midFixture){
        ret = handleCollidingFixtures(contact, fix1, fix2);
    }

    if (fix1 == midFixture && (collidingFixtures[0].find(fix2) != collidingFixtures[0].end() ||
        collidingFixtures[1].find(fix2) != collidingFixtures[1].end()))
    {
        contact->SetEnabled(false);
    }

    return ret;
}

// sends 2 rays through the fixture from the edge points of portal
bool Portal::rayCheck(b2Fixture* fix){
    b2RayCastOutput rcOutput;

    bool res1;
    bool res2;

    res1 = fix->RayCast(&rcOutput, rcInp1, b2_dynamicBody);
    
    res2 = fix->RayCast(&rcOutput, rcInp2, b2_dynamicBody);

    return res1 && res2;
}

// collision started from side 0 : 0
// collision started from side 1 : 1
// released from side 0 : 2
// released from side 1 : 3
// released without being inserted into the releaseFixtures set: 4
int Portal::handleCollidingFixtures(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    int ret = -1;
    
    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);

    float angle = vecAngle(wManifold.normal, dir);
    int side = getFixtureSide(fix2);
    bool rayRes = rayCheck(fix2);
    if (angle < 0.01f || rayRes || (angle < (b2_pi + 0.01f) && angle > (b2_pi - 0.01f))){
        std::set<b2Fixture*>::iterator iter0 = collidingFixtures[0].find(fix2);
        std::set<b2Fixture*>::iterator iter1 = collidingFixtures[1].find(fix2);
        if (angle < 0.01f){
            if (releaseFixtures[1].find(fix2) != releaseFixtures[1].end())
            {
                if (iter1 == collidingFixtures[1].end()) ret = 1;
                collidingFixtures[1].insert(fix2);
                releaseFixtures[1].erase(fix2);
            }
            else if (iter1 == collidingFixtures[1].end() && connections[0].size() > 0)
            {
                if (iter0 == collidingFixtures[0].end()) ret = 0;
                collidingFixtures[0].insert(fix2);
                releaseFixtures[0].erase(fix2);
            }
        }
        else if (angle < (b2_pi + 0.01f) && angle > (b2_pi - 0.01f)){
            if (releaseFixtures[0].find(fix2) != releaseFixtures[0].end())
            {
                if (iter0 == collidingFixtures[0].end()) ret = 0;
                collidingFixtures[0].insert(fix2);
                releaseFixtures[0].erase(fix2);
            }
            else if (iter0 == collidingFixtures[0].end() && connections[1].size() > 0)
            {
                if (iter1 == collidingFixtures[1].end()) ret = 1;
                collidingFixtures[1].insert(fix2);
                releaseFixtures[1].erase(fix2);
            }
        }
    }
    else{
        if (collidingFixtures[1 ^ side].find(fix2) != collidingFixtures[1 ^ side].end()){
            releaseFixtures[1 ^ side].insert(fix2);
            ret = 3 - side;
        }
        else if (collidingFixtures[side].find(fix2) != collidingFixtures[side].end()){
            ret = 4;
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

bool Portal::shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, portalCollision* coll){
    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);

    if (collidingFixtures[1 ^ coll->side].find(fix2) != collidingFixtures[1 ^ coll->side].end()){
        return false;
    }

    float tHold = 0.0f;
    if (fix2->GetBody()->GetType() == b2_staticBody) tHold = -0.001f;

    std::vector<b2Vec2> collPoints = getCollisionPoints(fix1, fix2);

    if (collPoints.size() == 0 && fix2->GetBody()->GetType() == b2_staticBody){
        b2RayCastOutput rcOutput;
        b2RayCastInput rcInput;
        rcInput.maxFraction = 1.0f;
        for (int i = 0; i < contact->GetManifold()->pointCount; i++){
            bool res;
            rcInput.p1 = wManifold.points[i] - contact->GetManifold()->localNormal;
            rcInput.p2 = wManifold.points[i] + contact->GetManifold()->localNormal;
            res = fix2->RayCast(&rcOutput, rcInput, fix2->GetBody()->GetType());
            if (res) {
                b2Vec2 result = getRayPoint(rcInput, rcOutput);
                collPoints.push_back(result);
            }
        }
    }

    for (int i = 0; i < contact->GetManifold()->pointCount; i++){
        //drawer->DrawPoint(wManifold.points[i], 10.0f, b2Color(1, 0, 0, 1));
    }

    for (b2Vec2 p : collPoints){
        //drawer->DrawPoint(p, 10.0f, b2Color(0, 1, 1, 1));
    }

    for (b2Vec2 p : collPoints){
        if (!isLeft(points[1 ^ coll->side], points[coll->side], p, tHold)){
            return true;
        }
    }
    if (collPoints.size() > 0) return false;

    for (int i = 0; i < contact->GetManifold()->pointCount; i++){
        if (isLeft(points[1 ^ coll->side], points[coll->side], wManifold.points[i], tHold)){
            return false;
        }
    }
    
    return true;
}

void Portal::postHandle(){
    
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

    if (portal2 != this || side1 != side2){
        portalConnection* c2 = (portalConnection*)malloc(sizeof(portalConnection));

        c2->portal1 = portal2;
        c2->portal2 = this;
        c2->side1 = side2;
        c2->side2 = side1;
        c2->isReversed = isReversed;

        portal2->connections[c2->side1].push_back(c2);
    }

    this->color = b2Color(1.0f, (float)rand() / RAND_MAX, ((float)rand()) / RAND_MAX, 1.0f);
    portal2->color = b2Color(1.0f - this->color.r, this->color.g, 1.0f - this->color.b);
}