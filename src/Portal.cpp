#include "Portal.h"
#include <math.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <GLFW/glfw3.h>

Portal::Portal(b2Vec2 pos, b2Vec2 dir, float size, PortalWorld* pWorld){
    dir.Normalize();

    this->pWorld = pWorld;
    this->pos = pos;
    this->dir = dir;
    this->size = size;
    calculatePoints();
    createPortalBody();
    
    this->rcInp1.p1 = points[0];
    this->rcInp1.p2 = points[1];
    this->rcInp1.maxFraction = 1.0f;

    this->rcInp2.p1 = points[1];
    this->rcInp2.p2 = points[0];
    this->rcInp2.maxFraction = 1.0f;

    this->color = b2Color(0.0f, 0.3f, 1.0f, 1.0f);
}

Portal::~Portal() {
    
}

void Portal::calculatePoints(){
    this->angle = pWorld->calcAngle2(this->dir) + b2_pi / 2.0f;

    points[0].x = pos.x + cos(angle) * size;
    points[0].y = pos.y + sin(angle) * size;

    points[1].x = pos.x - cos(angle) * size;
    points[1].y = pos.y - sin(angle) * size;
}

void Portal::createPortalBody(){
    b2BodyDef bd;
    bodyData* data = (bodyData*)malloc(sizeof(bodyData));
    *data = { PORTAL, this };
    bd.userData.pointer = (uintptr_t)data;

    bd.type = b2_staticBody;
    body = pWorld->world->CreateBody(&bd);

    b2EdgeShape shape;
    shape.SetTwoSided(points[0], points[1]);

    b2FixtureDef midPortal;
    midPortal.shape = &shape;
    midPortal.isSensor = false;

    midFixture = body->CreateFixture(&midPortal);

    b2Vec2 pVec = points[1] - points[0];
    pWorld->normalize(&pVec);

    shape.SetTwoSided(points[0], points[0]);
    yFix[0] = body->CreateFixture(&shape, 0.0f);
    
    shape.SetTwoSided(points[1], points[1]);
    yFix[1] = body->CreateFixture(&shape, 0.0f);

    pVec = b2Vec2(pVec.x * 0.1f, pVec.y * 0.1f);

    float widthMul = 10.0f;
    float dirMult = 2.0f;
    b2Vec2 p1 = points[0] - b2Vec2(pVec.x * widthMul, pVec.y * widthMul);
    b2Vec2 p2 = points[1] + b2Vec2(pVec.x * widthMul, pVec.y * widthMul);
    b2Vec2 p3 = p1 + b2Vec2(dir.x * dirMult, dir.y * dirMult);
    b2Vec2 p4 = p2 + b2Vec2(dir.x * dirMult, dir.y * dirMult);
    p1 -= b2Vec2(dir.x * dirMult, dir.y * dirMult);
    p2 -= b2Vec2(dir.x * dirMult, dir.y * dirMult);
    
    b2Vec2 polyPoints[4] = { p1, p2, p3, p4 };

    b2PolygonShape polyShape;
    polyShape.Set(polyPoints, 4);
    collisionSensor = body->CreateFixture(&polyShape, 0.0f);
    collisionSensor->SetSensor(true);
}

int Portal::getPointSide(b2Vec2 point){
    return pWorld->isLeft(points[1], points[0], point, 0.0f);
}

int Portal::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (!bData || bData->type != PORTAL_BODY) return 0;
    
    int ret = -1;

    PortalBody* pBody = (PortalBody*)bData->data;

    if (fix1 == collisionSensor){
        prepareFixtures.insert(fix2);
        ret = 5;
    }
    else if (fix1 == midFixture){
        ret = handleCollidingFixtures(contact, fix1, fix2);
    }

    return ret;
}

int Portal::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (!bData || bData->type != PORTAL_BODY) return 0;

    int ret = -1;

    PortalBody* pBody = (PortalBody*)bData->data;

    if (fix1 == collisionSensor){
        prepareFixtures.erase(fix2);
        ret = 6;
    }
    else if (fix1 == midFixture){
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
    const float angleThold = 0.005f;
    int ret = -1;
    
    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);

    float angle = pWorld->vecAngle(wManifold.normal, dir);
    if (angle > b2_pi * 2) angle -= (int)(angle / (b2_pi * 2));
    int side = getFixtureSide(fix2);
    bool rayRes = rayCheck(fix2);
    if (rayRes ||   angle < angleThold || b2_pi * 2 < (angle + angleThold) ||
                    (angle < (b2_pi + angleThold) && angle > (b2_pi - angleThold)))
    {
        std::set<b2Fixture*>::iterator iter0 = collidingFixtures[0].find(fix2);
        std::set<b2Fixture*>::iterator iter1 = collidingFixtures[1].find(fix2);
        if (angle < angleThold || b2_pi * 2 < (angle + angleThold)){
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
        else if (angle < (b2_pi + angleThold) && angle > (b2_pi - angleThold)){
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
            collidingFixtures[1 ^ side].erase(fix2);
            ret = 3 - side;
        }
        else if (collidingFixtures[side].find(fix2) != collidingFixtures[side].end()){
            collidingFixtures[side].erase(fix2);
            ret = 4;
        }
        //collidingFixtures[0].erase(fix2);
        //collidingFixtures[1].erase(fix2);
    }

    return ret;
}

int Portal::getFixtureSide(b2Fixture* fix){
    int side = 1;

    if (fix->GetType() == b2Shape::Type::e_circle){
        b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
        b2Vec2 pos = fix->GetBody()->GetWorldPoint(shape->m_p);
        if (pWorld->isLeft(points[0], points[1], pos, 0.0f)) side = 0;
    }
    else{
        b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();
        float totalDist = 0.0f;
        for (int i = 0; i < shape->m_count; i++){
            b2Vec2 pos = fix->GetBody()->GetWorldPoint(shape->m_vertices[i]);
            totalDist += pWorld->getDist(points[0], points[1], pos);
        }
        if (totalDist >= 0.0f) side = 0;
    }

    return side;
}

std::vector<b2Vec2> Portal::getUsableRayPoints(b2Fixture* fix, int side){
    std::vector<b2Vec2> usablePoints;
    if (fix->GetType() == b2Shape::Type::e_polygon){
        b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();
        for (int i = 0; i < polyShape->m_count; i++){
            b2Vec2 point = fix->GetBody()->GetWorldPoint(polyShape->m_vertices[i]);
            float dist = pWorld->getDist(points[side], points[1 - side], point);
            if (dist > 0.2f) usablePoints.push_back(point);
        }
    }
    else{
        b2CircleShape* circleShape = (b2CircleShape*)fix->GetShape();
        b2Vec2 point = fix->GetBody()->GetWorldPoint(circleShape->m_p);
        float dist = pWorld->getDist(points[side], points[1 - side], point);
        if (dist -circleShape->m_radius > 0.2f) usablePoints.push_back(point);
    }

    return usablePoints;
}

b2Vec2* Portal::getMaxRayPoints(std::vector<b2Vec2> usableRayPoints, int side){
    b2Vec2* retPoints = (b2Vec2*)calloc(2, sizeof(b2Vec2));

    b2Vec2 copyDir = side ? -dir : dir;
    float dirAngle = pWorld->calcAngle2(copyDir);

    for (int i = 0; i < 2; i++){
        float maxAngle = -999999.0f;
        int j = 0;
        for (b2Vec2 p : usableRayPoints){
            b2Vec2 v = (p - points[i]);
            float angle = pWorld->calcAngle2(v);
            angle = (angle - dirAngle) * (i * 2 - 1);
            if (angle > maxAngle){
                retPoints[i] = p;
                maxAngle = angle;
            }
        }
    }

    return retPoints;
}

bool Portal::isPointIn(b2Vec2 p1, b2Vec2 p2, b2Vec2 point, int side){
    bool check1, check2;
    if (side){
        check1 = pWorld->isLeft(points[0], p1, point, -0.001f);
        check2 = pWorld->isLeft(p2, points[1], point, -0.001f);
    }else{
        check1 = pWorld->isLeft(p1, points[0], point, -0.001f);
        check2 = pWorld->isLeft(points[1], p2, point, -0.001f);
    }
    
    bool check3 = pWorld->isLeft(points[1 ^ side], points[side], point, -0.001f);

    return check1 && check2 && check3;
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

    if (fix1->GetType() == b2Shape::Type::e_circle){
        b2CircleShape* cShape = (b2CircleShape*)fix1->GetShape();
        b2Vec2 wCenter = fix1->GetBody()->GetWorldPoint(cShape->m_p);
        bool res;
        b2RayCastOutput rcOutput;
        b2RayCastInput rcInput;
        rcInput.maxFraction = 1.0f;
        rcInput.p1 = wCenter + cShape->m_radius * contact->GetManifold()->localNormal;
        rcInput.p2 = wCenter - cShape->m_radius * contact->GetManifold()->localNormal;
        res = fix2->RayCast(&rcOutput, rcInput, b2_staticBody);
        if (res) {
            b2Vec2 result = getRayPoint(rcInput, rcOutput);
            collPoints.clear();
            collPoints.push_back(result);
        }
    }

    if (collPoints.size() == 0 && fix2->GetBody()->GetType() == b2_staticBody){
        b2RayCastOutput rcOutput;
        b2RayCastInput rcInput;
        rcInput.maxFraction = 1.0f;
        for (int i = 0; i < contact->GetManifold()->pointCount; i++){
            bool res;
            rcInput.p1 = wManifold.points[i] + contact->GetManifold()->localNormal;
            rcInput.p2 = wManifold.points[i] - contact->GetManifold()->localNormal;
            res = fix2->RayCast(&rcOutput, rcInput, b2_staticBody);
            if (res) {
                b2Vec2 result = getRayPoint(rcInput, rcOutput);
                collPoints.push_back(result);
            }
        }
    }

    // rendering things
    {
        for (int i = 0; i < contact->GetManifold()->pointCount; i++){
            //pWorld->drawer->DrawPoint(wManifold.points[i], 10.0f, b2Color(1, 0, 0, 1));
        }

        for (b2Vec2 p : collPoints){
            //pWorld->drawer->DrawPoint(p, 10.0f, b2Color(0, 1, 1, 1));
        }
    }
    
    if (collPoints.size() > 0){
        if (fix2->GetBody()->GetType() == b2_staticBody && fix1->GetType() != b2Shape::Type::e_circle &&
            contact->GetManifold()->pointCount == 2 && collPoints.size() == 2)
        {
            std::vector<b2Vec2> points = getUsableRayPoints(fix1, coll->side);
            if (points.size() != 0){
                b2Vec2* maxRays = getMaxRayPoints(points, coll->side);
                
                bool in1 = isPointIn(maxRays[0], maxRays[1], collPoints.at(0), coll->side);
                bool in2 = isPointIn(maxRays[0], maxRays[1], collPoints.at(1), coll->side);
                if (in1 && !in2){
                    contact->GetManifold()->points[0] = contact->GetManifold()->points[1];
                    contact->GetManifold()->pointCount = 1;
                    return true;
                }
                else if (!in1 && in2){
                    contact->GetManifold()->pointCount = 1;
                    return true;
                }

                free(maxRays);
            }
        }
        for (b2Vec2 p : collPoints){
            if (!pWorld->isLeft(points[1 ^ coll->side], points[coll->side], p, tHold)){
                return true;
            }
        }
        return false;
    }
    
    for (int i = 0; i < contact->GetManifold()->pointCount; i++){
        if (pWorld->isLeft(points[1 ^ coll->side], points[coll->side], wManifold.points[i], tHold)){
            return false;
        }
    }
    
    return true;
}

bool Portal::prepareCollisionCheck(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* data = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (data && data->data == this) return false;

    int side = getFixtureSide(fix1);

    if (collidingFixtures[1 ^ side].find(fix1) != collidingFixtures[1 ^ side].end()){
        return false;
    }

    std::vector<b2Vec2> collPoints = getCollisionPoints(fix1, fix2);

    if (fix1->GetType() == b2Shape::Type::e_circle){
        b2CircleShape* cShape = (b2CircleShape*)fix1->GetShape();
        b2Vec2 wCenter = fix1->GetBody()->GetWorldPoint(cShape->m_p);
        bool res;
        b2RayCastOutput rcOutput;
        b2RayCastInput rcInput;
        rcInput.maxFraction = 1.0f;
        rcInput.p1 = wCenter + cShape->m_radius * contact->GetManifold()->localNormal;
        rcInput.p2 = wCenter - cShape->m_radius * contact->GetManifold()->localNormal;
        res = fix2->RayCast(&rcOutput, rcInput, b2_staticBody);
        if (res) {
            b2Vec2 result = getRayPoint(rcInput, rcOutput);
            collPoints.clear();
            collPoints.push_back(result);
        }
    }

    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);
    if (collPoints.size() == 0 && fix2->GetBody()->GetType() == b2_staticBody){
        b2RayCastOutput rcOutput;
        b2RayCastInput rcInput;
        rcInput.maxFraction = 1.0f;
        for (int i = 0; i < contact->GetManifold()->pointCount; i++){
            bool res;
            rcInput.p1 = wManifold.points[i] + contact->GetManifold()->localNormal;
            rcInput.p2 = wManifold.points[i] - contact->GetManifold()->localNormal;
            res = fix2->RayCast(&rcOutput, rcInput, b2_staticBody);
            if (res) {
                b2Vec2 result = getRayPoint(rcInput, rcOutput);
                collPoints.push_back(result);
            }
        }
    }

    std::vector<b2Vec2> points = getUsableRayPoints(fix1, side);
    if (points.size() != 0){
        b2Vec2* maxRays = getMaxRayPoints(points, side);
        for (int i = 0; i < collPoints.size(); i++){
            if (isPointIn(maxRays[0], maxRays[1], collPoints.at(i), side)){
                contact->SetEnabled(false);
                free(maxRays);
                return false;
            }
        }
        free(maxRays);
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

    // draw dir
    // glLineWidth(2.0f);
    // glColor4f(1, 1, 1, 1);
    // glBegin(GL_LINES);
    // glVertex2d(pos.x, pos.y);
    // glVertex2d(pos.x + dir.x * 4.0f, pos.y + dir.y * 4.0f);
    // glEnd();
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
    portal2->color = b2Color(1.0f - this->color.r, this->color.g, 1.0f - this->color.b, 1.0f);
}