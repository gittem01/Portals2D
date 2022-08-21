#include <DebugDrawer.h>
#include "PortalBody.h"
#include "Portal.h"
#include <RotationJoint.h>
#include <PositionJoint.h>


PortalWorld::PortalWorld() : b2World(b2Vec2(0, -11.15f)){
    this->rayHandler = new PortalRay(this);
    this->evalRayHandler = new EvaluationRay(this);
}

PortalWorld::~PortalWorld(){
    for (Portal* portal : portals){
        delete portal;
    }
    for (PortalBody* pb : portalBodies){
        delete pb;
    }
    for (auto& indices : bodyIndices){
        delete indices;
    }

    delete rayHandler;
    delete evalRayHandler;
}

Portal* PortalWorld::createPortal(b2Vec2 pos, b2Vec2 dir, float size){
    Portal* portal = new Portal(pos, dir, size, this);
    portals.push_back(portal);

    return portal;
}

void PortalWorld::destroyPortal(Portal* portal){
    for (int i = 0; i < portals.size(); i++){
        Portal* p = portals.at(i);
        if (p == portal){
            portals.erase(portals.begin() + i);
            delete portal;
            break;
        }
    }
}

PortalBody* PortalWorld::createPortalBody(b2Body* body){
    PortalBody* pBody = new PortalBody(body, this);
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

        std::vector<float>* baseDensities = new std::vector<float>();
        pBody->baseDensities = baseDensities;
        for (b2Fixture* fix = pBody->body->GetFixtureList(); fix; fix = fix->GetNext()){
            baseDensities->push_back(fix->GetDensity());
        }
    }
    else{
        baseBody->worldIndex->push_back(pBody);
        pBody->worldIndex = baseBody->worldIndex;

        handledBodies.insert(pBody->worldIndex);
    }
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

    closestFixture = NULL;
    closestPortalFixture = NULL;
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

    pWorld->getDebugDraw()->DrawSegment(rayStart, collPoint, b2Color(1, 0, 0));

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

                b2Vec2 localPos = PortalWorld::rotateVec(posDiff, angleRot + b2_pi);
                if (conn->isReversed){
                    localPos = pWorld->mirror(p2Dir, localPos);
                }

                b2Vec2 ray2Pos = conn->portal2->pos + localPos;

                b2Vec2 dir2 = PortalWorld::rotateVec(dirVec, angleRot);

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

void PortalWorld::PortalStep(float timeStep, int32 velocityIterations, int32 positionIterations){
    Step(timeStep, velocityIterations, positionIterations);
    
    for (int i = 0; i < portalBodies.size(); i++){
        PortalBody* body = portalBodies.at(i);
        body->postHandle();
    }
    
    // rayHandler->sendRay(b2Vec2(-10, 10.0f), b2Vec2(1, -1), 10.0f);

    // for (rayData* rd : rayHandler->rayResult){
    //     DebugDrawer* drawer = (DebugDrawer*)m_debugDraw;
    //     drawer->renderer->drawArrow(rd->endPos - rd->dir, rd->endPos, b2Color(1, 1, 0));
    // }

    globalPostHandle();
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

    for (auto& hBody : handledBodies){
        int numBodies = hBody->size();
        for (PortalBody* portBody : *hBody){
            int i = 0;
            for (b2Fixture* fix = portBody->body->GetFixtureList(); fix; fix = fix->GetNext()){
                fix->SetDensity(portBody->baseDensities->at(i++) / numBodies);
            }
            portBody->body->ResetMassData();
        }
    }

    handledBodies.clear();
    destroyBodies.clear();
}

void PortalWorld::createCloneBody(bodyStruct* s){
    PortalBody* pBody = s->pBody;
    int side = s->side;
    Portal* collPortal = s->collPortal;
    b2Body* body1 = s->pBody->body;
    b2Vec2 dir1 = side ? -collPortal->dir : collPortal->dir;
    b2Vec2 posDiff = collPortal->pos - body1->GetPosition();
    b2Vec2 speed = body1->GetLinearVelocity();

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
        PortalBody* npb = new PortalBody(body2, this);

        npb->baseDensities = pBody->baseDensities;
        npb->isMirrored = pBody->isMirrored ^ c->isReversed;

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

        b2Fixture* prev = NULL;
        b2Fixture* current = body1->GetFixtureList();
        b2Fixture* next = NULL;
        while (current != NULL) {
            next = current->m_next;
            current->m_next = prev;
            prev = current;
            current = next;
        }
        body1->m_fixtureList = prev;

        bool allOut = true;
        int fixMargin = 0;
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

            fixtureData* fixData = (fixtureData*)calloc(1, sizeof(fixtureData));
            f->GetUserData().pointer = (uintptr_t)fixData;

            portalCollision* col = (portalCollision*)malloc(sizeof(portalCollision));;

            if (collPortal->collidingFixtures[side].find(fix) != collPortal->collidingFixtures[side].end()){
                int fixSide = collPortal->getFixtureSide(fix);
                
                bool rayCheck = false;
                bool release = false;
                if (fixSide == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                else{
                    bool check = collPortal->rayCheck(fix);
                    if (!check){
                        release = true;
                    }
                    else{
                        rayCheck = true;
                    }
                }

                if (rayCheck){
                    fixMargin -= 1;
                    portal2->collidingFixtures[c->side2].insert(f);
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = 1;
                    fixData->pCollisions.push_back(col);
                    allOut = false;
                }
                else if (release){
                    fixMargin -= 2;
                    portal2->releaseFixtures[c->side2][f] = 1;
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = 0;
                    fixData->pCollisions.push_back(col);
                    allOut = false;
                }
                else{
                    free(col);
                }
            }
            else if (collPortal->releaseFixtures[side].find(fix) == collPortal->releaseFixtures[side].end()){
                if (collPortal->collidingFixtures[side ^ 1].find(fix) != collPortal->collidingFixtures[side ^ 1].end()){
                    fixMargin -= 2;
                    portal2->releaseFixtures[c->side2][f] = 2;
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = 0;
                    fixData->pCollisions.push_back(col);

                    bool rayCheck;
                    int fixSide = collPortal->getFixtureSide(fix);
                    if (fixSide == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                    else rayCheck = 1;

                    if (!rayCheck){
                        portal2->releaseFixtures[c->side2][f]--;
                    }
                }
                else if (collPortal->releaseFixtures[side ^ 1].find(fix) != collPortal->releaseFixtures[side ^ 1].end()){
                    fixMargin -= 2;
                    portal2->releaseFixtures[c->side2][f] = collPortal->releaseFixtures[side ^ 1][fix] + 2;
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = 0;
                    fixData->pCollisions.push_back(col);

                    if (collPortal->releaseFixtures[side ^ 1][fix] % 2 == 0){
                        bool rayCheck;
                        int fixSide = collPortal->getFixtureSide(fix);
                        if (fixSide == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                        else rayCheck = 1;

                        if (!rayCheck){
                            portal2->releaseFixtures[c->side2][f]--;
                        }
                    }
                }
                else{
                    fixMargin -= 2;
                    portal2->releaseFixtures[c->side2][f] = 1;
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = 0;
                    fixData->pCollisions.push_back(col);
                }
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

        prev = NULL;
        current = body1->GetFixtureList();
        next = NULL;
        while (current != NULL) {
            next = current->m_next;
            current->m_next = prev;
            prev = current;
            current = next;
        }
        body1->m_fixtureList = prev;

        createPortalBody_i(npb, pBody, false);
        if (allOut) destroyBodies.insert(pBody);
        else connectBodies(pBody, npb, c, side);

        npb->numFixtures = pBody->numFixtures;
        npb->outFixtures[c->side2][portal2] = npb->numFixtures + fixMargin;
    }
}

void PortalWorld::connectBodies(PortalBody* body1, PortalBody* body2, portalConnection* connection, int side) {
    RotationJointDef rotDef;
    rotDef.Initialize(body1->body, body2->body, connection->isReversed);
    rotDef.collideConnected = true;

    CreateRotationJoint(&rotDef, body1, body2);

    PositionJointDef posJointDef;
    posJointDef.Initialize(body1->body, body2->body, connection);
    posJointDef.collideConnected = true;

    CreatePositionJoint(&posJointDef);
    
    posJointDef.Initialize(body1->body, body2->body, connection, true);

    CreatePositionJoint(&posJointDef);
}

void PortalWorld::CreateRotationJoint(RotationJointDef* def, PortalBody* pb1, PortalBody* pb2){
    void* mem = m_blockAllocator.Allocate(sizeof(RotationJoint));
	RotationJoint* j = new (mem) RotationJoint(def, pb1, pb2);
    commonJointCreation(j);
}

void PortalWorld::CreatePositionJoint(PositionJointDef* def){
    void* mem = m_blockAllocator.Allocate(sizeof(PositionJoint));
	PositionJoint* j = new (mem) PositionJoint(def);
    commonJointCreation(j);
}

void PortalWorld::commonJointCreation(b2Joint* cj){
    cj->m_prev = nullptr;
	cj->m_next = m_jointList;
	if (m_jointList)
	{
		((b2Joint*)m_jointList)->m_prev = cj;
	}
	m_jointList = cj;
	++m_jointCount;
	
	cj->m_edgeA.joint = cj;
	cj->m_edgeA.other = cj->m_bodyB;
	cj->m_edgeA.prev = nullptr;
	cj->m_edgeA.next = cj->m_bodyA->m_jointList;
	if (cj->m_bodyA->m_jointList) cj->m_bodyA->m_jointList->prev = &cj->m_edgeA;
	cj->m_bodyA->m_jointList = &cj->m_edgeA;

	cj->m_edgeB.joint = cj;
	cj->m_edgeB.other = cj->m_bodyA;
	cj->m_edgeB.prev = nullptr;
	cj->m_edgeB.next = cj->m_bodyB->m_jointList;
	if (cj->m_bodyB->m_jointList) cj->m_bodyB->m_jointList->prev = &cj->m_edgeB;
	cj->m_bodyB->m_jointList = &cj->m_edgeB;
}

void PortalWorld::DestroyCustomJoint(b2Joint* cj){
	bool collideConnected = cj->m_collideConnected;

	if (cj->m_prev)
	{
		cj->m_prev->m_next = cj->m_next;
	}

	if (cj->m_next)
	{
		cj->m_next->m_prev = cj->m_prev;
	}

	if (cj == m_jointList)
	{
		m_jointList = cj->m_next;
	}

	b2Body* bodyA = cj->m_bodyA;
	b2Body* bodyB = cj->m_bodyB;

	bodyA->SetAwake(true);
	bodyB->SetAwake(true);

	if (cj->m_edgeA.prev)
	{
		cj->m_edgeA.prev->next = cj->m_edgeA.next;
	}

	if (cj->m_edgeA.next)
	{
		cj->m_edgeA.next->prev = cj->m_edgeA.prev;
	}

	if (&cj->m_edgeA == bodyA->m_jointList)
	{
		bodyA->m_jointList = cj->m_edgeA.next;
	}

	cj->m_edgeA.prev = nullptr;
	cj->m_edgeA.next = nullptr;

	if (cj->m_edgeB.prev)
	{
		cj->m_edgeB.prev->next = cj->m_edgeB.next;
	}

	if (cj->m_edgeB.next)
	{
		cj->m_edgeB.next->prev = cj->m_edgeB.prev;
	}

	if (&cj->m_edgeB == bodyB->m_jointList)
	{
		bodyB->m_jointList = cj->m_edgeB.next;
	}

	cj->m_edgeB.prev = nullptr;
	cj->m_edgeB.next = nullptr;

    if (((BaseCustomJoint*)cj)->jointType == ROTATION){
        m_blockAllocator.Free(cj, sizeof(RotationJoint));
    }
    else{
        m_blockAllocator.Free(cj, sizeof(PositionJoint));
    }
	b2Assert(m_jointCount > 0);
	--m_jointCount;

	if (collideConnected == false)
	{
		b2ContactEdge* edge = bodyB->GetContactList();
		while (edge)
		{
			if (edge->other == bodyA)
			{
				edge->contact->FlagForFiltering();
			}

			edge = edge->next;
		}
	}
}

b2Vec2 PortalWorld::rotateVec(b2Vec2 vec, float angle){
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return {x, y};
}

bool PortalWorld::isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) >= t;
}

float PortalWorld::vecAngle(b2Vec2 v1, b2Vec2 v2){
    return abs(calcAngle2(v1) - calcAngle2(v2));
}

float PortalWorld::getDist(b2Vec2 a, b2Vec2 b, b2Vec2 c){
     return ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
}

float PortalWorld::getRealDist(b2Vec2 a, b2Vec2 b, b2Vec2 c){
    float v1 = (b.x - a.x) * (a.y - c.y);
    float v2 = (a.x - c.x) * (b.y - a.y);
    float top = v1 - v2;

    v1 = pow(b.x - a.x, 2);
    v2 = pow(b.y - a.y, 2);
    float bot = sqrt(v1 + v2);

    return top / bot;
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

b2Vec2 PortalWorld::getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4){
    float d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    float up1 = (p1.x * p2.y - p1.y * p2.x) * (p3.x - p4.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x);
    float up2 = (p1.x * p2.y - p1.y * p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x);

    return b2Vec2(up1 / d, up2 / d);
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
            fixtureData* fixData = (fixtureData*)(fixture->GetUserData().pointer);
            std::vector<portalCollision*> collidingPortals = fixData->pCollisions;
            for (portalCollision* pc : collidingPortals){
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
