#include "PortalBody.h"
#include "Portal.h"
#include "PortalWorld.h"
#include "glm/common.hpp"
#include <DebugDrawer.h>

#define ODD_MASK 0b10101010101010101010101010101010

PortalBody::PortalBody(b2Body* bBody, PortalWorld* pWorld){
    this->pWorld = pWorld;
    this->body = bBody;
    this->bodyMaps = new std::vector<bodyCollisionStatus*>();

    this->body->SetGravityScale(0.0f);
    this->offsetAngle = 0;

    bodyData* bd = (bodyData*)malloc(sizeof(bodyData));
    bd->data = this;
    bd->type = PORTAL_BODY;
    bd->extraData = nullptr;
    body->GetUserData().pointer = (uintptr_t)bd;

    isMirrored = false;

    this->numFixtures = 0;
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        fixtures.push_back(fix);
        fixtureData* fixData = (fixtureData*)calloc(1, sizeof(fixtureData));
        fixData->index = numFixtures;
        fix->GetUserData().pointer = (uintptr_t)fixData;
        numFixtures++;
    }
}

PortalBody::~PortalBody(){
    pWorld->handledBodies.insert(worldIndex);
    for (int i = 0; i < worldIndex->size(); i++){
        if (worldIndex->at(i) == this){
            worldIndex->erase(worldIndex->begin() + i);
            break;
        }
    }
    if (worldIndex->size() == 0){
        delete baseDensities;
    }

    for (int i = 0; i < bodyMaps->size(); i++){
        bodyCollisionStatus* bodyCollStatus = bodyMaps->at(i);
		for (int j = 0; j < bodyCollStatus->body->bodyMaps->size(); j++) {
			bodyCollisionStatus* bcs = bodyCollStatus->body->bodyMaps->at(j);
			if (bcs->body == this) {
				bodyCollStatus->body->bodyMaps->erase(bodyCollStatus->body->bodyMaps->begin() + j);
				j--;
				delete bcs;
			}
		}
        delete bodyCollStatus;
    }
    delete bodyMaps;

    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){  
        fixtureData* fixData = (fixtureData*)fix->GetUserData().pointer;      
        for (portalCollision* coll : fixData->pCollisions){
            free(coll);
        }
        delete fixData;
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

void PortalBody::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    
    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, bData);
    if (!shouldCollide){
        contact->SetEnabled(false);
        return;
    }

    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        PortalCollisionType out = p->collisionBegin(contact, fix2, fix1);
        handleOut(fix1, p, out);
    }
}

void PortalBody::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    if (!pWorld->IsLocked()){
        return;
    }
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    
    // may not be needed, so it should stay like this for a while for testing
    // bool shouldCollide = this->shouldCollide(contact, fix1, fix2, bData);
    // if (!shouldCollide){
    //     return;
    // }

    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        PortalCollisionType out = p->collisionEnd(contact, fix2, fix1);
        handleOut(fix1, p, out);
    }
}

void PortalBody::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;

    if (!contact->IsEnabled()){
        return;
    }

    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, bData);
    if (!shouldCollide){
        contact->SetEnabled(false);
        return;
    }

    if (bData && bData->type == PORTAL){
        Portal* portal = (Portal*)bData->data;
        PortalCollisionType out = portal->preCollision(contact, fix2, fix1);
        handleOut(fix1, portal, out);
    }
}

bool PortalBody::shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, bodyData* bData){
    fixtureData* fixData1 = (fixtureData*)fix1->GetUserData().pointer;
    std::vector<portalCollision*> collidingPortals1 = fixData1->pCollisions;

    if (bData && bData->type == PORTAL){
        for (portalCollision* coll1 : collidingPortals1){
            if (fix2 == coll1->portal->yFix[0] || fix2 == coll1->portal->yFix[1]) return true;
        }
        Portal* portal = (Portal*)bData->data;
        if (fix2 != portal->yFix[0] && fix2 != portal->yFix[1]){
            for (bodyCollisionStatus* stat : *bodyMaps){
                if (stat->body->bodyMaps->size() < 2){
                    continue;
                }
                b2Fixture* loopFix = stat->body->fixtures[fixData1->index];
                fixtureData* fixDataLoop = (fixtureData*)loopFix->GetUserData().pointer;
                for (portalCollision* pc : fixDataLoop->pCollisions){
                    if (pc->portal != stat->connection->portal2){
                        if (pc->status < 0){
                            return false;
                        }
                        else if (pc->status == 1){
                            b2WorldManifold wManifold;
                            contact->GetWorldManifold(&wManifold);
                            b2Vec2 point;
                            if (contact->GetManifold()->pointCount == 1){
                                point = wManifold.points[0];
                            }
                            else{
                                point = 0.5f * (wManifold.points[0] + wManifold.points[1]);
                            }
                            
                            b2Vec2 intersectionPoint = PortalWorld::getLineIntersection(
                                point - wManifold.normal, point + wManifold.normal,
                                portal->points[0], portal->points[1]);

                            intersectionPoint = PortalWorld::getRelativePortalPosition(intersectionPoint, stat->connection);
                            int intersectSide = pc->portal->getPointSide(intersectionPoint);
                            if (intersectSide != pc->side){
                                return false;
                            }
                        }
                    }
                }
            }
        }

        for (auto pColl : collidingPortals1){
            if (pColl->portal == portal) return true;
        }
    }
    else if (collidingPortals1.size() == 0){
        return true;
    }

    std::vector<portalCollision*> collidingPortals2;
    fixtureData* fixData2 = (fixtureData*)fix2->GetUserData().pointer;
    if (bData && bData->type == PORTAL_BODY){
        collidingPortals2 = fixData2->pCollisions;
    }
    
    for (portalCollision* coll1 : collidingPortals1){
        if (bData && bData->type == PORTAL_BODY){
            int val = shouldCollideHelper(coll1, fixData1, fixData2, (PortalBody*)bData->data);
            if (val == 1){
                continue;
            }
            else if (val == 2){
                return false;
            }
        }

        int status = 2;
        if (bData && bData->type == PORTAL_BODY) status = fixData2->getPortalStatus(coll1->portal, 1 ^ coll1->side);
        if (coll1->status < 0 || status == 1){
            return false;
        }

        bool didJob = false;
        if (collidingPortals2.size() > 0){
            for (portalCollision* coll2 : collidingPortals2){
                if (fixData1->getPortalCollision(coll2->portal) != fixData1->pCollisions.end()){
                    continue;
                }
                int val = shouldCollideHelper(coll2, fixData2, fixData1, this);
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

    // if (fix2->GetBody()->GetType() == b2_dynamicBody && !bData){
    //     extern int isPaused;
    //     b2DistanceOutput out;
    //     b2DistanceProxy distanceProxyA;
    //     b2DistanceProxy distanceProxyB;
    //     distanceProxyA.Set(fix1->GetShape(), b2_dynamicBody);
    //     distanceProxyB.Set(fix2->GetShape(), fix2->GetType());

    //     b2DistanceInput input;
    //     input.proxyA = distanceProxyA;
    //     input.proxyB = distanceProxyB;
    //     input.transformA = fix1->GetBody()->GetTransform();
    //     input.transformB = fix2->GetBody()->GetTransform();
    //     // finds single collision between fixtures
    //     // not suitable for this problem, we need 2 collision points at the outer edge of the fixtures
    //     input.useRadii = false;

    //     // may be used in the future
    //     b2SimplexCache cache;
    //     cache.count = 0;

    //     b2Distance(&out, &cache, &input);

    //     for (int i = 0; i < ((b2PolygonShape*)fix1->GetShape())->m_count; i++){
    //         b2Vec2 pos = fix1->GetBody()->GetWorldPoint(((b2PolygonShape*)fix1->GetShape())->m_vertices[i]);
    //         ((DebugDrawer*)pWorld->getDebugDraw())->addPointToQueue(pos, 5.0f, b2Color(1, 1, 0, 1));
    //     }

    //     for (int i = 0; i < ((b2PolygonShape*)fix2->GetShape())->m_count; i++){
    //         b2Vec2 pos = fix2->GetBody()->GetWorldPoint(((b2PolygonShape*)fix2->GetShape())->m_vertices[i]);
    //         ((DebugDrawer*)pWorld->getDebugDraw())->addPointToQueue(pos, 5.0f, b2Color(1, 0, 1, 1));
    //     }

    //     ((DebugDrawer*)pWorld->getDebugDraw())->addPointToQueue(out.pointA, 8.0f, b2Color(1, 1, 1, 1));
    //     ((DebugDrawer*)pWorld->getDebugDraw())->addPointToQueue(out.pointB, 8.0f, b2Color(1, 1, 1, 1));

    //     ((DebugDrawer*)pWorld->getDebugDraw())->addPointToQueue(Portal::getFixtureCenter(fix1), 10.0f, b2Color(1, 0, 0, 1));
    //     ((DebugDrawer*)pWorld->getDebugDraw())->addPointToQueue(Portal::getFixtureCenter(fix2), 10.0f, b2Color(0, 1, 1, 1));
    //     isPaused = true;
    // }

    return true;
}

int PortalBody::shouldCollideHelper(portalCollision* coll, fixtureData* fixData1, fixtureData* fixData2, PortalBody* pb){
    int status1 = fixData1->getPortalStatus(coll->portal, coll->side);
    int status2 = fixData2->getPortalStatus(coll->portal, coll->side);
    if (status1 == 1 && status2 == 1){

    }
    else{
        return 0;
    }

    b2Fixture* otherFix2 = NULL;
    for (bodyCollisionStatus* bcs : *pb->bodyMaps){
        if (bcs->connection->portal1 == coll->portal){
            otherFix2 = bcs->body->fixtures[fixData2->index];
        }
    }

    if (otherFix2 == NULL){
        return 1;
    }
    
    for (bodyCollisionStatus* bcs : *bodyMaps){
        if (bcs->body->bodyMaps->size() < 2){
            continue;
        }

        b2Fixture* otherFix1 = bcs->body->fixtures[fixData1->index];

        fixtureData* loopFixData = (fixtureData*)otherFix1->GetUserData().pointer;
        for (portalCollision* pc : loopFixData->pCollisions){
            if (pc->portal == bcs->connection->portal1 || pc->portal == bcs->connection->portal2){
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

void PortalBody::outHelper(b2Fixture* fix, Portal* portal, int status, PortalCollisionType out){
    int side = (uint32_t)out & ODD_MASK ? 1 : 0;
    if (status < 0){
        outFixtures[side][portal]--;
    }
    else{
        if (out & (RELEASE_COLLIDING_0 | RELEASE_COLLIDING_1)){
            outFixtures[side][portal]++;
        }
        else if (out & (NONE_COLLIDING_0 | NONE_COLLIDING_1)){
            if (outFixtures[side].find(portal) == outFixtures[side].end()){
                outFixtures[side][portal] = numFixtures - 1;
            }
            else{
                outFixtures[side][portal]--;
            }
        }
    }

    // Extra void portal logic
    if (portal->isVoid[side] && status < 0){
#if 1
        if (outFixtures[side][portal] == -numFixtures){
            // pWorld->destroyBodies.insert(this);
            // return;
        }
#else
        bool found = false;
        for (b2Fixture* f = fix->GetBody()->GetFixtureList(); f; f = f->GetNext()){
            if (f == fix) continue;
            if (portal->releaseFixtures[side].find(f) == portal->releaseFixtures[side].end())
            {
                found = true;
                f = nullptr;
                break;
            }
        }
        if (!found){
            pWorld->destroyBodies.insert(this);
            return;
        }
#endif
    }
}

void PortalBody::postHandle(){
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        applyPartialGravity(fix);
    }

    for (bodyStruct* s : createBodies){
        pWorld->createCloneBody(s);
        free(s);
    }
    createBodies.clear();
}

bool PortalBody::shouldCreate(b2Body* bBody, Portal* portal, PortalCollisionType out){
    int side = (uint32_t)out & ODD_MASK ? 1 : 0;
    if (portal->isVoid[side]) return false;

    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal){
            const auto iter = pWorld->destroyBodies.find(s->body);
            if (iter != pWorld->destroyBodies.end()){
                pWorld->destroyBodies.erase(iter);
            }
        }
    }
    // Experimental
#if 0
    const auto iter = outFixtures[side].find(portal);
    if (iter != outFixtures[side].end() &&
        iter->second != numFixtures){
        return false;
    }
#else
    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal && s->connection->side1 == side)
            return false;
    }
#endif

    for (bodyStruct* bs : createBodies){
        if (bs->collPortal == portal && bs->side == side)
            return false;
    }

    return true;
}

void PortalBody::releaseOut2(b2Fixture* fix1, Portal* portal){
    // TODO : optimize this
    b2Body* body = fix1->GetBody();
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        fixtureData* fixData = (fixtureData*)fix->GetUserData().pointer;
        for (auto pCollision : fixData->pCollisions){
            if (pCollision->portal == portal){
                if (pCollision->status > -3 && pCollision->status < 0){
                    return;
                }
                else{
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
    case RELEASE_COLLIDING_0:
    case RELEASE_COLLIDING_1:
        if (shouldCreate(fix->GetBody(), portal, out)){
            int side = (uint32_t)out & ODD_MASK ? 1 : 0;
            bodyStruct* s = (bodyStruct*)malloc(sizeof(bodyStruct));
            *s = {this, portal, side};
            createBodies.push_back(s);
        }
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

// Experimental
// Disable it if it is causing problems
#if 1
    if (outFixtures[side][portal] == numFixtures){
        for (bodyCollisionStatus* s : *bodyMaps){
            if (s->connection->portal1 == portal && s->connection->side1 == side){
                pWorld->destroyBodies.insert(s->body);
            }
        }
    }
#else
    for (b2Fixture* fix = f->GetBody()->GetFixtureList(); fix; fix = fix->GetNext()){
        if (fix == f) continue;
        for (portalCollision* coll : *fixtureCollisions[fix]){
            if (coll->portal == portal){
                auto iter = portal->releaseFixtures[coll->side].find(fix);
                if (iter == portal->releaseFixtures[coll->side].end()){
                    return;
                }
            }
        }
    }

    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal){
            pWorld->destroyBodies.insert(s->body);
        }
    }
#endif
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

// may add an option to enable advanced gravity since it is relatively expensive to calculate it
// other easy to implement option may check for side of the center of mass and apply gravity to
// the one that is outside the collision side (same side it entered)
void PortalBody::applyPartialGravity(b2Fixture* fixture){
    fixtureData* fixData = (fixtureData*)fixture->GetUserData().pointer;
    auto fixColls = fixData->pCollisions;
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
        body->ApplyForce(force, center, false);

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
            body->ApplyForce(((float)bodyMaps->size() + 1) * force, loopCenterOfMass, false);
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
                    body->ApplyForce(((float)bodyMaps->size() + 1) * force, loopCenterOfMass, false);
                }
            }
        }
    }
}