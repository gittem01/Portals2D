#include "PortalBody.h"
#include "Portal.h"
#include "PortalWorld.h"
#include "glm/common.hpp"

#define ODD_MASK 0b10101010101010101010101010101010

PortalBody::PortalBody(b2Body* bBody, PortalWorld* pWorld)
{
    this->pWorld = pWorld;
    this->body = bBody;

    this->body->SetGravityScale(0.0f);

    bodyData* bd = (bodyData*)malloc(sizeof(bodyData));
    bd->data = this;
    bd->type = B_PORTAL_BODY;
    bd->extraData = nullptr;
    body->GetUserData().pointer = (uintptr_t)bd;

    isMirrored = false;

    this->numFixtures = 0;
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext())
    {
        fixtures.push_back(fix);
        fixtureData* fixData = new fixtureData();
        fixData->type = F_PORTAL_BODY;
        PBFixtureData* pbfData = new PBFixtureData();
        fixData->data = pbfData;
        pbfData->index = numFixtures;
        pbfData->angleThold = calculateAngleThold(fix);
        fix->GetUserData().pointer = (uintptr_t)fixData;
        numFixtures++;
    }
}

PortalBody::~PortalBody(){
    pWorld->handledBodies.insert(worldIndex);
    for (int i = 0; i < worldIndex->pbIndices.size(); i++)
    {
        if (worldIndex->pbIndices.at(i) == this)
        {
            worldIndex->pbIndices.erase(worldIndex->pbIndices.begin() + i);
            break;
        }
    }

    for (int i = 0; i < bodyMaps.size(); i++)
    {
        bodyCollisionStatus* bodyCollStatus = bodyMaps.at(i);
		for (int j = 0; j < bodyCollStatus->body->bodyMaps.size(); j++)
        {
			bodyCollisionStatus* bcs = bodyCollStatus->body->bodyMaps.at(j);
			if (bcs->body == this)
            {
				bodyCollStatus->body->bodyMaps.erase(bodyCollStatus->body->bodyMaps.begin() + j);
                bcs->connection->portal1->collidingBodies[bcs->connection->side1].erase(bodyCollStatus->body);
                bcs->connection->portal2->collidingBodies[bcs->connection->side2].erase(this);
				j--;
				delete bcs;
			}
		}
        delete bodyCollStatus;
    }

    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext())
    {  
        fixtureData* fixData = (fixtureData*)fix->GetUserData().pointer;
        fix->GetUserData().pointer = 0;
        PBFixtureData* pbfData = (PBFixtureData*)fixData->data;
        for (portalCollision* coll : pbfData->pCollisions)
        {
            free(coll);
        }
        delete fixData;
        delete pbfData;
    }

    uintptr_t bData = body->GetUserData().pointer;
    body->GetUserData().pointer = 0;
    free((void*)bData);

    b2JointEdge* jointEdge = body->GetJointList();
    for ( ; jointEdge ; ){
        b2JointEdge* nextEdge = jointEdge->next;
        if (jointEdge->joint->GetType() == e_unknownJoint){
            pWorld->DestroyCustomJoint(jointEdge->joint);
        }
        jointEdge = nextEdge;
    }

    pWorld->DestroyBody(body);
}

void PortalBody::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2)
{
    fixtureData* fixData = (fixtureData*)fix2->GetUserData().pointer;
    
    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, fixData);
    if (!shouldCollide)
    {
        contact->SetEnabled(false);
        return;
    }

    if (fixData && fixData->type == F_PORTAL)
    {
        Portal* portal = (Portal*)fixData->data;
        PortalCollisionType out = portal->collisionBegin(contact, fix2, fix1);
        handleOut(fix1, portal, out);
    }
}

void PortalBody::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    fixtureData* fixData = (fixtureData*)fix2->GetUserData().pointer;

    if (fixData && fixData->type == F_PORTAL && pWorld->IsLocked())
    {
        Portal* p = (Portal*)fixData->data;
        PortalCollisionType out = p->collisionEnd(contact, fix2, fix1);
        handleOut(fix1, p, out);
    }
}

void PortalBody::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    fixtureData* fixData = (fixtureData*)fix2->GetUserData().pointer;
    
    if (!contact->IsEnabled()){
        return;
    }

    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, fixData);
    if (!shouldCollide){
        contact->SetEnabled(false);
        return;
    }

    if (fixData && fixData->type == F_PORTAL){
        Portal* portal = (Portal*)fixData->data;
        PortalCollisionType out = portal->preCollision(contact, fix2, fix1);
        handleOut(fix1, portal, out);
    }
}

bool PortalBody::shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, fixtureData* fData)
{
    fixtureData* fixData1 = (fixtureData*)fix1->GetUserData().pointer;
    PBFixtureData* pbfData1 = (PBFixtureData*)fixData1->data;
    std::vector<portalCollision*> collidingPortals1 = pbfData1->pCollisions;

    if (fData && fData->type == F_PORTAL)
    {
        for (portalCollision* coll1 : collidingPortals1)
        {
            if (fix2 == coll1->portal->yFix[0] || fix2 == coll1->portal->yFix[1])
            {
                return true;
            }
        }

        Portal* portal = (Portal*)fData->data;
        if (fix2 != portal->yFix[0] && fix2 != portal->yFix[1])
        {
            for (bodyCollisionStatus* stat : bodyMaps){
                if (stat->body->bodyMaps.size() < 2)
                {
                    continue;
                }
                b2Fixture* loopFix = stat->body->fixtures[pbfData1->index];
                fixtureData* fixDataLoop = (fixtureData*)loopFix->GetUserData().pointer;
                PBFixtureData* pbfDataLoop = (PBFixtureData*)fixDataLoop->data;
                for (portalCollision* pc : pbfDataLoop->pCollisions)
                {
                    if (pc->portal != stat->connection->portal2)
                    {
                        if (pc->status < 0)
                        {
                            return false;
                        }
                        else if (pc->status == 1)
                        {
                            b2WorldManifold wManifold;
                            contact->GetWorldManifold(&wManifold);
                            b2Vec2 point;
                            if (contact->GetManifold()->pointCount == 1)
                            {
                                point = wManifold.points[0];
                            }
                            else
                            {
                                point = 0.5f * (wManifold.points[0] + wManifold.points[1]);
                            }
                            
                            b2Vec2 intersectionPoint = PortalWorld::getLineIntersection(
                                point - wManifold.normal, point + wManifold.normal,
                                portal->points[0], portal->points[1]);

                            intersectionPoint = PortalWorld::getRelativePortalPosition_W(intersectionPoint, stat->connection);
                            int intersectSide = pc->portal->getPointSide(intersectionPoint);
                            if (intersectSide != pc->side)
                            {
                                return false;
                            }
                        }
                    }
                }
            }
        }

        for (auto pColl : collidingPortals1)
        {
            if (pColl->portal == portal) return true;
        }
    }
    else if (collidingPortals1.size() == 0)
    {
        return true;
    }

    std::vector<portalCollision*> collidingPortals2;
    fixtureData* fixData2 = (fixtureData*)fix2->GetUserData().pointer;
    PBFixtureData* pbfData2;
    if (fixData2)
    {
        pbfData2 = (PBFixtureData*)fixData2->data;
    }
    else
    {
        pbfData2 = nullptr;
    }

    if (fData && fData->type == F_PORTAL_BODY)
    {
        collidingPortals2 = pbfData2->pCollisions;
    }
    
    for (portalCollision* coll1 : collidingPortals1)
    {
        if (fData && fData->type == F_PORTAL_BODY)
        {
            bodyData* bd = (bodyData*)fix2->GetBody()->GetUserData().pointer;
            PortalBody* pb = (PortalBody*)bd->data;
            int val = shouldCollideHelper(coll1, pbfData1, pbfData2, pb);
            if (val == 1)
            {
                continue;
            }
            else if (val == 2)
            {
                return false;
            }
        }

        int status = 2;
        if (fData && fData->type == F_PORTAL_BODY) status = pbfData2->getPortalStatus(coll1->portal, 1 ^ coll1->side);
        if (coll1->status < 0 || status == 1)
        {
            return false;
        }

        bool didJob = false;
        if (collidingPortals2.size() > 0)
        {
            for (portalCollision* coll2 : collidingPortals2)
            {
                if (pbfData1->getPortalCollision(coll2->portal) != pbfData1->pCollisions.end())
                {
                    continue;
                }

                int val = shouldCollideHelper(coll2, pbfData2, pbfData1, this);
                if (val == 1){
                    continue;
                }
                else if (val == 2){
                    return false;
                }
                bool shouldCollide = coll1->portal->shouldCollide(contact, fix1, fix2, coll1, coll2);
                if (!shouldCollide) return false;

                didJob = true;
            }
        }
        else{
            bool shouldCollide = coll1->portal->shouldCollide(contact, fix1, fix2, coll1, NULL);
            if (!shouldCollide){
                return false;
            }
        }
        if (!didJob){
            bool shouldCollide = coll1->portal->shouldCollide(contact, fix1, fix2, coll1, NULL);
            if (!shouldCollide){
                return false;
            }
        }
    }

    return true;
}

int PortalBody::shouldCollideHelper(portalCollision* coll, PBFixtureData* pbfData1, PBFixtureData* pbfData2, PortalBody* pb){
    int status1 = pbfData1->getPortalStatus(coll->portal, coll->side);
    int status2 = pbfData2->getPortalStatus(coll->portal, coll->side);
    if (status1 == 1 && status2 == 1){

    }
    else{
        return 0;
    }

    b2Fixture* otherFix2 = NULL;
    for (bodyCollisionStatus* bcs : pb->bodyMaps){
        if (bcs->connection->portal1 == coll->portal){
            otherFix2 = bcs->body->fixtures[pbfData2->index];
        }
    }

    if (otherFix2 == NULL){
        return 1;
    }
    
    for (bodyCollisionStatus* bcs : bodyMaps){
        if (bcs->body->bodyMaps.size() < 2){
            continue;
        }

        b2Fixture* otherFix1 = bcs->body->fixtures[pbfData1->index];

        fixtureData* loopFixData = (fixtureData*)otherFix1->GetUserData().pointer;
        PBFixtureData* pbfDataLoop = (PBFixtureData*)loopFixData->data;
        for (portalCollision* pc : pbfDataLoop->pCollisions){
            if (pc->portal == bcs->connection->portal1 || pc->portal == bcs->connection->portal2)
            {
                continue;
            }

            b2DistanceOutput out;
            b2DistanceProxy distanceProxyA;
            b2DistanceProxy distanceProxyB;
            distanceProxyA.Set(otherFix1->GetShape(), b2_dynamicBody);
            distanceProxyB.Set(otherFix2->GetShape(), b2_dynamicBody);

            b2DistanceInput input;
            input.proxyA = distanceProxyA;
            input.proxyB = distanceProxyB;
            input.transformA = otherFix1->GetBody()->GetTransform();
            input.transformB = otherFix2->GetBody()->GetTransform();
            input.useRadii = false;

            b2SimplexCache cache;
            cache.count = 0;

            b2Distance(&out, &cache, &input);

            if (PortalWorld::isLeft(pc->portal->points[1 ^ pc->side], pc->portal->points[pc->side], out.pointA, -0.001f) ||
                PortalWorld::isLeft(pc->portal->points[1 ^ pc->side], pc->portal->points[pc->side], out.pointB, -0.001f))
            {
                return 2;
            }
        }
    }

    return 1;
}

void PortalBody::outHelper(b2Fixture* fix, Portal* portal, int status, PortalCollisionType out)
{
    int side = (uint32_t)out & ODD_MASK ? 1 : 0;
    if (status < 0)
    {
        outFixtures[side][portal]--;
    }
    else
    {
        if (out & (RELEASE_COLLIDING_0 | RELEASE_COLLIDING_1))
        {
            outFixtures[side][portal]++;
        }
        else if (out & (NONE_COLLIDING_0 | NONE_COLLIDING_1))
        {
            if (outFixtures[side].find(portal) == outFixtures[side].end())
            {
                outFixtures[side][portal] = numFixtures - 1;
            }
            else
            {
                outFixtures[side][portal]--;
            }
            for (b2ContactEdge* contactedge=body->GetContactList(); contactedge; contactedge=contactedge->next)
            {
                if (!contactedge->contact->IsEnabled())
                {
                    continue;
                }
                b2Fixture* m_fixture;
                b2Fixture* o_fixture;
                bool collide = true;
                if (contactedge->contact->GetFixtureA()->GetBody() == this->body)
                {
                    m_fixture = contactedge->contact->GetFixtureA();
                    o_fixture = contactedge->contact->GetFixtureB();
                }
                else
                {
                    o_fixture = contactedge->contact->GetFixtureA();
                    m_fixture = contactedge->contact->GetFixtureB();
                }
                fixtureData* fData = (fixtureData*)o_fixture->GetUserData().pointer;
                if ((fData && fData->type != F_PORTAL) || !fData)
                {
                    collide = this->shouldCollide(contactedge->contact, m_fixture, o_fixture, fData);
                }
                if (!collide)
                {
                    contactedge->contact->SetEnabled(false);
                }
            }
        }
    }

    // Extra void portal logic
    if (portal->isVoid[side] && status < 0)
    {
        if (outFixtures[side][portal] == -numFixtures)
        {
            pWorld->destroyBodies.insert(this);
            return;
        }
    }
}

void PortalBody::postHandle()
{
    std::vector<Portal*> destroyRequests;
    int createCount = 0;
    for (int side = 0; side < 2; side++)
    {
        for (auto iter : createDestroyMap[side])
        {
            if (iter.second > 0)
            {
                /*bodyStruct* s = (bodyStruct*)malloc(sizeof(bodyStruct));
                *s = {this, iter.first, side};
                createBodies.push_back(s);*/
                createCount++;

                bool shouldDestroyItself = pWorld->createCloneBody(iter.first, side, this);
                if (shouldDestroyItself)
                {
                    destroyRequests.push_back(iter.first);
                }
            }
            else if (iter.second < 0)
            {
                for (bodyCollisionStatus* s : bodyMaps)
                {
                    if (s->connection->portal1 == iter.first && s->connection->side1 == side)
                    {
                        pWorld->destroyBodies.insert(s->body);
                    }
                }
            }
        }
        createDestroyMap[side].clear();
    }

    if (destroyRequests.size() == 1)
    {
        if (bodyMaps.size() == 1)
        {
            pWorld->destroyBodies.insert(this);
        }
        else
        {
            for (bodyCollisionStatus* bcs : bodyMaps)
            {
                if (bcs->connection->portal1 == destroyRequests.at(0))
                {
                    pWorld->destroyBodies.insert(bcs->body);
                }
            }
        }
    }
    else if (destroyRequests.size() > 1)
    {
        if (destroyRequests.size() != createCount)
        {
            for (Portal* portal : destroyRequests)
            {
                for (bodyCollisionStatus* bcs : bodyMaps)
                {
                    if (bcs->connection->portal1 == portal)
                    {
                        pWorld->destroyBodies.insert(bcs->body);
                    }
                }
            }
        }
        else
        {
            // How did this happen?
            // I want to see it happen and then fix it
        }
    }

#if 0

    if (worldIndex->pbIndices.size() == 1)
    {
        body->ApplyForceToCenter(body->GetMass() * pWorld->GetGravity(), true);
    }
    else
    {
        b2Vec2 centerOfMass = body->GetPosition();
        b2MassData md;
        body->GetMassData(&md);
        centerOfMass = body->GetWorldPoint(md.center);
        for (bodyCollisionStatus* bcs : bodyMaps)
        {
            if (bcs->connection->portal1->getPointSide(centerOfMass) == bcs->connection->side1)
            {
                body->ApplyForceToCenter(worldIndex->pbIndices.size() * body->GetMass() * pWorld->GetGravity(), true);
                break;
            }
        }
    }
#else
    if (body->IsAwake() && worldIndex->gravityScale > 0.0f)
    {
        if (bodyMaps.size() > 0)
        {
            for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext())
            {
                applyPartialGravity(fix);
            }
        }
        else
        {
            body->ApplyForceToCenter(worldIndex->gravityScale * body->GetMass() * pWorld->GetGravity(), true);
        }
    }
#endif
}

bool PortalBody::shouldCreate(b2Fixture* fix, Portal* portal, PortalCollisionType out){
    int side = (uint32_t)out & ODD_MASK ? 1 : 0;
    if (portal->isVoid[side]) return false;

    if (outFixtures[side].find(portal) == outFixtures[side].end() || outFixtures[side][portal] == numFixtures)
    {
        return true;
    }

    return false;
}

void PortalBody::releaseOut2(b2Fixture* fix1, Portal* portal)
{
    // TODO : optimize this
    b2Body* body = fix1->GetBody();
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext())
    {
        fixtureData* fixData = (fixtureData*)fix->GetUserData().pointer;
        PBFixtureData* pbfData = (PBFixtureData*)fixData->data;
        for (auto pCollision : pbfData->pCollisions)
        {
            if (pCollision->portal == portal)
            {
                if (pCollision->status > -3 && pCollision->status < 0)
                {
                    return;
                }
                else
                {
                    break;
                }
            }
        }
    }

    pWorld->destroyBodies.insert(this);
}

void PortalBody::handleOut(b2Fixture* fix, Portal* portal, PortalCollisionType out){
    switch (out)
    {
    case NONE_COLLIDING_0:
    case NONE_COLLIDING_1:
        if (shouldCreate(fix, portal, out))
        {
            int side = (uint32_t)out & ODD_MASK ? 1 : 0;
            createDestroyMap[side][portal]++;
        }
    case RELEASE_COLLIDING_0:
    case RELEASE_COLLIDING_1:
        outHelper(fix, portal, 1, out);
        break;
    
    case COLLIDING_RELEASE_0:
        outHelper(fix, portal, -1, out);
        break;
    case COLLIDING_RELEASE_1:
        outHelper(fix, portal, -1, out);
        break;

    case E_COLLIDING_RELEASE_0:
    case E_COLLIDING_RELEASE_1:
        releaseOut2(fix, portal);
        break;

    case COLLIDING_NONE_0:
    case COLLIDING_NONE_1:
        destroyCheck(fix, portal, out);
        break;

    default:
        break;
    }
}

void PortalBody::destroyCheck(b2Fixture* f, Portal* portal, PortalCollisionType out){
    int side = (uint32_t)out & ODD_MASK ? 1 : 0;
    outFixtures[side][portal]++;

    if (outFixtures[side][portal] == numFixtures)
    {
        createDestroyMap[side][portal]--;
    }
}

b2Vec2 PortalBody::getWorldCenterOfMass()
{
    b2Vec2 localCenter;
    if (isMirrored)
    {
        localCenter = worldIndex->mirroredMassCenter;
    }
    else
    {
        localCenter = worldIndex->massCenter;
    }

    return body->GetWorldPoint(localCenter);
}

void PortalBody::getBodyOut(pbWorldData* worldData)
{
    PortalBody* leftOutBody;
    Portal* outPortal;
    int side;

    PortalBody* pb = worldData->pbIndices[0];

    if (pb->bodyMaps.size() == 0) return;

    bodyCollisionStatus* bcs = pb->bodyMaps.at(0);

    b2Vec2 center = pb->getWorldCenterOfMass();
    if (bcs->connection->portal1->getPointSide(center) == bcs->connection->side1)
    {
        leftOutBody = pb;
        outPortal = bcs->connection->portal1;
        side = bcs->connection->side1;
    }
    else
    {
        leftOutBody = bcs->body;
        outPortal = bcs->connection->portal2;
        side = bcs->connection->side2;
    }

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

    for (b2Fixture* fix : leftOutBody->fixtures)
    {
        fixtureData* fixData = (fixtureData*)fix->GetUserData().pointer;
        PBFixtureData* pbfData = (PBFixtureData*)fixData->data;
        pbfData->pCollisions.clear();
    }

    for (int j = worldData->pbIndices.size() - 1; j > -1; j--)
    {
        PortalBody* deleteStat = worldData->pbIndices.at(j);

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
    leftOutBody->bodyMaps.clear();
}

float PortalBody::findPolygonArea(b2Vec2* vertices, int numVertices){
    float baseArea = 0.0f;
    for (int i = 0; i < numVertices; i++){
        baseArea += vertices[i].x * vertices[(i + 1) % numVertices].y;
        baseArea -= vertices[(i + 1) % numVertices].x * vertices[i].y;
    }
    baseArea *= 0.5f;

    return baseArea;
}

b2Vec2 PortalBody::findPolygonCentroid(b2Vec2* vertices, int numVertices){
    b2Vec2 off = vertices[0];
    float twicearea = 0;
    float x = 0;
    float y = 0;
    b2Vec2 p1, p2;
    float f;
    for (int i = 0, j = numVertices - 1; i < numVertices; j = i++) {
        p1 = vertices[i];
        p2 = vertices[j];
        f = (p1.x - off.x) * (p2.y - off.y) - (p2.x - off.x) * (p1.y - off.y);
        twicearea += f;
        x += (p1.x + p2.x - 2 * off.x) * f;
        y += (p1.y + p2.y - 2 * off.y) * f;
    }

    f = twicearea * 3;

    float rv1 = x / f + off.x;
    float rv2 = y / f + off.y;

    return b2Vec2(rv1, rv2);
}

void PortalBody::applyPartialGravity(b2Fixture* fixture){
    fixtureData* fixData = (fixtureData*)fixture->GetUserData().pointer;
    PBFixtureData* pbfData = (PBFixtureData*)fixData->data;
    auto fixColls = pbfData->pCollisions;
    if (fixColls.size() == 0){
        b2Vec2 center;
        float area;
        if (fixture->GetType() == b2Shape::Type::e_circle){
            b2CircleShape* cShape = (b2CircleShape*)fixture->GetShape();
            center = body->GetWorldPoint(cShape->m_p);
            area = b2_pi * cShape->m_radius * cShape->m_radius;
        }
        else{
            b2PolygonShape* pShape = (b2PolygonShape*)fixture->GetShape();
            center = body->GetWorldPoint(findPolygonCentroid(pShape->m_vertices, pShape->m_count));
            area = findPolygonArea(pShape->m_vertices, pShape->m_count);
        }

        b2Vec2 force = area * fixture->GetDensity() * pWorld->GetGravity();
        body->ApplyForce(worldIndex->gravityScale * force, center, false);

        return;
    }

    b2RayCastInput rayInp;
    rayInp.maxFraction = 1.0f;

    b2RayCastOutput rayOut;

    b2Vec2 hitPoint1;
    b2Vec2 hitPoint2;

    bool hit;

    float baseArea;
    float loopArea;
    b2Vec2 center;
    b2Vec2 loopCenterOfMass;
    
    if (fixture->GetType() == b2Shape::Type::e_circle){
        b2CircleShape* cShape = (b2CircleShape*)fixture->GetShape();
        baseArea = b2_pi * cShape->m_radius * cShape->m_radius;
        loopArea = baseArea;
        center = fixture->GetBody()->GetWorldPoint(cShape->m_p);
        loopCenterOfMass = center;

        for (portalCollision* pColl : fixColls){
            if (pColl->status < 0){
                return;
            }
            int side = pColl->portal->getFixtureSide(fixture);
            bool mostlyOut = side != pColl->side;

            rayInp.p1 = pColl->portal->points[0];
            rayInp.p2 = pColl->portal->points[1];
            hit = fixture->RayCast(&rayOut, rayInp, b2_dynamicBody);
            if (!hit){
                if (mostlyOut){
                    loopArea = 0.0f;
                    break;
                }
                else{
                    continue;
                }
            }
            hitPoint1 = Portal::getRayPoint(rayInp, rayOut);

            rayInp.p1 = pColl->portal->points[1];
            rayInp.p2 = pColl->portal->points[0];
            hit = fixture->RayCast(&rayOut, rayInp, b2_dynamicBody);
            if (!hit){
                if (mostlyOut){
                    loopArea = 0.0f;
                    break;
                }
                else{
                    continue;
                }
            }
            hitPoint2 = Portal::getRayPoint(rayInp, rayOut);

            b2Vec2 cMid = 0.5f * (hitPoint1 + hitPoint2);
            b2Vec2 centerToMid = cMid - center;
            centerToMid.Normalize();

            float cDist = (hitPoint2 - hitPoint1).Length();

            float angle = acos((cDist * cDist - (cShape->m_radius * cShape->m_radius * 2)) / -(cShape->m_radius * cShape->m_radius * 2));
            float outerArea = cShape->m_radius * cShape->m_radius * 0.5f * (angle - sin(angle));
            float x = (4 * cShape->m_radius * pow(sin(angle * 0.5f), 3)) / (3 * (angle - sin(angle)));
            float x2;
            b2Vec2 centerOfMass = center + x * centerToMid;
            b2Vec2 loopCenterOfMassTocenterOfMass;
            if (mostlyOut){
                x = (x * outerArea) / (loopArea - outerArea);
                centerOfMass = center - x * centerToMid;
                outerArea = baseArea - outerArea;
                x2 = -(x * outerArea) / (loopArea - outerArea);
                loopCenterOfMassTocenterOfMass = loopCenterOfMass - centerOfMass;
            }
            else{
                x2 = (x * outerArea) / (loopArea - outerArea);
                loopCenterOfMassTocenterOfMass = centerOfMass - loopCenterOfMass;
            }
            loopCenterOfMassTocenterOfMass.Normalize();

            loopArea -= outerArea;

            loopCenterOfMass -= x2 * loopCenterOfMassTocenterOfMass;
        }

        b2Vec2 force = loopArea * fixture->GetDensity() * pWorld->GetGravity();

        // validity check for loopCenterOfMass is probably extra
        // just to be sure program does not crash
        if (loopArea > 0.0f && b2IsValid(loopCenterOfMass.x) && b2IsValid(loopCenterOfMass.y)){
            body->ApplyForce(worldIndex->gravityScale* ((float)bodyMaps.size() + 1) * force, loopCenterOfMass, false);
        }
    }
    else{
        // every polygon cut may create 1 additional vertex and maximum number of vertices
        // a polygon has is b2_maxPolygonVertices which is 8, so we need a polygon to intersect
        // with 33 portals, which is impossible with 8 vertices. So, 41 is just an arbitary number
        b2Vec2 innerVertices[41];
        b2Vec2 latestVertices[41];
        b2PolygonShape* pShape = (b2PolygonShape*)fixture->GetShape();
        for (int i = 0; i < pShape->m_count; i++){
            latestVertices[i] = body->GetWorldPoint(pShape->m_vertices[i]);
        }
        int numLatestVertices = pShape->m_count;
        int numVertices = 0;
        baseArea = findPolygonArea(pShape->m_vertices, pShape->m_count);
        loopArea = baseArea;
        center = findPolygonCentroid(pShape->m_vertices, pShape->m_count);
        loopCenterOfMass = center;
        for (portalCollision* pColl : fixColls){
            if (pColl->status < 0){
                return;
            }
            b2Vec2 lastVertex = latestVertices[0];
            int lastSide = pColl->portal->getPointSide(lastVertex);
            bool isInside = lastSide == pColl->side;
            for (int i = 1; i < numLatestVertices + 1; i++){
                int j = i % numLatestVertices;

                b2Vec2 vertex = latestVertices[j];
                int side = pColl->portal->getPointSide(vertex);

                // inside to outside transition
                if (side != lastSide){
                    b2Vec2 intersectionPoint = PortalWorld::getLineIntersection(lastVertex, vertex, pColl->portal->points[0], pColl->portal->points[1]);
                    if (isInside){
                        innerVertices[numVertices++] = lastVertex;
                        innerVertices[numVertices++] = intersectionPoint;
                        isInside = false;
                    }
                    else{
                        innerVertices[numVertices++] = intersectionPoint;
                    }
                }
                else if (isInside){
                    innerVertices[numVertices++] = lastVertex;
                }
                
                isInside = side == pColl->side;
                lastVertex = vertex;
                lastSide = side;
            }

            memcpy(latestVertices, innerVertices, sizeof(b2Vec2) * numVertices);

            numLatestVertices = numVertices;
            numVertices = 0;
        }

        if (numLatestVertices > 2){
            loopArea = findPolygonArea(latestVertices, numLatestVertices);
            if (loopArea > 0.0f){
                loopCenterOfMass = findPolygonCentroid(latestVertices, numLatestVertices);

                b2Vec2 force = loopArea * fixture->GetDensity() * pWorld->GetGravity();
                if (loopArea > 0.0f && b2IsValid(loopCenterOfMass.x) && b2IsValid(loopCenterOfMass.y)){
                    body->ApplyForce(worldIndex->gravityScale * ((float)bodyMaps.size() + 1) * force, loopCenterOfMass, false);
                }
            }
        }
    }
}

float PortalBody::calculateAngleThold(b2Fixture* fix){
    const float defaultAngleThold = 0.0001f;
    if (fix->GetType() == b2Shape::e_circle){
        return defaultAngleThold;
    }
    
    float maxLength = 0.0f;
    b2PolygonShape* pShape = (b2PolygonShape*)fix->GetShape();
    for (int i = 0; i < pShape->m_count; i++){
        int j = (i + 1) % pShape->m_count;
        float length = (pShape->m_vertices[i] - pShape->m_vertices[j]).Length();
        if (length > maxLength){
            maxLength = length;
        }
    }

    return atan2(b2_polygonRadius, maxLength);
}