#include "Portal.h"
#include <math.h>
#include <stdio.h>
#include <chrono>
#include <thread>

Portal::Portal(b2Vec2 pos, b2Vec2 dir, float size, PortalWorld* pWorld, b2Body* onBody)
{
    dir.Normalize();

    this->pWorld = pWorld;
    this->pos = pos;
    this->dirAngle = PortalWorld::calcAngle2(dir);
    this->size = size;
    this->isVoid[0] = false; this->isVoid[1] = false;

    this->sideFixtures[0] = nullptr;
    this->sideFixtures[1] = nullptr;

    createPortalBody(onBody);
    calculatePoints();

    this->queryClass = new PortalWorldQuery(this);
}

Portal::~Portal()
{
    delete queryClass;

    std::set<pbWorldData*> handledIndices;
    for (int i = 0; i < 2; i++)
    {
        std::vector<PortalBody*> leftOutBodies;
        for (PortalBody* pb : collidingBodies[i])
        {
            if (handledIndices.find(pb->worldIndex) != handledIndices.end())
            {
                continue;
            }

            PortalBody* leftOutBody = pb;
            Portal* outPortal = this;
            int side = i;

            for (bodyCollisionStatus* bcs : *pb->bodyMaps)
            {
                b2Vec2 center = bcs->body->getWorldCenterOfMass();
                if (bcs->connection->portal2->getPointSide(center) == bcs->connection->side2)
                {
                    leftOutBody = bcs->body;
                    outPortal = bcs->connection->portal2;
                    side = bcs->connection->side2;
                    break;
                }
            }
            leftOutBodies.push_back(leftOutBody);
            handledIndices.insert(leftOutBody->worldIndex);

            b2Vec2 posChangeDir;
            if (side == 0)
            {
                posChangeDir = outPortal->dir;
            }
            else
            {
                posChangeDir = -outPortal->dir;
            }

            float d = PortalWorld::getDistanceFromPortal(outPortal, leftOutBody);

            if (d > 0.0)
            {
                leftOutBody->body->SetTransform(leftOutBody->body->GetPosition() + d * posChangeDir, leftOutBody->body->GetAngle());
            }
        }

        for (PortalBody* leftOutBody : leftOutBodies)
        {
            for (b2Fixture* fix : leftOutBody->fixtures)
            {
                fixtureData* fixData = (fixtureData*)fix->GetUserData().pointer;
                PBFixtureData* pbfData = (PBFixtureData*)fixData->data;
                pbfData->pCollisions.clear();
            }

            for (int j = leftOutBody->worldIndex->pbIndices.size() - 1; j > -1; j--)
            {
                PortalBody* deleteStat = leftOutBody->worldIndex->pbIndices.at(j);

                if (deleteStat == leftOutBody) continue;

                for (auto pair : deleteStat->worldIndex->destroyCallbacks)
                {
                    pair.first(deleteStat, pair.second);
                }

                delete deleteStat;
            }

            for (int i = 0; i < 2; i++)
            {
                leftOutBody->outFixtures[i].clear();
                leftOutBody->createDestroyMap[i].clear();
            }
            leftOutBody->bodyMaps->clear();
        }
    }

    for (int i = 0; i < 2; i++)
    {
        if (isVoid[i])
        {
            free(connections[i][0]);
            continue;
        }

        for (int j = 0; j < connections[i].size(); j++)
        {
            Portal* otherPortal = connections[i][j]->portal2;
            if (otherPortal == this)
            {
                free(connections[i][j]);
                break;
            }

            int connectionSide2 = connections[i][j]->side2;
            for (int k = 0; k < otherPortal->connections[connectionSide2].size(); k++)
            {
                portalConnection* pc = otherPortal->connections[connectionSide2].at(k);
                if (pc->portal2 == this)
                {
                    free(pc);
                    otherPortal->connections[connectionSide2].erase(otherPortal->connections[connectionSide2].begin() + k);
                }
            }

            free(connections[i][j]);
        }
    }

    free((void*)midFixture->GetUserData().pointer);

    body->DestroyFixture(midFixture);
    body->DestroyFixture(yFix[0]);
    body->DestroyFixture(yFix[1]);
}

b2Fixture* Portal::getPreDestructionFixture()
{
    fixtureData* newFixtureData = nullptr;
    int newRange[2] = { 0, 0 };
    b2Vec2 newPoints[2] = { points[1], points[0] };
    b2Fixture* newSideFixtures[2] = { nullptr, nullptr };

    fixtureData* fixData = (fixtureData*)sideFixtures[0]->GetUserData().pointer;
    newFixtureData = fixData;
    s_edgeLocalData* eLocalData = (s_edgeLocalData*)fixData->data;
    newRange[0] = eLocalData->globalRange[0];

    newSideFixtures[0] = eLocalData->prevNextFixtures[0];

    b2EdgeShape* eShape = (b2EdgeShape*)sideFixtures[0]->GetShape();
    newPoints[0] = eShape->m_vertex1;
    body->DestroyFixture(sideFixtures[0]);

    fixtureData* fixData2 = (fixtureData*)sideFixtures[1]->GetUserData().pointer;
    s_edgeLocalData* eLocalData2 = (s_edgeLocalData*)fixData2->data;
    newRange[1] = eLocalData2->globalRange[1];

    newSideFixtures[1] = eLocalData2->prevNextFixtures[1];

    delete eLocalData2;
    delete fixData2;

    eShape = (b2EdgeShape*)sideFixtures[1]->GetShape();
    newPoints[1] = eShape->m_vertex2;
    body->DestroyFixture(sideFixtures[1]);

    eLocalData->prevNextFixtures[0] = newSideFixtures[0];
    eLocalData->prevNextFixtures[1] = newSideFixtures[1];

    eLocalData->globalRange[0] = newRange[0];
    eLocalData->globalRange[1] = newRange[1];

    b2EdgeShape newShape;
    newShape.SetTwoSided(newPoints[0], newPoints[1]);
    b2Fixture* newFix = body->CreateFixture(&newShape, 0.0f);
    newFix->GetUserData().pointer = (uintptr_t)newFixtureData;

    if (newSideFixtures[0])
    {
        fixtureData* nextFData = (fixtureData*)newSideFixtures[0]->GetUserData().pointer;
        if (nextFData && nextFData->type == F_PORTAL)
        {
            Portal* p = (Portal*)nextFData->data;
            p->sideFixtures[1] = newFix;
            p->range[1] = range[0];
        }
        else if (nextFData && nextFData->type == F_EDGE)
        {
            s_edgeLocalData* nextELocal = (s_edgeLocalData*)nextFData->data;
            nextELocal->prevNextFixtures[1] = newFix;
            nextELocal->globalRange[1] = range[0];
        }
    }

    if (newSideFixtures[1])
    {
        fixtureData* nextFData = (fixtureData*)newSideFixtures[1]->GetUserData().pointer;
        if (nextFData && nextFData->type == F_PORTAL)
        {
            Portal* p = (Portal*)nextFData->data;
            p->sideFixtures[0] = newFix;
            p->range[0] = range[1];
        }
        else if (nextFData && nextFData->type == F_EDGE)
        {
            s_edgeLocalData* nextELocal = (s_edgeLocalData*)nextFData->data;
            nextELocal->prevNextFixtures[0] = newFix;
            nextELocal->globalRange[0] = range[1];
        }
    }

    return newFix;
}

void Portal::reviveAround()
{
    boxMin = b2Vec2(fmin(points[0].x, points[1].x) - 0.01f, fmin(points[0].y, points[1].y) - 0.01f);
    boxMax = b2Vec2(fmax(points[0].x, points[1].x) + 0.01f, fmax(points[0].y, points[1].y) + 0.01f);

    b2AABB queryAABB = { boxMin, boxMax };
    pWorld->QueryAABB(queryClass, queryAABB);
}

void Portal::calculatePoints()
{
    pos = body->GetWorldPoint(posOffset);
    dirAngle = body->GetAngle() + angleOffset;

    dir = b2Vec2(cos(dirAngle), sin(dirAngle));

    points[0].x = pos.x + cos(dirAngle + b2_pi * 0.5f) * size;
    points[0].y = pos.y + sin(dirAngle + b2_pi * 0.5f) * size;

    points[1].x = pos.x - cos(dirAngle + b2_pi * 0.5f) * size;
    points[1].y = pos.y - sin(dirAngle + b2_pi * 0.5f) * size;

    rotDir = points[0] - points[1];
    rotDir.Normalize();

    float inMargin = 0.01f / b2Distance(points[0], points[1]);

    rcInp1.p1 = points[0];
    rcInp1.p2 = points[1];
    rcInp1.maxFraction = 1.0f - inMargin;

    rcInp2.p1 = points[1];
    rcInp2.p2 = points[0];
    rcInp2.maxFraction = 1.0f - inMargin;
}

void Portal::createPortalBody(b2Body* onBody)
{
    if (onBody)
    {
        body = onBody;
        posOffset = onBody->GetLocalPoint(pos);
        angleOffset = dirAngle - onBody->GetAngle();
    }
    else
    {
        b2BodyDef bd;
        bd.bullet = true;
        bd.angularDamping = 0.0f;
        bd.linearDamping = 0.0f;
        bd.position = pos;
        bd.type = b2_staticBody;
        bd.angle = dirAngle;
        body = pWorld->CreateBody(&bd);

        posOffset = b2Vec2(0.0f, 0.0f);
        angleOffset = 0.0f;
    }

    b2Vec2 leftPoint = PortalWorld::rotateVec(b2Vec2(0.0f, -size), angleOffset) + posOffset;
    b2Vec2 rightPoint = PortalWorld::rotateVec(b2Vec2(0.0f, +size), angleOffset) + posOffset;

    b2EdgeShape shape;
    shape.SetTwoSided(leftPoint, rightPoint);

    b2FixtureDef midPortal;
    midPortal.shape = &shape;
    midPortal.isSensor = false;
    
    midFixture = body->CreateFixture(&midPortal);

    shape.SetTwoSided(leftPoint, leftPoint);
    yFix[0] = body->CreateFixture(&shape, 0.0f);

    shape.SetTwoSided(rightPoint, rightPoint);
    yFix[1] = body->CreateFixture(&shape, 0.0f);
    fixtureData* mainFixData = new fixtureData();
    mainFixData->type = F_PORTAL;
    mainFixData->data = (void*)this;

    midFixture->GetUserData().pointer = (uintptr_t)mainFixData;
    yFix[0]->GetUserData().pointer = (uintptr_t)mainFixData;
    yFix[1]->GetUserData().pointer = (uintptr_t)mainFixData;
}

void Portal::setLinVel(b2Vec2 speed)
{
    body->SetLinearVelocity(speed);
}

b2Vec2 Portal::getLinVel()
{
    return body->GetLinearVelocity();
}

void Portal::setAngVel(float angVel)
{
    body->SetAngularVelocity(angVel);
}

float Portal::getAngVel()
{
    return body->GetAngularVelocity();
}

int Portal::getPointSide(b2Vec2 point)
{
    return PortalWorld::isLeft(points[1], points[0], point, 0.0f);
}

int Portal::getPointSideVertical(b2Vec2 point)
{
    return PortalWorld::isLeft(pos + dir, pos, point, 0.0f);
}

PortalCollisionType Portal::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){    
    PortalCollisionType ret = DEFAULT_COLLISION;
    
    if (fix1 == midFixture){
        ret = handleCollidingFixtures(contact, fix2);
    }

    return ret;
}

PortalCollisionType Portal::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    fixtureData* fData = (fixtureData*)fix2->GetUserData().pointer;
    if (!fData || fData->type != F_PORTAL_BODY) return DEFAULT_COLLISION;

    PortalCollisionType ret = DEFAULT_COLLISION;

    if (fix1 == midFixture){
        int side = getFixtureSide(fix2);
        PBFixtureData* pbfData = (PBFixtureData*)fData->data;
        portalCollision* pColl;
        auto iter = pbfData->getPortalCollision(this);
        if (iter != pbfData->pCollisions.end()){
            pColl = *iter;
        }
        else{
            pColl = NULL;
        }

        if (!pColl) return ret;

        if (pColl->side == (side ^ 1) && pColl->status == 1){
            pColl->status = -1;            
            ret = (PortalCollisionType)((int)COLLIDING_RELEASE_0 << (1 ^ side));
            return ret;
        }
        else if (pColl->side == side && pColl->status == 1){
            for (int i = 0; i < pbfData->pCollisions.size(); i++) {
                if (pbfData->pCollisions.at(i) == pColl) {
                    free(pbfData->pCollisions.at(i));
                    pbfData->pCollisions.erase(pbfData->pCollisions.begin() + i);
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

PortalCollisionType Portal::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2)
{
    PortalCollisionType ret = DEFAULT_COLLISION;
    
    if (fix1 == midFixture)
    {
        fixtureData* fixData = (fixtureData*)fix2->GetUserData().pointer;
        PBFixtureData* pbfData = (PBFixtureData*)fixData->data;
        ret = handleCollidingFixtures(contact, fix2);
        portalCollision* pColl;
        auto iter = pbfData->getPortalCollision(this);
        if (iter != pbfData->pCollisions.end())
        {
            pColl = *iter;
        }
        else
        {
            pColl = NULL;
        }
        if (pColl)
        {
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
    PortalCollisionType ret = DEFAULT_COLLISION;
    fixtureData* fixData = (fixtureData*)fix2->GetUserData().pointer;
    PBFixtureData* pbfData = (PBFixtureData*)fixData->data;
    const float angleThold = pbfData->angleThold;

    portalCollision* pColl;
    auto iter = pbfData->getPortalCollision(this);
    if (iter != pbfData->pCollisions.end()) {
        pColl = *iter;
    }
    else {
        iter = pbfData->getPortalCollision(this, 0);
        if (iter != pbfData->pCollisions.end()){
            return ret;
        }
        pColl = NULL;
    }

    float angle;

    if (fix2->IsSensor())
    {
        angle = 0.0;
    }
    else
    {
        b2WorldManifold wManifold;
        contact->GetWorldManifold(&wManifold);

        angle = pWorld->vecAngle(wManifold.normal, dir);
    }
    

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
                    if (pColl->status != 1){
                        ret = RELEASE_COLLIDING_1;
                    }
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
                    pbfData->pCollisions.push_back(pColl);

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
                    if (pColl->status != 1){
                        ret = RELEASE_COLLIDING_0;
                    }
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
                    pbfData->pCollisions.push_back(pColl);

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
                for (int i = 0; i < pbfData->pCollisions.size(); i++) {
                    if (pbfData->pCollisions.at(i) == pColl) {
                        free(pbfData->pCollisions.at(i));
                        pbfData->pCollisions.erase(pbfData->pCollisions.begin() + i);
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
        b2Vec2 l_pos = fix->GetBody()->GetWorldPoint(shape->m_p);
        if (pWorld->isLeft(points[0], points[1], l_pos, 0.0f)) side = 0;
    }
    else{
        b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();
        float totalDist = 0.0f;
        for (int i = 0; i < shape->m_count; i++){
            b2Vec2 l_pos = fix->GetBody()->GetWorldPoint(shape->m_vertices[i]);
            totalDist += pWorld->getDist(points[0], points[1], l_pos);
        }
        if (totalDist >= 0.0f) side = 0;
    }

    return side;
}

bool Portal::shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, portalCollision* coll1, portalCollision* coll2){
    if (fix2 == midFixture) return false;

    const float tHold = -0.001f;

	if (fix2->GetType() == b2Shape::Type::e_edge) {
		b2WorldManifold wManifold;
		contact->GetWorldManifold(&wManifold);

		b2Vec2 p1 = fix2->GetBody()->GetWorldPoint(((b2EdgeShape*)fix2->GetShape())->m_vertex1);
		b2Vec2 p2 = fix2->GetBody()->GetWorldPoint(((b2EdgeShape*)fix2->GetShape())->m_vertex2);

		float d1 = abs(points[0].x - p1.x) + abs(points[0].y - p1.y);
		float d2 = abs(points[1].x - p1.x) + abs(points[1].y - p1.y);

		float d3 = abs(points[0].x - p2.x) + abs(points[0].y - p2.y);
		float d4 = abs(points[1].x - p2.x) + abs(points[1].y - p2.y);

		if (isParallel(p1, p2)) {
			const float angleThold = 0.0001f;
			float angle = pWorld->vecAngle(wManifold.normal, dir);
			if (coll1->side == 0) {
				if (angle < (b2_pi + angleThold) && angle >(b2_pi - angleThold)) {
					return false;
				}
				else {
					return true;
				}
			}
			else {
				if (angle < angleThold || b2_pi * 2 < (angle + angleThold)) {
					return false;
				}
				else {
					return true;
				}
			}
		}
		else {
			if ((d1 < 0.001f || d2 < 0.001f) && d1 + d2 > 0.1f) {
				int p2Side = getPointSide(p2);
				if (p2Side == coll1->side) {
					return true;
				}
				else {
					return false;
				}
			}
			else if ((d3 < 0.001f || d4 < 0.001f) && d3 + d4 > 0.1f) {
				int p1Side = getPointSide(p1);
				if (p1Side == coll1->side) {
					return true;
				}
				else {
					return false;
				}
			}
		}
	}

    if (fix1->GetShape()->GetType() == b2Shape::e_circle && fix2->GetType() == b2Shape::e_circle){        
        b2Vec2 collPointA;
        b2Vec2 collPointB;

        b2Vec2 center1 = fix1->GetBody()->GetWorldPoint(((b2CircleShape*)fix1->GetShape())->m_p);
        b2Vec2 center2 = fix2->GetBody()->GetWorldPoint(((b2CircleShape*)fix2->GetShape())->m_p);

        b2Vec2 diff = center2 - center1;
        float len = diff.Normalize();
        if (len == 0.0f){
            collPointA = center1;
            collPointB = center1;
        }
        else{
            collPointA = center1 + ((b2CircleShape*)fix1->GetShape())->m_radius * diff;
            collPointB = center2 - ((b2CircleShape*)fix2->GetShape())->m_radius * diff;
        }

        if (coll2){
            if (pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], collPointA, tHold) ||
                pWorld->isLeft(coll2->portal->points[1 ^ coll2->side], coll2->portal->points[coll2->side], collPointA, tHold) ||
                pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], collPointB, tHold) ||
                pWorld->isLeft(coll2->portal->points[1 ^ coll2->side], coll2->portal->points[coll2->side], collPointB, tHold))
            {
                return false;
            }
        }
        else {
            if (pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], collPointA, tHold) ||
                pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], collPointB, tHold))
            {
                return false;
            }
        }
    }
    else{
        b2DistanceOutput out;
        b2DistanceProxy distanceProxyA;
        b2DistanceProxy distanceProxyB;
        distanceProxyA.Set(fix1->GetShape(), b2_dynamicBody);
        distanceProxyB.Set(fix2->GetShape(), fix2->GetBody()->GetType());

        b2DistanceInput input;
        input.proxyA = distanceProxyA;
        input.proxyB = distanceProxyB;
        input.transformA = fix1->GetBody()->GetTransform();
        input.transformB = fix2->GetBody()->GetTransform();
        // finds single collision between fixtures
        // not suitable for this problem, we need 2 collision points at the outer edge of the fixtures
        input.useRadii = false;

        // may be used in the future
        b2SimplexCache cache;
        cache.count = 0;

        b2Distance(&out, &cache, &input);

        if (fix1->GetType() == b2Shape::e_circle && fix2->GetType() != b2Shape::e_circle){
            b2Vec2 circCenter = fix1->GetBody()->GetWorldPoint((((b2CircleShape*)fix1->GetShape())->m_p));
            b2Vec2 diff = out.pointB - circCenter;
            float len = diff.Normalize();
            if (len > 0.0001f){
                out.pointA = circCenter + ((b2CircleShape*)fix1->GetShape())->m_radius * diff;
            }
        }
        else if (fix2->GetType() == b2Shape::e_circle && fix1->GetType() != b2Shape::e_circle){
            b2Vec2 circCenter = fix2->GetBody()->GetWorldPoint((((b2CircleShape*)fix2->GetShape())->m_p));
            b2Vec2 diff = out.pointA - circCenter;
            float len = diff.Normalize();
            if (len > 0.0001f){
                out.pointB = circCenter + ((b2CircleShape*)fix2->GetShape())->m_radius * diff;
            }
        }

        // ((DebugDrawer*)pWorld->getDebugDraw())->addPointToQueue(out.pointA, 25.0f, b2Color(1, 1, 1, 0.2));
        // ((DebugDrawer*)pWorld->getDebugDraw())->addPointToQueue(out.pointB, 25.0f, b2Color(1, 1, 1, 0.2));

        // ((DebugDrawer*)pWorld->getDebugDraw())->addPointToQueue(getFixtureCenter(fix1), 5.0f, b2Color(0, 1, 1, 1));

        if (coll2){
            if (pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], out.pointA, tHold) ||
                pWorld->isLeft(coll2->portal->points[1 ^ coll2->side], coll2->portal->points[coll2->side], out.pointA, tHold) ||
                pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], out.pointB, tHold) ||
                pWorld->isLeft(coll2->portal->points[1 ^ coll2->side], coll2->portal->points[coll2->side], out.pointB, tHold))
            {
                return false;
            }
        }
        else{
            if (pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], out.pointA, tHold) ||
                pWorld->isLeft(points[1 ^ coll1->side], points[coll1->side], out.pointB, tHold))
            {
                return false;
            }
        }
    }

    return true;
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


b2Vec2 Portal::getFixtureCenter(b2Fixture* fix){
	if (fix->GetType() == b2Shape::Type::e_circle){
		b2Vec2 point = ((b2CircleShape*)fix->GetShape())->m_p;
		return fix->GetBody()->GetWorldPoint(point);
	}
	else{
		b2Vec2 avgPoints = b2Vec2(0, 0);
		b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();
		for (int i = 0; i < polyShape->m_count; i++){
			avgPoints += polyShape->m_vertices[i];
		}
		b2Vec2 avgPoint = (1.0f / polyShape->m_count) * avgPoints;
		avgPoint = fix->GetBody()->GetWorldPoint(avgPoint);
		return avgPoint;
	}
}

b2Vec2 Portal::getRayPoint(b2RayCastInput& rcInput, b2RayCastOutput& rcOutput) {
	b2Vec2 result = rcInput.p1 +
				b2Vec2(
					(rcInput.p2.x - rcInput.p1.x) * rcOutput.fraction,
					(rcInput.p2.y - rcInput.p1.y) * rcOutput.fraction
				);

	return result;
}

PortalWorldQuery::PortalWorldQuery(Portal* portal)
{
    this->portal = portal;
}

bool PortalWorldQuery::ReportFixture(b2Fixture* fixture)
{
    fixture->GetBody()->SetAwake(true);

    return true;
}