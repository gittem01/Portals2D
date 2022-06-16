#include "PortalWorld.h"
#include "PortalBody.h"
#include "Portal.h"
#include <RotationJoint.h>

PortalWorld::PortalWorld(DebugDrawer* drawer) : b2World(b2Vec2(0, -10)){
    this->drawer = drawer;

    this->drawReleases = false;
    this->releaseColor = b2Color(1.0f, 1.0f, 1.0f, 0.2f);

    this->rayHandler = new PortalRay(this);
}

Portal* PortalWorld::createPortal(b2Vec2 pos, b2Vec2 dir, float size){
    Portal* portal = new Portal(pos, dir, size, this);
    portals.push_back(portal);

    return portal;
}

PortalBody* PortalWorld::createPortalBody(b2Body* body, b2Color bodyColor){
    PortalBody* pBody = new PortalBody(body, this, bodyColor);
    portalBodies.push_back(pBody);

    createPortalBody_i(pBody, NULL, true);

    return pBody;
}

void PortalWorld::createPortalBody_i(PortalBody* pBody, PortalBody* baseBody, bool isNew){
    if (isNew){
        std::vector<PortalBody*>* pbIndices = new std::vector<PortalBody*>();
        pbIndices->push_back(pBody);
        bodyIndices.push_back(pbIndices);
        pBody->worldIndex = pbIndices;
    }
    else{
        baseBody->worldIndex->push_back(pBody);
        pBody->worldIndex = baseBody->worldIndex;
    }
}

b2Vec2 rotateVec(b2Vec2 vec, float angle){
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return {x, y};
}

bool PortalRay::portalOutCheck(Portal* portal, b2Vec2 rayStart, b2Vec2 dirVec){
    int raySide = portal->getPointSide(rayStart + dirVec);

    b2RayCastInput input1;
    b2RayCastInput input2;
    b2RayCastOutput raycastOutput1;
    b2RayCastOutput raycastOutput2;

    input1.p1 = rayStart - 100.0f * dirVec;
    input1.p2 = rayStart + 100.0f * dirVec;
    input1.maxFraction = 1.0f;
    input2.p1 = rayStart + 100.0f * dirVec;
    input2.p2 = rayStart - 100.0f * dirVec;
    input2.maxFraction = 1.0f;

    for (b2Fixture* fix : portal->collidingFixtures[raySide]){
        bool res1 = fix->RayCast(&raycastOutput1, input1, b2_dynamicBody);
        bool res2 = fix->RayCast(&raycastOutput2, input2, b2_dynamicBody);

        if (res1){
            b2Vec2 collPos1 = raycastOutput1.fraction * (input1.p2 - input1.p1) + input1.p1;
            b2Vec2 collPos2 = raycastOutput2.fraction * (input2.p2 - input2.p1) + input2.p1;

            float dist1 = pWorld->getDist(portal->points[raySide], portal->points[raySide ^ 1], collPos1);
            float dist2 = pWorld->getDist(portal->points[raySide], portal->points[raySide ^ 1], collPos2);

            if (dist1 < +0.05f && dist2 > -0.0f){
                return false;
            }
        }
    }

    return true;
}

void PortalRay::prepareRaySend(){
    for (rayData* rd : rayResult)
    {
        delete rd;
    }
    rayResult.clear();
}

void PortalRay::sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, int rayIndex, Portal* rayOutPortal){
    prepareRaySend();

    sendRay_i(rayStart, dirVec, rayLength, rayIndex, rayOutPortal);
}

void PortalRay::sendRay_i(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, int rayIndex, Portal* rayOutPortal){
    if (rayIndex >= maxRayCount || rayLength < 0.0001f){
        return;
    }
    pWorld->normalize(&dirVec);
    b2Vec2 rayEnd = rayStart + rayLength * dirVec;
    b2Vec2 diff = rayEnd - rayStart;

    reset();

    if (rayOutPortal && !portalOutCheck(rayOutPortal, rayStart, dirVec)){
        rayData* rd = new rayData;
        rd->endPos = rayStart;
        rd->dir = dirVec;

        rayResult.push_back(rd);
        return;
    }

    pWorld->RayCast(this, rayStart, rayEnd);

    endHandle();

    float minFrac = minFraction;

    diff = minFrac * diff;
    b2Vec2 collPoint = rayStart + diff;

    pWorld->drawer->DrawSegment(rayStart, collPoint, b2Color(1, 0, 0));

    if (closestFixture != NULL){
        if (closestFixture == closestPortalFixture){
            bodyData* bd = (bodyData*)closestFixture->GetBody()->GetUserData().pointer;
            Portal* p = (Portal*)bd->data;
            b2Vec2 posDiff = p->pos - collPoint;
            int raySide = p->getPointSide(rayStart);
            b2Vec2 p1Dir = raySide ? -p->dir : p->dir;

            for (portalConnection* conn : p->connections[raySide]){
                b2Vec2 p2Dir = conn->side2 ? -conn->portal2->dir : conn->portal2->dir;

                float angleRot = -pWorld->calcAngle2(p1Dir) + pWorld->calcAngle2(-p2Dir);

                b2Vec2 localPos = rotateVec(posDiff, angleRot + b2_pi);
                if (conn->isReversed){
                    localPos = pWorld->mirror(p2Dir, localPos);
                }

                b2Vec2 ray2Pos = conn->portal2->pos + localPos;

                b2Vec2 dir2 = rotateVec(dirVec, angleRot);

                sendRay_i(ray2Pos, dir2, rayLength - minFrac * rayLength, rayIndex + 1, conn->portal2);
            }
        }
        else{
            rayData* rd = new rayData;
            rd->endPos = collPoint;
            rd->dir = dirVec;

            rayResult.push_back(rd);
        }
    }
}

void PortalWorld::portalUpdate(){
    for (int i = 0; i < portalBodies.size(); i++){
        PortalBody* body = portalBodies.at(i);
        body->postHandle();
    }

    for (Portal* p : portals) {
        p->postHandle();
    }

    rayHandler->sendRay(b2Vec2(-10.0f, 10.0f), b2Vec2(1, -1), 15.0f);

    glLineWidth(3.0f);
    for (rayData* rd : rayHandler->rayResult){
        drawer->DrawArrow(rd->endPos - rd->dir, rd->endPos, b2Color(1, 1, 0));
    }
    glLineWidth(1.0f);

    globalPostHandle();
}

void PortalWorld::drawUpdate(){
    for (PortalBody* body : portalBodies){
        body->drawBodies();
    }
    for (Portal* p : portals) {
        p->draw();
    }
}

void PortalWorld::globalPostHandle(){
    for (PortalBody* b : destroyBodies){
        for (int i = 0 ; i < portalBodies.size(); i++){
            if (portalBodies.at(i) == b){
                portalBodies.erase(portalBodies.begin() + i);
                break;
            }
        }
        delete b;
    }

    destroyBodies.clear();
}

std::vector<PortalBody*> PortalWorld::createCloneBody(bodyStruct* s){
    PortalBody* pBody = s->pBody;
    int side = s->side;
    Portal* collPortal = s->collPortal;
    b2Body* body1 = s->pBody->body;
    b2Vec2 dir1 = side ? -collPortal->dir : collPortal->dir;
    b2Vec2 posDiff = collPortal->pos - body1->GetPosition();
    b2Vec2 speed = body1->GetLinearVelocity();

    std::vector<PortalBody*> retBodies;

    for (portalConnection* c : collPortal->connections[side]){
        Portal* portal2 = c->portal2;
        
        b2Vec2 dir2 = c->side2 ? -portal2->dir : portal2->dir;
        float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);

        b2Vec2 localPos = rotateVec(posDiff, angleRot + b2_pi);
        if (c->isReversed)
            localPos = mirror(portal2->dir, localPos);

        b2BodyDef def;
        def.type = b2_dynamicBody;
        def.position = portal2->pos + localPos;
        def.angle = body1->GetTransform().q.GetAngle();
        def.fixedRotation = body1->IsFixedRotation();
        def.linearDamping = body1->GetLinearDamping();
        def.angularDamping = body1->GetAngularDamping();
        def.bullet = body1->IsBullet();
        def.allowSleep = body1->IsSleepingAllowed();
        def.gravityScale = 0.0f;

        b2Body* body2 = CreateBody(&def);
        PortalBody* npb = new PortalBody(body2, this, pBody->bodyColor);

        if (c->isReversed){
            b2Vec2 v = rotateVec(b2Vec2(1, 0), angleRot + body2->GetAngle());
            float ang1 = calcAngle2(v);
            v = mirror(portal2->dir, v);
            float ang2 = calcAngle2(v);
            v = rotateVec(v, -body2->GetAngle());
            npb->offsetAngle = ang2 - ang1 + angleRot - b2_pi - pBody->offsetAngle;
        }
        else{
            npb->offsetAngle = angleRot + pBody->offsetAngle;
        }
        if (npb->offsetAngle > b2_pi){
            npb->offsetAngle -= 2 * b2_pi;
        }
        else{
            npb->offsetAngle += 2 * b2_pi;
        }

        portalBodies.push_back(npb);

        retBodies.push_back(npb);

        bodyCollisionStatus* bcs = new bodyCollisionStatus;
        bcs->body = pBody;
        npb->bodyMaps->push_back(bcs);

        for (portalConnection* conn : portal2->connections[c->side2]){
            if (conn->portal2 == collPortal && conn->side2 == c->side1){
                bcs->connection = conn;
                break;
            }
        }

        bodyCollisionStatus* t_bcs = new bodyCollisionStatus;
        t_bcs->body = npb;
        pBody->bodyMaps->push_back(t_bcs);
        t_bcs->connection = c;
        b2Vec2 lvToSet = rotateVec(speed, angleRot);
        if (c->isReversed){
            body2->SetAngularVelocity(-body1->GetAngularVelocity());
            lvToSet = mirror(dir2, lvToSet);
        }
        else{
            body2->SetAngularVelocity(body1->GetAngularVelocity());
        }
        body2->SetLinearVelocity(lvToSet);

        bool allOut = true;
        for (b2Fixture* fix = body1->GetFixtureList(); fix; fix = fix->GetNext()){
            b2Fixture* f;
            if (fix->GetType() == b2Shape::Type::e_polygon){
                b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();
                b2FixtureDef fDef;
                fDef.density = fix->GetDensity();
                fDef.restitution = fix->GetRestitution();
                fDef.friction = fix->GetFriction();
                b2PolygonShape newShape;
                fDef.shape = &newShape;

                newShape.m_count = polyShape->m_count;
                b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2) * polyShape->m_count);

                for (int i = 0; i < polyShape->m_count; i++){
                    if (c->isReversed){
                        b2Vec2 v = rotateVec(polyShape->m_vertices[i], angleRot + body2->GetAngle());
                        v = mirror(portal2->dir, v);
                        v = rotateVec(v, -body2->GetAngle());
                        vertices[i] = v;
                    }
                    else{
                        vertices[i] = rotateVec(polyShape->m_vertices[i], angleRot);
                    }
                }
                newShape.Set(vertices, polyShape->m_count);

                f = body2->CreateFixture(&fDef);
                free(vertices);
            }
            else{
                b2CircleShape* circleShape = (b2CircleShape*)fix->GetShape();
                b2FixtureDef fDef;
                fDef.density = fix->GetDensity();
                fDef.restitution = fix->GetRestitution();
                fDef.friction = fix->GetFriction();
                b2CircleShape newShape;
                fDef.shape = &newShape;

                newShape.m_radius = circleShape->m_radius;

                b2Vec2 v;
                if (c->isReversed){
                    v = rotateVec(circleShape->m_p, angleRot + body2->GetAngle());
                    v = mirror(portal2->dir, v);
                    v = rotateVec(v, -body2->GetAngle());
                }
                else{
                    v = rotateVec(circleShape->m_p, angleRot);
                }
                newShape.m_p = v;
                
                f = body2->CreateFixture(&fDef);
            }

            npb->fixtureCollisions[f] = new std::set<portalCollision*>();
            npb->allParts[f] = new std::vector<std::vector<b2Vec2>*>();

            for (Portal* portal : portals){
                std::vector<b2Vec2> ps = portal->getCollisionPoints(portal->collisionSensor, f);
                if (ps.size() > 0) npb->prepareMaps[f].insert(portal);
            }

            portalCollision* col = (portalCollision*)malloc(sizeof(portalCollision));;

            if (collPortal->collidingFixtures[side].find(fix) != collPortal->collidingFixtures[side].end()){
                bool rayCheck;
                int side = collPortal->getFixtureSide(fix);
                
                if (side == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                else rayCheck = 1;

                if (rayCheck){
                    portal2->collidingFixtures[c->side2].insert(f);
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = 1;
                    npb->fixtureCollisions[f]->insert(col);
                    allOut = false;
                }
                else{
                    free(col);
                }
            }
            else if (collPortal->releaseFixtures[side].find(fix) == collPortal->releaseFixtures[side].end()){
                portal2->releaseFixtures[c->side2].insert(f);
                col->portal = portal2;
                col->side = c->side2;
                col->status = 0;
                npb->fixtureCollisions[f]->insert(col);
            }
            else{
                free(col);
            }

            if (collPortal->collidingFixtures[0].find(fix) == collPortal->collidingFixtures[0].end() &&
                collPortal->collidingFixtures[1].find(fix) == collPortal->collidingFixtures[1].end())
            {
                allOut = false;
            }
        }
        createPortalBody_i(npb, pBody, false);
        if (allOut) destroyBodies.insert(pBody);
        else connectBodies(pBody, npb, c, side, calcAngle2(dir1) - calcAngle2(dir2));
    }

    return retBodies;
}

void PortalWorld::connectBodies(PortalBody* body1, PortalBody* body2, portalConnection* connection, int side, float angleRot) {
    b2PrismaticJointDef prismDef;
    prismDef.Initialize(body1->body, body2->body, b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, 0.0f));
    prismDef.collideConnected = true;

    CreateRotationJoint(&prismDef, connection->isReversed, body1, body2, angleRot);

    b2Vec2 dirClone1 = connection->side1 == 0 ? connection->portal1->dir : -connection->portal1->dir;
    b2Vec2 dirClone2 = connection->side2 == 0 ? connection->portal2->dir : -connection->portal2->dir;

    float mult = 100000.0f;

    b2PulleyJointDef pulleyDef;

    b2MassData data1;
    b2MassData data2;
    body1->body->GetMassData(&data1);
    body2->body->GetMassData(&data2);
    
    b2Vec2 center1 = body1->body->GetWorldPoint(data1.center);
    b2Vec2 center2 = body2->body->GetWorldPoint(data2.center);

    b2Vec2 anchor1 = center1;
    b2Vec2 anchor2 = center2;
    b2Vec2 groundAnchor1(dirClone1.x * mult, dirClone1.y * mult);
    b2Vec2 groundAnchor2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1->body, body2->body, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    CreateJoint(&pulleyDef);

    dirClone1 = rotateVec(dirClone1, b2_pi / 2.0f);
    if (connection->isReversed)
        dirClone2 = rotateVec(dirClone2, -b2_pi / 2.0f);
    else
        dirClone2 = rotateVec(dirClone2, b2_pi / 2.0f);

    anchor1 = center1;
    anchor2 = center2;
    groundAnchor1 = b2Vec2(dirClone1.x * mult, dirClone1.y * mult);
    groundAnchor2 = b2Vec2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1->body, body2->body, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    CreateJoint(&pulleyDef);
}

void PortalWorld::CreateRotationJoint(b2PrismaticJointDef* def, bool isReversed, PortalBody* pb1, PortalBody* pb2, float angleRot){
    b2Assert(IsLocked() == false);
	if (IsLocked())
	{
		return;
	}

    void* mem = m_blockAllocator.Allocate(sizeof(RotationJoint));
	RotationJoint* j = new (mem) RotationJoint(def, isReversed, pb1, pb2, angleRot);

	j->m_prev = nullptr;
	j->m_next = m_jointList;
	if (m_jointList)
	{
		((RotationJoint*)m_jointList)->m_prev = j;
	}
	m_jointList = j;
	++m_jointCount;
	
	j->m_edgeA.joint = j;
	j->m_edgeA.other = j->m_bodyB;
	j->m_edgeA.prev = nullptr;
	j->m_edgeA.next = j->m_bodyA->m_jointList;
	if (j->m_bodyA->m_jointList) j->m_bodyA->m_jointList->prev = &j->m_edgeA;
	j->m_bodyA->m_jointList = &j->m_edgeA;

	j->m_edgeB.joint = j;
	j->m_edgeB.other = j->m_bodyA;
	j->m_edgeB.prev = nullptr;
	j->m_edgeB.next = j->m_bodyB->m_jointList;
	if (j->m_bodyB->m_jointList) j->m_bodyB->m_jointList->prev = &j->m_edgeB;
	j->m_bodyB->m_jointList = &j->m_edgeB;
}

bool PortalWorld::isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) >= t;
}

float PortalWorld::vecAngle(b2Vec2 v1, b2Vec2 v2){
    return abs(calcAngle2(v1) - calcAngle2(v2));
}

float PortalWorld::getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x));
}

float PortalWorld::calcAngle2(b2Vec2 vec) {
    float angle = atan2(vec.y, vec.x);
    if (angle < 0) angle += b2_pi * 2.0f;

    return angle;
}

void PortalWorld::normalize(b2Vec2* vec){
    float length = vec->Length();
    if (length == 0) { return; }
    
    vec->x /= length;
    vec->y /= length;
}

b2Vec2 PortalWorld::mirror(b2Vec2 mirror, b2Vec2 vec){
    float angle1 = calcAngle2(mirror);
    float angle2 = calcAngle2(vec);
    return rotateVec(vec, 2 * (angle1 - angle2));
}

PortalRay::PortalRay(PortalWorld* pWorld){
    this->pWorld = pWorld;
}

float PortalRay::ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction){
    if (fixture->IsSensor()) return 1.0f;

    bool isPortal = false;
    bodyData* bd = (bodyData*)(fixture->GetBody()->GetUserData().pointer);
    if (bd){
        if (bd->type == PORTAL){
            Portal* p = (Portal*)bd->data;
            int side = p->getPointSide(point + normal);
            if (p->connections[side].size() != 0){
                isPortal = true;
            }
        }
        else if (bd->type == PORTAL_BODY){
            PortalBody* pb = (PortalBody*)bd->data;
            std::set<portalCollision*>* collidingPortals = pb->fixtureCollisions[fixture];
            for (portalCollision* pc : *collidingPortals){
                if (!pc->status || pc->portal->getPointSide(point) == (pc->side ^ 1)){
                    return 1.0f;
                }
            }
        }
    }

    if (fraction > this->portalThold){
        if (isPortal && fraction < this->portalFraction){
            this->portalFraction = fraction;
            this->closestPortalFixture = fixture;
        }
        if (fraction <= this->minFraction){
            this->minFraction = fraction;
            this->closestFixture = fixture;
        }
    }

    return 1.0f;
}

void PortalRay::reset(){
    this->minFraction = 1.0f;
    this->portalFraction = 1.0f;

    this->closestFixture = NULL;
    this->closestPortalFixture = NULL;
}

void PortalRay::endHandle(){
    if (this->minFraction > this->portalFraction - this->portalThold){
        this->closestFixture = this->closestPortalFixture;
        this->minFraction = this->portalFraction;
    }
}