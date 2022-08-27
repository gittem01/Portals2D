#include "Portal.h"
#include <math.h>
#include <stdio.h>
#include <chrono>
#include <thread>

Portal::Portal(b2Vec2 pos, b2Vec2 dir, float size, PortalWorld* pWorld){
    dir.Normalize();

    this->pWorld = pWorld;
    this->pos = pos;
    this->dir = dir;
    this->size = size;
    this->isVoid[0] = false; this->isVoid[1] = false;
    calculatePoints();
    createPortalBody();
    
    float inMargin = 0.01f / b2Distance(points[0], points[1]);

    this->rcInp1.p1 = points[0];
    this->rcInp1.p2 = points[1];
    this->rcInp1.maxFraction = 1.0f - inMargin;

    this->rcInp2.p1 = points[1];
    this->rcInp2.p2 = points[0];
    this->rcInp2.maxFraction = 1.0f - inMargin;

    this->queryClass = new PortalWorldQuery(this);
}

Portal::~Portal(){
    delete queryClass;

    for (int i = 0; i < 2; i++){
        if (isVoid[i]){
            free(connections[i][0]);
            continue;
        }
        for (int j = 0; j < connections[i].size(); j++){
            Portal* otherPortal = connections[i][j]->portal2;
            if (otherPortal == this){
                free(connections[i][j]);
                break;    
            }
            int connectionSide2 = connections[i][j]->side2;
            for (int k = 0; k < otherPortal->connections[connectionSide2].size(); k++){
                portalConnection* pc = otherPortal->connections[connectionSide2].at(k);
                if (pc->portal2 == this){
                    free(pc);
                    otherPortal->connections[connectionSide2].erase(otherPortal->connections[connectionSide2].begin() + k);
                }
            }

            free(connections[i][j]);
        }
    }

    for (int i = 0; i < 2; i++){
        for (b2Fixture* fix : collidingFixtures[i]){
            PortalBody* pb = (PortalBody*)((bodyData*)fix->GetBody()->GetUserData().pointer)->data;
            fixtureData* fixData = (fixtureData*)fix->GetUserData().pointer;
            for (int i = 0; i < fixData->pCollisions.size(); i++){
                portalCollision* pc = fixData->pCollisions.at(i);
                if (pc->portal == this){
                    free(pc);
                    fixData->pCollisions.erase(fixData->pCollisions.begin() + i);
                    i--;
                }
            }
        }
    }

    free((void*)body->GetUserData().pointer);
    pWorld->DestroyBody(body);
}

void Portal::evaluateWorld(){
    boxMin = b2Vec2(fmin(points[0].x, points[1].x) - 0.01f, fmin(points[0].y, points[1].y) - 0.01f);
    boxMax = b2Vec2(fmax(points[0].x, points[1].x) + 0.01f, fmax(points[0].y, points[1].y) + 0.01f);
    
    b2AABB queryAABB = { boxMin, boxMax };
    pWorld->QueryAABB(queryClass, queryAABB);
}

void Portal::calculatePoints(){
    float angle = pWorld->calcAngle2(this->dir) + b2_pi / 2.0f;

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
    body = pWorld->CreateBody(&bd);

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
}

int Portal::getPointSide(b2Vec2 point){
    return pWorld->isLeft(points[1], points[0], point, 0.0f);
}

PortalCollisionType Portal::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){    
    PortalCollisionType ret = DEFAULT_COLLISION;
    
    if (fix1 == midFixture){
        ret = handleCollidingFixtures(contact, fix2);
    }

    return ret;
}

PortalCollisionType Portal::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (!bData || bData->type != PORTAL_BODY) return DEFAULT_COLLISION;

    PortalCollisionType ret = DEFAULT_COLLISION;

    if (fix1 == midFixture){
        int side = getFixtureSide(fix2);
        fixtureData* fixData = (fixtureData*)fix2->GetUserData().pointer;
        portalCollision* pColl = fixData->getPortalCollision(this);

        if (!pColl) return ret;

        if (pColl->side == (side ^ 1) && pColl->status == 1){
            pColl->status = -1;
            
            collidingFixtures[pColl->side ^ 1].erase(fix2);
            
            ret = (PortalCollisionType)((int)COLLIDING_RELEASE_0 << (1 ^ side));
            return ret;
        }
        else if (pColl->side == side && pColl->status == 1){
            for (int i = 0; i < fixData->pCollisions.size(); i++){
                if (fixData->pCollisions.at(i) == pColl){
                    fixData->pCollisions.erase(fixData->pCollisions.begin() + i);
                    break;
                }
            }

            ret = (PortalCollisionType)((int)COLLIDING_NONE_0 << side);
            return ret;
        }
        
        if (pColl->status < 0 && pColl->status % 2 == 0){
            if (side == 1){
                if (pColl->side == 0){
                    pColl->status--;
                    ret = E_COLLIDING_RELEASE_0;
                }
                else if (pColl->side == 1){
                    pColl->status++;
                }
            }
            else if (side == 0){
                if (pColl->side == 0){
                    pColl->status++;                
                }
                else if (pColl->side == 1){
                    pColl->status--;
                    ret = E_COLLIDING_RELEASE_1;
                }
            }
        }
    }

    return ret;
}

PortalCollisionType Portal::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    PortalCollisionType ret = DEFAULT_COLLISION;
    
    if (fix1 == midFixture){
        fixtureData* fixData = (fixtureData*)fix2->GetUserData().pointer;
        ret = handleCollidingFixtures(contact, fix2);
        portalCollision* pColl = fixData->getPortalCollision(this);
        if (pColl){
            contact->SetEnabled(false);
        }
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

    return res1 || res2;
}

PortalCollisionType Portal::handleCollidingFixtures(b2Contact* contact, b2Fixture* fix2){
    const float angleThold = 0.0001f;
    PortalCollisionType ret = DEFAULT_COLLISION;

    fixtureData* fixData = (fixtureData*)fix2->GetUserData().pointer;
    portalCollision* pColl = fixData->getPortalCollision(this);

    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);

    float angle = pWorld->vecAngle(wManifold.normal, dir);

    int side = getFixtureSide(fix2);
    bool rayRes = rayCheck(fix2);
    bool cond1 = angle < angleThold || b2_pi * 2 < (angle + angleThold); // dir
    bool cond2 = angle < (b2_pi + angleThold) && angle > (b2_pi - angleThold); // -dir
        
    if (rayRes || cond1 || cond2)
    {
        if (cond1){
            if (pColl && pColl->side == 1 && pColl->status < 0)
            {
                if (pColl->status % 2 != 0 && pColl->status < -1){
                    pColl->status++;
                }
                else if (pColl->status % 2 != 0){
                    if (pColl->status != 1) ret = RELEASE_COLLIDING_1;
                    pColl->status = 1;
                }
            }
            else if ((!pColl || pColl->status != 1) && connections[0].size() > 0)
            {
                if (!pColl){
                    pColl = (portalCollision*)malloc(sizeof(portalCollision));
                    pColl->portal = this;
                    pColl->side = 0;
                    pColl->status = 1;
                    fixData->pCollisions.push_back(pColl);

                    ret = NONE_COLLIDING_0;
                }
                else if (pColl->side == 0 && pColl->status < 0 && pColl->status % 2 == -1){
                    pColl->status--;
                }
            }
        }
        else if (cond2){
            if (pColl && pColl->status < 0 && pColl->side == 0)
            {
                if (pColl->status % 2 != 0 && pColl->status < -1){
                    pColl->status++;
                }
                else if (pColl->status % 2 != 0){
                    if (pColl->status != 1) ret = RELEASE_COLLIDING_0;
                    pColl->status = 1;
                }
            }
            else if ((!pColl || pColl->status != 1) && connections[1].size() > 0)
            {
                if (!pColl){
                    pColl = (portalCollision*)malloc(sizeof(portalCollision));
                    pColl->portal = this;
                    pColl->side = 1;
                    pColl->status = 1;
                    fixData->pCollisions.push_back(pColl);

                    ret = NONE_COLLIDING_1;
                }
                else if (pColl->side == 1 && pColl->status < 0 && pColl->status % 2 == -1){
                    pColl->status--;
                }
            }
        }
    }
    else if (pColl){
        if (pColl->status == 1){
            if (pColl->side == (1 ^ side)){
                pColl->status = -1;
                ret = (PortalCollisionType)((int)COLLIDING_RELEASE_0 << (1 ^ side));
            }
            else if (pColl->side == side){
                for (int i = 0; i < fixData->pCollisions.size(); i++){
                    if (fixData->pCollisions.at(i) == pColl){
                        fixData->pCollisions.erase(fixData->pCollisions.begin() + i);
                        break;
                    }
                }
                ret = (PortalCollisionType)((int)COLLIDING_NONE_0 << side);
            }
        }
        
        if (pColl->status % 2 == 0){
            if (pColl->side == 0 && pColl->status < 0){
                if (side){
                    pColl->status--;
                    ret = E_COLLIDING_RELEASE_0;
                }
                else{
                    pColl->status++;
                }
            }
            else if (pColl->side == 1 && pColl->status < 0){
                if (!side){
                    pColl->status--;
                    ret = E_RELEASE_COLLIDING_1;
                }
                else{
                    pColl->status++;
                }
            }
        }
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

bool Portal::isPointIn(b2Vec2 p1, b2Vec2 p2, b2Vec2 point, int side, float sideThold){
    bool check1, check2;
    if (side){
        check1 = pWorld->isLeft(points[0], p1, point, sideThold);
        check2 = pWorld->isLeft(p2, points[1], point, sideThold);
    }else{
        check1 = pWorld->isLeft(p1, points[0], point, sideThold);
        check2 = pWorld->isLeft(points[1], p2, point, sideThold);
    }
    
    bool check3 = pWorld->isLeft(points[1 ^ side], points[side], point, -0.001f);

    return check1 && check2 && check3;
}

bool Portal::shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, portalCollision* coll1, portalCollision* coll2){
    if (fix2 == midFixture) return false;

    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);

    if (fix2->GetType() == b2Shape::Type::e_edge){
        b2Vec2 p1 = fix2->GetBody()->GetWorldPoint(((b2EdgeShape*)fix2->GetShape())->m_vertex1);
        b2Vec2 p2 = fix2->GetBody()->GetWorldPoint(((b2EdgeShape*)fix2->GetShape())->m_vertex2);

        float d1 = abs(points[0].x - p1.x) + abs(points[0].y - p1.y);
        float d2 = abs(points[1].x - p1.x) + abs(points[1].y - p1.y);

        float d3 = abs(points[0].x - p2.x) + abs(points[0].y - p2.y);
        float d4 = abs(points[1].x - p2.x) + abs(points[1].y - p2.y);

        if (isParallel(p1, p2)){
            const float angleThold = 0.0001f;
            float angle = pWorld->vecAngle(wManifold.normal, dir);
            if (coll1->side == 0){
                if (angle < (b2_pi + angleThold) && angle > (b2_pi - angleThold)){
                    return false;
                }
                else{
                    return true;
                }
            }
            else{
                if (angle < angleThold || b2_pi * 2 < (angle + angleThold)){
                    return false;
                }
                else{
                    return true;
                }
            }
        }
        else{
            if ((d1 < 0.001f || d2 < 0.001f) && d1 + d2 > 0.1f){
                int p2Side = getPointSide(p2);
                if (p2Side == coll1->side){
                    return true;
                }
                else{
                    return false;
                }
            }
            else if ((d3 < 0.001f || d4 < 0.001f) && d3 + d4 > 0.1f){
                int p1Side = getPointSide(p1);
                if (p1Side == coll1->side){
                    return true;
                }
                else{
                    return false;
                }
            }
        }
    }

    std::vector<b2Vec2> collPoints;
    const float tHold = 0.0f;
    if (fix2->GetShape()->GetType() == b2Shape::e_circle){
        if (coll2 == NULL){
            collPoints.push_back(fix2->GetBody()->GetWorldPoint(((b2CircleShape*)fix2->GetShape())->m_p));
        }
        else{
            collPoints = getCollisionPoints(fix1, fix2);    
        }
    }
    else{
        collPoints = getCollisionPoints(fix1, fix2);
    }

    if (collPoints.size() == 0 && fix2->GetBody()->GetType() == b2_staticBody){
        b2RayCastOutput rcOutput;
        b2RayCastInput rcInput;
        rcInput.maxFraction = 1.0f;
        for (int i = 0; i < contact->GetManifold()->pointCount; i++){
            bool res;
            rcInput.p1 = wManifold.points[i] + 100.0f * wManifold.normal;
            rcInput.p2 = wManifold.points[i] - 100.0f * wManifold.normal;
            res = fix2->RayCast(&rcOutput, rcInput, b2_staticBody);
            if (res) {
                b2Vec2 result = getRayPoint(rcInput, rcOutput);
                collPoints.push_back(result);
            }
        }
    }

    if (collPoints.size() > 0){
        for (b2Vec2 p : collPoints){
            if (coll2){
                if (pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], p, tHold) ||
                    pWorld->isLeft(coll2->portal->points[1 ^ coll2->side], coll2->portal->points[coll2->side], p, tHold))
                {
                    return false;
                }
            }
            else{
                if (pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], p, tHold)){
                    return false;
                }
            }
        }
        return true;
    }
    else{
        for (int i = 0; i < contact->GetManifold()->pointCount; i++){
            if (coll2){
                if (pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], wManifold.points[i], tHold) ||
                    pWorld->isLeft(coll2->portal->points[1 ^ coll2->side], coll2->portal->points[coll2->side],
                    wManifold.points[i], tHold))
                {
                    return false;
                }
            }
            else{
                if (pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], wManifold.points[i], tHold)){
                    return false;
                }
            }
            return true;
        }

        return true;
    }
}

void Portal::connect(Portal* portal2, bool isReversed, int side1, int side2){
    if (this->isVoid[side1] || portal2->isVoid[side2]) return;

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
}

void Portal::setVoid(int side){
    isVoid[side] = true;

    portalConnection* c1 = (portalConnection*)malloc(sizeof(portalConnection));
    c1->portal1 = this;
    c1->portal2 = this;
    c1->side1 = side;
    c1->side2 = side;
    c1->isReversed = false;

    connections[side].push_back(c1);
}


EvaluationRay::EvaluationRay(PortalWorld* pWorld){
    this->pWorld = pWorld;
}

void EvaluationRay::rayCast(b2Vec2 p1, b2Vec2 p2){
    closestFixture = NULL;

    hitFixtures.clear();
    
    pWorld->RayCast(this, p1, p2);

    if (closestFixture_i){
        closestFixture = closestFixture_i;
        minFraction = minFraction_i;
    }
    
    closestFixture_i = NULL;
    minFraction_i = 1.0f;
}

float EvaluationRay::ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction){
    if (fixture->IsSensor() || fixture->GetBody()->GetType() != b2_staticBody) return 1.0f;

    bool isPortal = false;
    bodyData* bd = (bodyData*)(fixture->GetBody()->GetUserData().pointer);
    if (bd){
        switch (bd->type)
        {
        case PORTAL:
            return 1.0f;
            break;

        default:
            break;
        }
    }
    hitFixtures.push_back(std::pair<b2Fixture*, b2Vec2>(fixture, normal));
    if (fraction < minFraction_i){
        minFraction_i = fraction;
        closestFixture_i = fixture;
        rayNormal = normal;
    }

    return 1.0f;
}

bool Portal::isParallel(b2Vec2 p1, b2Vec2 p2){
    float d1 = abs(PortalWorld::getRealDist(points[0], points[1], p1));
    float d2 = abs(PortalWorld::getRealDist(points[0], points[1], p2));

    if (d1 + d2 < 0.001f){
        return true;
    }
    else{
        return false;
    }
}

PortalWorldQuery::PortalWorldQuery(Portal* portal) {
    this->portal = portal;
    this->mode = 0;
}

void PortalWorldQuery::polygonTransform(b2Fixture* fixture){
    if (fixture->GetShape()->GetType() == b2Shape::e_polygon){
        bool parallelFound = false;

        b2PolygonShape* pShape = (b2PolygonShape*)fixture->GetShape();
        for (int i = 0; i < pShape->m_count; i++){
            b2Vec2 v1 = fixture->GetBody()->GetWorldPoint(pShape->m_vertices[i]);
            b2Vec2 v2 = fixture->GetBody()->GetWorldPoint(pShape->m_vertices[(i + 1) % pShape->m_count]);
            if (portal->isParallel(v1, v2)){
                parallelFound = true;
                break;
            }
        }
        if (parallelFound){
            PortalWorld* pWorld = portal->pWorld;
            b2Body* fBody = fixture->GetBody();
            for (int i = 0; i < pShape->m_count; i++){
                b2FixtureDef edgeDef;
                
                b2EdgeShape edgeShape;
                edgeShape.SetTwoSided(pShape->m_vertices[i], pShape->m_vertices[(i + 1) % pShape->m_count]);

                edgeDef.shape = &edgeShape;

                fBody->CreateFixture(&edgeDef);
            }
            fBody->DestroyFixture(fixture);
        }
    }
}

bool PortalWorldQuery::edgeSplitHorizontal(b2Fixture* fixture){
    b2EdgeShape* eShape = (b2EdgeShape*)fixture->GetShape();
    b2Body* fBody = fixture->GetBody();

    b2Vec2 p1 = fBody->GetWorldPoint(eShape->m_vertex1);
    b2Vec2 p2 = fBody->GetWorldPoint(eShape->m_vertex2);

    PortalWorld* pWorld = portal->pWorld;
    if (portal->isParallel(p1, p2)){
        bool leftFound = false;
        bool rightFound = false;

        b2Vec2 rightPoint;
        b2Vec2 leftPoint;

        float v1 = PortalWorld::getDist(portal->points[0] + portal->dir, portal->points[0], p1);
        float v2 = PortalWorld::getDist(portal->points[1] + portal->dir, portal->points[1], p1);

        if (v1 < 0.0f){
            leftFound = true;
            leftPoint = p1;
        }
        else if (v2 > 0.0f){
            rightFound = true;
            rightPoint = p1;
        }

        v1 = PortalWorld::getDist(portal->points[0] + portal->dir, portal->points[0], p2);
        v2 = PortalWorld::getDist(portal->points[1] + portal->dir, portal->points[1], p2);

        if (v1 < 0.0f){
            if (leftFound) return true;
            leftFound = true;
            leftPoint = p2;
        }
        else if (v2 > 0.0f){
            if (rightFound) return true;
            rightFound = true;
            rightPoint = p2;
        }
        if (!leftFound && !rightFound){
            fBody->DestroyFixture(fixture);
            return true;
        }

        // needed to conserve position of the edge
        b2BodyDef bDef;
        b2Body* dummyBody = pWorld->CreateBody(&bDef);

        if (rightFound){
            b2FixtureDef edgeDef;
            b2EdgeShape edgeShape;
            edgeShape.SetTwoSided(rightPoint, portal->points[1]);
            edgeDef.shape = &edgeShape;

            dummyBody->CreateFixture(&edgeDef);
        }
        if (leftFound){
            b2FixtureDef edgeDef;
            b2EdgeShape edgeShape;
            edgeShape.SetTwoSided(leftPoint, portal->points[0]);
            edgeDef.shape = &edgeShape;

            dummyBody->CreateFixture(&edgeDef);
        }
        fBody->DestroyFixture(fixture);

        return true;
    }
    else{
        return false;
    }
}

void PortalWorldQuery::edgeSplitVertical(b2Fixture* fixture){
    b2EdgeShape* eShape = (b2EdgeShape*)fixture->GetShape();
    b2Body* fBody = fixture->GetBody();

    b2Vec2 p1 = fBody->GetWorldPoint(eShape->m_vertex1);
    b2Vec2 p2 = fBody->GetWorldPoint(eShape->m_vertex2);

    float s1 = PortalWorld::getDist(portal->points[0], portal->points[1], p1);
    float s2 = PortalWorld::getDist(portal->points[0], portal->points[1], p2);

    if (s1 < 0 && s2 < 0 || s1 > 0 && s2 > 0){
        return;
    }
    if (abs(s1) < 0.001f || abs(s2) < 0.001f){
        return;
    }

    float dist1 = abs(PortalWorld::getRealDist(p1, p2, portal->points[0]));
    float dist2 = abs(PortalWorld::getRealDist(p1, p2, portal->points[1]));
    
    bool willSplit = false;
    b2Vec2 splitPoint;

    if (dist1 < 0.0001f){
        willSplit = true;
        splitPoint = portal->points[0];
    }
    else if (dist2 < 0.0001f){
        willSplit = true;
        splitPoint = portal->points[1];
    }

    if (willSplit){
        b2BodyDef bDef;
        b2Body* dummyBody = portal->pWorld->CreateBody(&bDef);

        b2FixtureDef edgeDef;
        b2EdgeShape edgeShape;
        edgeShape.SetTwoSided(p1, splitPoint);
        edgeDef.shape = &edgeShape;

        b2Fixture* fix1 = dummyBody->CreateFixture(&edgeDef);

        edgeShape.SetTwoSided(p2, splitPoint);
        b2Fixture* fix2 = dummyBody->CreateFixture(&edgeDef);

        fBody->DestroyFixture(fixture);
    }
}

bool PortalWorldQuery::ReportFixture(b2Fixture* fixture){
    if (fixture->IsSensor() || fixture->GetBody()->GetType() != b2_staticBody || 
        fixture == portal->midFixture || fixture == portal->yFix[0] || fixture == portal->yFix[1]){
        return true;
    }

    bodyData* bData = (bodyData*)fixture->GetBody()->GetUserData().pointer;
    if (bData && bData->type == PORTAL){
        return true;
    }

    // TODO : store all creation and destructions
    // So, after the portal is destroyed world can be restored to its original state
    if (mode == 0){
        polygonTransform(fixture);
    }
    else if(!edgeSplitHorizontal(fixture)){
        edgeSplitVertical(fixture);
    }

    return true;
}