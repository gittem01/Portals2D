#include "PortalBody.h"
#include "Portal.h"
#include <RotationJoint.h>
#include <PositionJoint.h>
#include <GameBase.h>

PortalWorld::PortalWorld() : b2World(b2Vec2(0, -11.15f)){
    this->rayHandler = new PortalRay(this);
    this->evalRayHandler = new EvaluationRay(this);
}

PortalWorld::~PortalWorld(){
    for (PortalBody* pb : portalBodies)
    {
        delete pb;
    }
    for (int i = 0; i < portals.size(); i++)
    {
        destroyPortal(portals.at(i));
        i--;
    }

    for (Portal* portal : portals)
    {
        delete portal;
    }

    for (auto indices : bodyIndices)
    {
        delete indices;
    }

    delete rayHandler;
    delete evalRayHandler;
}

Portal* PortalWorld::createPortal(b2Vec2 pos, b2Vec2 dir, float size, b2Body* onBody)
{
    Portal* portal = new Portal(pos, dir, size, this, onBody);
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

void PortalWorld::createPortalBody_i(PortalBody* pBody, PortalBody* baseBody, bool isNew)
{
    if (isNew)
    {
        pbWorldData* pbData = new pbWorldData();
        pbData->pbIndices.push_back(pBody);
        bodyIndices.push_back(pbData);
        pBody->worldIndex = pbData;

        std::vector<float>* baseDensities = new std::vector<float>();
        pBody->baseDensities = baseDensities;
        for (b2Fixture* fix = pBody->body->GetFixtureList(); fix; fix = fix->GetNext())
        {
            baseDensities->push_back(fix->GetDensity());
        }
    }
    else
    {
        baseBody->worldIndex->pbIndices.push_back(pBody);
        pBody->worldIndex = baseBody->worldIndex;

        handledBodies.insert(pBody->worldIndex);

        for (auto pair : baseBody->worldIndex->createCallbacks)
        {
            pair.first(pBody, pair.second);
        }
    }
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

    // change this
    if (rayOutPortal && false){
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

    if (closestFixture != NULL){
        if (closestFixture == closestPortalFixture){
            fixtureData* fData = (fixtureData*)closestFixture->GetUserData().pointer;
            Portal* p = (Portal*)fData->data;
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

void PortalWorld::PortalStep(float timeStep, int32 velocityIterations, int32 positionIterations)
{
    Step(timeStep, velocityIterations, positionIterations);
    
    for (int i = 0; i < portalBodies.size(); i++)
    {
        PortalBody* body = portalBodies.at(i);
        body->postHandle();
    }
    
    // for moving portals
    for (Portal* p : portals)
    {
        p->calculatePoints();
    }

    rayHandler->sendRay(b2Vec2(-10, 10.0f), b2Vec2(1, -1), 10.0f);

    globalPostHandle();
}

void PortalWorld::globalPostHandle()
{
    for (PortalBody* portalBody : destroyBodies)
    {
        for (int i = 0; i < portalBodies.size(); i++)
        {
            if (portalBodies.at(i) == portalBody)
            {
                portalBodies.erase(portalBodies.begin() + i);
                break;
            }
        }

        for (auto pair : portalBody->worldIndex->destroyCallbacks)
        {
            pair.first(portalBody, pair.second);
        }

        delete portalBody;
    }

    for (auto& hBody : handledBodies){
        int numBodies = hBody->pbIndices.size();
        for (PortalBody* portBody : hBody->pbIndices){
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

b2Vec2 PortalWorld::getRelativePortalPosition_L(b2Vec2 point, portalConnection* pConnect) {
    Portal* portal1 = pConnect->portal1;
    Portal* portal2 = pConnect->portal2;
    int side1 = pConnect->side1;
    int side2 = pConnect->side2;
    bool reversed = pConnect->isReversed;

    b2Vec2 dir1 = side1 ? -portal1->dir : portal1->dir;
    b2Vec2 dir2 = side2 ? -portal2->dir : portal2->dir;

    float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);
    b2Vec2 pos = rotateVec(point, angleRot + b2_pi);
    if (reversed)
        pos = mirror(dir2, pos);

    return pos;
}

b2Vec2 PortalWorld::getRelativePortalPositionReverse_L(b2Vec2 point, portalConnection* pConnect) {
    Portal* portal1 = pConnect->portal2;
    Portal* portal2 = pConnect->portal1;
    int side1 = pConnect->side2;
    int side2 = pConnect->side1;
    bool reversed = pConnect->isReversed;

    b2Vec2 dir1 = side1 ? -portal1->dir : portal1->dir;
    b2Vec2 dir2 = side2 ? -portal2->dir : portal2->dir;

    float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);
    b2Vec2 pos = rotateVec(point, angleRot + b2_pi);
    if (reversed)
        pos = mirror(dir2, pos);

    return pos;
}

b2Vec2 PortalWorld::getRelativePortalPosition_W(b2Vec2 point, portalConnection* pConnect){
    Portal* portal1 = pConnect->portal1;
    Portal* portal2 = pConnect->portal2;
    int side1 = pConnect->side1;
    int side2 = pConnect->side2;
    bool reversed = pConnect->isReversed;

    b2Vec2 dir1 = side1 ? -portal1->dir : portal1->dir;
    b2Vec2 dir2 = side2 ? -portal2->dir : portal2->dir;

    b2Vec2 posDiff = portal1->pos - point;
    float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);
    b2Vec2 pos = rotateVec(posDiff, angleRot + b2_pi);
    if (reversed)
        pos = mirror(dir2, pos);

    pos += portal2->pos;

    return pos;
}

b2Vec2 PortalWorld::getRelativePortalPositionReverse_W(b2Vec2 point, portalConnection* pConnect){
    Portal* portal1 = pConnect->portal2;
    Portal* portal2 = pConnect->portal1;
    int side1 = pConnect->side2;
    int side2 = pConnect->side1;
    bool reversed = pConnect->isReversed;

    b2Vec2 dir1 = side1 ? -portal1->dir : portal1->dir;
    b2Vec2 dir2 = side2 ? -portal2->dir : portal2->dir;

    b2Vec2 posDiff = portal1->pos - point;
    float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);
    b2Vec2 pos = rotateVec(posDiff, angleRot + b2_pi);
    if (reversed)
        pos = mirror(dir2, pos);

    pos += portal2->pos;

    return pos;
}

bool PortalWorld::createCloneBody(Portal* collPortal, int side, PortalBody* pBody)
{
    bool retVal = false;

    b2Body* body1 = pBody->body;
    b2Vec2 dir1 = side ? -collPortal->dir : collPortal->dir;
    b2Vec2 posDiff = collPortal->pos - body1->GetPosition();
    b2Vec2 baseSpeed = body1->GetLinearVelocity();

    void* extraData0 = ((bodyData*)body1->GetUserData().pointer)->extraData;

    collPortal->collidingBodies[side].insert(pBody);

    b2Vec2 relativePortalPos1 = collPortal->pos - collPortal->body->GetPosition();
    b2Vec2 linSpeedAddition1 = collPortal->body->GetAngularVelocity() * PortalWorld::rotateVec(relativePortalPos1, b2_pi * 0.5f);
    b2Vec2 portalLinSpeed1 = collPortal->getLinVel() + linSpeedAddition1;

    for (portalConnection* c : collPortal->connections[side])
    {
        b2Vec2 speed = baseSpeed - portalLinSpeed1;
        Portal* portal2 = c->portal2;
        
        b2Vec2 dir2 = c->side2 ? -portal2->dir : portal2->dir;
        float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);

        b2Vec2 localPos = rotateVec(posDiff, angleRot + b2_pi);
        if (c->isReversed)
            localPos = mirror(dir2, localPos);

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
        bodyData* extraDataFiller = (bodyData*)body2->GetUserData().pointer;
        extraDataFiller->extraData = extraData0;

        npb->baseDensities = pBody->baseDensities;
        npb->isMirrored = pBody->isMirrored ^ c->isReversed;

        if (c->isReversed){
            float pAngle = calcAngle2(portal2->dir);
            float angleRes = (pAngle - (angleRot + body2->GetAngle())) * 2.0f;
            npb->offsetAngle = angleRes + angleRot - b2_pi - pBody->offsetAngle;
        }
        else{
            npb->offsetAngle = angleRot + pBody->offsetAngle;
        }

        for (b2JointEdge* edge = body1->GetJointList(); edge != NULL; edge = edge->next)
        {
            b2Joint* j = edge->joint;
            if (j->GetType() != e_revoluteJoint)
            {
                continue;
            }
            b2Body* bodyA = j->GetBodyA();
            b2Body* bodyB = j->GetBodyB();

            if (bodyB->GetType() != b2_dynamicBody)
            {
                continue;
            }

            // body1 is original body so switch data loss is OK
            bool switched = false;
            if (bodyA != body1)
            {
                bodyB = bodyA;
                switched = true;
            }

            bodyData* bd = (bodyData*)bodyB->GetUserData().pointer;
            if (!bd)
            {
                continue;
            }

            PortalBody* pbB = (PortalBody*)bd->data;

            b2RevoluteJoint* rj = (b2RevoluteJoint*)j;
            for (bodyCollisionStatus* bcs : *pbB->bodyMaps)
            {
                if (bcs->connection == c)
                {
                    b2RevoluteJointDef jDef;

                    float angle1 = body2->GetAngle() + npb->offsetAngle;
                    float angle2 = bcs->body->body->GetAngle() + bcs->body->offsetAngle;

                    float length1;
                    float length2;

                    if (npb->isMirrored)
                    {
                        if (switched)
                        {
                            angle1 += b2_pi - rj->baseAnchorAngleB;
                            length1 = rj->baseAnchorLengthB;
                        }
                        else
                        {
                            angle1 += b2_pi - rj->baseAnchorAngleA;
                            length1 = rj->baseAnchorLengthA;
                        }
                    }
                    else
                    {
                        if (switched)
                        {
                            angle1 += rj->baseAnchorAngleB;
                            length1 = rj->baseAnchorLengthB;
                        }
                        else
                        {
                            angle1 += rj->baseAnchorAngleA;
                            length1 = rj->baseAnchorLengthA;
                        }
                    }

                    if (bcs->body->isMirrored)
                    {
                        if (switched)
                        {
                            angle2 += b2_pi - rj->baseAnchorAngleA;
                            length2 = rj->baseAnchorLengthA;
                        }
                        else
                        {
                            angle2 += b2_pi - rj->baseAnchorAngleB;
                            length2 = rj->baseAnchorLengthB;
                        }
                    }
                    else
                    {
                        if (switched)
                        {
                            angle2 += rj->baseAnchorAngleA;
                            length2 = rj->baseAnchorLengthA;
                        }
                        else
                        {
                            angle2 += rj->baseAnchorAngleB;
                            length2 = rj->baseAnchorLengthB;
                        }
                    }

                    if (!switched ^ c->isReversed)
                    {
                        jDef.bodyA = body2;
                        jDef.bodyB = bcs->body->body;

                        jDef.localAnchorA = length1 * b2Vec2(cos(angle1), sin(angle1));
                        jDef.localAnchorA = body2->GetLocalPoint(body2->GetPosition() + jDef.localAnchorA);

                        jDef.localAnchorB = length2 * b2Vec2(cos(angle2), sin(angle2));
                        jDef.localAnchorB = bcs->body->body->GetLocalPoint(bcs->body->body->GetPosition() + jDef.localAnchorB);

                        jDef.referenceAngle = npb->offsetAngle - bcs->body->offsetAngle;
                    }
                    else
                    {
                        jDef.bodyA = bcs->body->body;
                        jDef.bodyB = body2;

                        jDef.localAnchorA = length2 * b2Vec2(cos(angle2), sin(angle2));
                        jDef.localAnchorA = bcs->body->body->GetLocalPoint(bcs->body->body->GetPosition() + jDef.localAnchorA);

                        jDef.localAnchorB = length1 * b2Vec2(cos(angle1), sin(angle1));
                        jDef.localAnchorB = npb->body->GetLocalPoint(npb->body->GetPosition() + jDef.localAnchorB);

                        jDef.referenceAngle = bcs->body->offsetAngle - npb->offsetAngle;
                    }

                    jDef.enableLimit = rj->IsLimitEnabled();
                    jDef.lowerAngle = rj->GetLowerLimit();
                    jDef.upperAngle = rj->GetUpperLimit();

                    if (c->isReversed)
                    {
                        jDef.baseAnchorAngleA = rj->baseAnchorAngleB;
                        jDef.baseAnchorAngleB = rj->baseAnchorAngleA;

                        jDef.baseAnchorLengthA = rj->baseAnchorLengthB;
                        jDef.baseAnchorLengthB = rj->baseAnchorLengthA;
                    }
                    else
                    {
                        jDef.baseAnchorAngleA = rj->baseAnchorAngleA;
                        jDef.baseAnchorAngleB = rj->baseAnchorAngleB;

                        jDef.baseAnchorLengthA = rj->baseAnchorLengthA;
                        jDef.baseAnchorLengthB = rj->baseAnchorLengthB;
                    }

                    CreateJoint(&jDef);
                    break;
                }
            }
        }

        portalBodies.push_back(npb);

        bodyCollisionStatus* bcs = new bodyCollisionStatus;
        bcs->body = pBody;

        for (portalConnection* conn : portal2->connections[c->side2]){
            if (conn->portal2 == collPortal && conn->side2 == c->side1){
                bcs->connection = conn;
                conn->portal1->collidingBodies[conn->side1].insert(npb);
                break;
            }
        }

        bodyCollisionStatus* t_bcs = new bodyCollisionStatus;
        t_bcs->body = npb;
        npb->fixtures.resize(pBody->fixtures.size());
        t_bcs->connection = c;

        b2Vec2 lvToSet = rotateVec(speed, angleRot);
        if (c->isReversed){
            body2->SetAngularVelocity(-body1->GetAngularVelocity() /*+ collPortal->getAngVel() + portal2->getAngVel()*/);
            lvToSet = mirror(dir2, lvToSet);
        }
        else{
            body2->SetAngularVelocity(body1->GetAngularVelocity() /*- collPortal->getAngVel() + portal2->getAngVel()*/);
        }

        b2Vec2 relativePortalPos2 = c->portal2->pos - c->portal2->body->GetPosition();
        b2Vec2 linSpeedAddition2 = c->portal2->body->GetAngularVelocity() * PortalWorld::rotateVec(relativePortalPos2, b2_pi * 0.5f);
        b2Vec2 portalLinSpeed2 = c->portal2->getLinVel() + linSpeedAddition2;

        lvToSet += portalLinSpeed2;

        body2->SetLinearVelocity(lvToSet);

        bool allOut = true;
        int fixMargin = 0;
        for (int i = pBody->fixtures.size() - 1; i > -1; i--){
            b2Fixture* fix = pBody->fixtures.at(i);
            b2Fixture* f;
            b2FixtureDef fDef;
            fDef.density = fix->GetDensity();
            fDef.restitution = fix->GetRestitution();
            fDef.friction = fix->GetFriction();
            if (fix->GetType() == b2Shape::Type::e_polygon){
                b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();
                
                b2PolygonShape newShape;
                fDef.shape = &newShape;

                newShape.m_count = polyShape->m_count;
                b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2) * polyShape->m_count);

                for (int j = 0; j < polyShape->m_count; j++){
                    if (c->isReversed){
                        b2Vec2 v = rotateVec(polyShape->m_vertices[j], angleRot + body2->GetAngle());
                        v = mirror(portal2->dir, v);
                        v = rotateVec(v, -body2->GetAngle());
                        vertices[j] = v;
                    }
                    else{
                        vertices[j] = rotateVec(polyShape->m_vertices[j], angleRot);
                    }
                }
                newShape.Set(vertices, polyShape->m_count);

                f = body2->CreateFixture(&fDef);
                free(vertices);
            }
            else{
                b2CircleShape* circleShape = (b2CircleShape*)fix->GetShape();
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

            npb->fixtures[i] = f;

            fixtureData* fixData1 = (fixtureData*)fix->GetUserData().pointer;
            PBFixtureData* pbfData1 = (PBFixtureData*)fixData1->data;
            fixtureData* fixData2 = new fixtureData();
            PBFixtureData* pbfData2 = new PBFixtureData();
            fixData2->type = F_PORTAL_BODY;
            fixData2->data = (void*)pbfData2;

            pbfData2->index = i;
            pbfData2->angleThold = PortalBody::calculateAngleThold(f);
            f->GetUserData().pointer = (uintptr_t)fixData2;

            portalCollision* col = (portalCollision*)malloc(sizeof(portalCollision));;

            int status = pbfData1->getPortalStatus(collPortal, side);
            int status2 = pbfData1->getPortalStatus(collPortal, side ^ 1);
            if (status == 1){
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
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = 1;
                    pbfData2->pCollisions.push_back(col);
                    allOut = false;
                }
                else if (release){
                    fixMargin -= 2;
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = -1;
                    pbfData2->pCollisions.push_back(col);
                    allOut = false;
                }
                else{
                    free(col);
                }
            }
            else if (status == 2){
                fixMargin -= 2;
                if (status2 == 1){
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = -2;
                    pbfData2->pCollisions.push_back(col);

                    bool rayCheck;
                    int fixSide = collPortal->getFixtureSide(fix);
                    if (fixSide == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                    else rayCheck = 1;

                    if (!rayCheck){
                        col->status++;
                    }
                }
                else if (status2 < 0){
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = status2 - 2;
                    pbfData2->pCollisions.push_back(col);

                    if (status2 % 2 == 0){
                        bool rayCheck;
                        int fixSide = collPortal->getFixtureSide(fix);
                        if (fixSide == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                        else rayCheck = 1;

                        if (!rayCheck){
                            col->status++;
                        }
                    }
                }
                else{
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = -1;
                    pbfData2->pCollisions.push_back(col);
                }
            }
            else{
                free(col);
            }

            if (status != 1 && status2 != 1){
                //allOut = false;
            }
        }

        npb->numFixtures = pBody->numFixtures;
        npb->outFixtures[c->side2][portal2] = npb->numFixtures + fixMargin;
        pBody->bodyMaps->push_back(t_bcs);
        npb->bodyMaps->push_back(bcs);

        if (allOut)
        {
            retVal = true;
        }

        createPortalBody_i(npb, pBody, false);

        connectBodies(pBody, npb, c, side);
    }

    return retVal;
}

void PortalWorld::connectBodies(PortalBody* body1, PortalBody* body2, portalConnection* connection, int side)
{
    RotationJointDef rotDef;
    rotDef.Initialize(body1->body, body2->body, connection);
    rotDef.collideConnected = true;

    CreateRotationJoint(&rotDef, body1, body2);
    
    PositionJointDef posJointDef;
    posJointDef.Initialize(body1->body, body2->body, connection);
    posJointDef.collideConnected = true;
    CreatePositionJoint(&posJointDef);
    
    posJointDef.isRotated = true;
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

    if (((BaseCustomJoint*)cj)->jointType == ROTATION)
    {
        m_blockAllocator.Free(cj, sizeof(RotationJoint));
    }
    else
    {
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

// it checks for left (really)
bool PortalWorld::isLeft(b2Vec2 a, b2Vec2 b, b2Vec2 c, const float t)
{
	return ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) >= t;
}

float PortalWorld::vecAngle(b2Vec2 v1, b2Vec2 v2){
	return abs(calcAngle2(v1) - calcAngle2(v2));
}

// left side gives positive values
float PortalWorld::getDist(b2Vec2 a, b2Vec2 b, b2Vec2 c)
{
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

// project a on b
b2Vec2 PortalWorld::project(b2Vec2 a, b2Vec2 b)
{
    if (b.x == 0.0f && b.y == 0.0f) return b2Vec2(0.0f, 0.0f);
    return (b2Dot(a, b) / b2Dot(b, b)) * b;
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

// normalize angle between -pi and pi
float PortalWorld::normalizeAngle(float angle)
{
    angle = fmod(angle, 2 * b2_pi);
    if (angle < -b2_pi)
        angle += 2 * b2_pi;
    else if (angle > b2_pi)
        angle -= 2 * b2_pi;

    return angle;
}

b2Vec2 PortalWorld::getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4){
    float d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    float up1 = (p1.x * p2.y - p1.y * p2.x) * (p3.x - p4.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x);
    float up2 = (p1.x * p2.y - p1.y * p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x);

    return b2Vec2(up1 / d, up2 / d);
}

Portal* PortalWorld::createPortalWithChecks(b2Fixture* edgeFix, b2Vec2 pos, b2Vec2 dir, float size)
{
    if (!edgeFix)
    {
        return nullptr;
    }

    float maxReqSize = size + 0.5f;
    float rightSize = 0.0f;
    float leftSize = 0.0f;

    b2Vec2 rightPosCopy = pos;
    b2Vec2 leftPosCopy = pos;

    g_EdgeData* edgeData = nullptr;
    fixtureData* fixData = (fixtureData*)edgeFix->GetUserData().pointer;
    if (fixData && fixData->extraData)
    {
        g_PhysicsData* pData = (g_PhysicsData*)fixData->extraData;
        if (pData && pData->type == EDGE && pData->data)
        {
            edgeData = (g_EdgeData*)pData->data;
        }
    }

    g_EdgeData* rightEdgeData = edgeData;
    g_EdgeData* leftEdgeData = edgeData;

    bool rightDone = false;
    bool leftDone = false;

    for (; !rightDone || !leftDone;)
    {
        if (!rightDone && rightEdgeData && (rightEdgeData->wall == PORTALLABLE || rightEdgeData->gel == WHITE))
        {
            b2Vec2 vertexPos = getVertexByIndex(rightEdgeData, 0);
            float dist = (rightPosCopy - vertexPos).Length();
            rightPosCopy = vertexPos;
            rightSize += dist;
            if (rightSize > maxReqSize)
            {
                rightSize = maxReqSize;
                rightDone = true;
            }
            else
            {
                g_EdgeData* eData = rightEdgeData->edges[0];
                if (eData == nullptr || (eData->wall != PORTALLABLE && eData->gel != WHITE))
                {
                    rightDone = true;
                }
                else
                {
                    rightEdgeData = eData;
                }
            }
        }
        else
        {
            rightDone = true;
        }

        if (!leftDone && leftEdgeData && (leftEdgeData->wall == PORTALLABLE || leftEdgeData->gel & WHITE))
        {
            b2Vec2 vertexPos = getVertexByIndex(leftEdgeData, 1);
            float dist = (leftPosCopy - vertexPos).Length();
            leftPosCopy = vertexPos;
            leftSize += dist;
            if (leftSize > maxReqSize)
            {
                leftSize = maxReqSize;
                leftDone = true;
            }
            else
            {
                g_EdgeData* eData = leftEdgeData->edges[1];
                if (eData == nullptr || (eData->wall != PORTALLABLE && eData->gel != WHITE))
                {
                    leftDone = true;
                }
                else
                {
                    leftEdgeData = eData;
                }
            }
        }
        else
        {
            leftDone = true;
        }
    }

    float totalLen = rightSize + leftSize;

    if (totalLen < size - 0.000001f)
    {
        return nullptr;
    }

    destructonCreatorData* destrData = (destructonCreatorData*)malloc(sizeof(destructonCreatorData));
    createDestrData* cdData = (createDestrData*)malloc(sizeof(createDestrData));
    createDestrData* lastCdData = nullptr;
    destrData->datas[1] = cdData;
    cdData->datas[0] = nullptr;
    cdData->datas[1] = nullptr;

    fixtureData* portalFixData;
    g_EdgeData* portalEdgeData;
    createEdgeData(&portalFixData, &portalEdgeData);
    portalEdgeData->gel = NONE;
    portalEdgeData->wall = PORTAL_ITSELF;

    float reqLeft = rightSize > size * 0.5f ? fmin(leftSize, size * 0.5f) : size - rightSize;
    float remDist;

    b2Vec2 leftPortalPoint;
    g_EdgeData* leftEdge = edgeData;
    b2Vec2 lastVertexPos = pos;
    for (;;)
    {
        b2Vec2 vertexPos = getVertexByIndex(leftEdge, 1);
        float dist = (lastVertexPos - vertexPos).Length();
        reqLeft -= dist;

        if (reqLeft < 0.0f)
        {
            float innerLen = dist + reqLeft;
            b2Vec2 rightVertex = getVertexByIndex(leftEdge, 0);
            b2Vec2 origRightVertex = rightVertex;

            if (lastVertexPos == pos)
            {
                rightVertex = lastVertexPos;
            }

            b2Body* edgeBody = leftEdge->fix->GetBody();
            fillCdData(cdData, leftEdge);

            edgeBody->DestroyFixture(leftEdge->fix);

            b2Vec2 dir = vertexPos - rightVertex;
            dir.Normalize();
            b2Vec2 newVertexPos = rightVertex + innerLen * dir;
            
            g_EdgeData* newFixDL = createEdge(newVertexPos, vertexPos, this, edgeBody);
            newFixDL->gel = leftEdge->gel;
            newFixDL->wall = leftEdge->wall;

            portalEdgeData->edges[1] = newFixDL;
            newFixDL->edges[0] = portalEdgeData;
            newFixDL->edges[1] = leftEdge->edges[1];
            if (leftEdge->edges[1])
            {
                leftEdge->edges[1]->edges[0] = newFixDL;
            }

            if (lastVertexPos == pos)
            {
                b2Vec2 dir = vertexPos - lastVertexPos;
                dir.Normalize();
                b2Vec2 newVertexPos = lastVertexPos + innerLen * dir;

                dir *= -1.0f;

                float edgeSize = (origRightVertex - newVertexPos).Length();
                if (edgeSize > size)
                {
                    b2Vec2 newLeftVertex = newVertexPos + size * dir;

                    g_EdgeData* newFixDR = createEdge(origRightVertex, newLeftVertex, this, edgeBody);
                    newFixDR->gel = leftEdge->gel;
                    newFixDR->wall = leftEdge->wall;

                    portalEdgeData->edges[0] = newFixDR;
                    newFixDR->edges[1] = portalEdgeData;
                    newFixDR->edges[0] = leftEdge->edges[0];
                    if (leftEdge->edges[0])
                    {
                        leftEdge->edges[0]->edges[1] = newFixDR;
                    }

                    destrData->datas[0] = cdData;

                    b2Vec2 diff = newLeftVertex - newVertexPos;
                    diff.Normalize();
                    b2Vec2 mid = newVertexPos + size * 0.5f * diff;
                    b2Vec2 dir = rotateVec(diff, b2_pi * 0.5f);

                    Portal* portal = createPortal(mid, dir, size * 0.5, cdData->body);
                    portal->dumbFix->GetUserData().pointer = (uintptr_t)portalFixData;
                    portal->yFix[1]->GetUserData().pointer = (uintptr_t)portalFixData;
                    portal->yFix[0]->GetUserData().pointer = (uintptr_t)portalFixData;

                    portalEdgeData->fix = portal->dumbFix;

                    portal->destructionData = destrData;
                    //portal->body->SetLinearVelocity(edgeBody->GetLinearVelocity());

                    return portal;
                }
            }

            lastCdData = cdData;
            cdData->datas[0] = (createDestrData*)malloc(sizeof(createDestrData));
            cdData->datas[0]->datas[1] = cdData;
            cdData = cdData->datas[0];

            leftEdge = leftEdge->edges[0];

            remDist = size - (newVertexPos - origRightVertex).Length();

            leftPortalPoint = newVertexPos;

            break;
        }

        if (!leftEdge->edges[1])
        {    
            b2Vec2 v1 = getVertexByIndex(leftEdge, 1);
            if (lastVertexPos == pos)
            {
                b2Vec2 rightVertex = getVertexByIndex(leftEdge, 0);
                b2Vec2 dir = rightVertex - vertexPos;
                float edgeSize = dir.Length();

                if (edgeSize > size)
                {
                    b2Body* leftBody = leftEdge->fix->GetBody();
                    leftBody->DestroyFixture(leftEdge->fix);

                    dir.Normalize();
                    b2Vec2 newLeftVertex = vertexPos + size * dir;

                    g_EdgeData* newFixD = createEdge(rightVertex, newLeftVertex, this, leftBody);
                    newFixD->gel = leftEdge->gel;
                    newFixD->wall = leftEdge->wall;

                    portalEdgeData->edges[0] = newFixD;
                    newFixD->edges[1] = portalEdgeData;
                    newFixD->edges[0] = leftEdge->edges[0];
                    if (leftEdge->edges[0])
                    {
                        leftEdge->edges[0]->edges[1] = newFixD;
                    }

                    fillCdData(cdData, leftEdge);
                    destrData->datas[0] = cdData;

                    b2Vec2 diff = newLeftVertex - v1;
                    diff.Normalize();
                    b2Vec2 mid = v1 + size * 0.5f * diff;
                    b2Vec2 dir = rotateVec(diff, b2_pi * 0.5f);

                    Portal* portal = createPortal(mid, dir, size * 0.5, cdData->body);
                    portal->dumbFix->GetUserData().pointer = (uintptr_t)portalFixData;
                    portal->yFix[1]->GetUserData().pointer = (uintptr_t)portalFixData;
                    portal->yFix[0]->GetUserData().pointer = (uintptr_t)portalFixData;

                    portalEdgeData->fix = portal->dumbFix;

                    portal->destructionData = destrData;
                    //portal->body->SetLinearVelocity(leftBody->GetLinearVelocity());

                    return portal;
                }
            }

            remDist = size;

            leftPortalPoint = v1;

            break;
        }

        leftEdge = leftEdge->edges[1];

        if (leftEdge->gel != WHITE && leftEdge->wall != PORTALLABLE)
        {
            b2Vec2 v2 = getVertexByIndex(leftEdge, 0);
            g_EdgeData* nextEdgeData = leftEdge->edges[0];

            b2Body* nextBody = nextEdgeData->fix->GetBody();
            b2Vec2 rightVertex = getVertexByIndex(nextEdgeData, 0);

            g_EdgeData* leftEdgeTemp = leftEdge;
            leftEdge = leftEdge->edges[0];
            leftEdgeTemp->edges[0] = portalEdgeData;
            portalEdgeData->edges[1] = leftEdgeTemp;

            if (lastVertexPos == pos)
            {
                b2Vec2 dir = rightVertex - vertexPos;
                float edgeSize = dir.Length();
                if (edgeSize > size)
                {
                    nextBody->DestroyFixture(nextEdgeData->fix);

                    dir.Normalize();
                    b2Vec2 newLeftVertex = vertexPos + size * dir;

                    g_EdgeData* newFixD = createEdge(rightVertex, newLeftVertex, this, nextBody);
                    newFixD->gel = nextEdgeData->gel;
                    newFixD->wall = nextEdgeData->wall;

                    portalEdgeData->edges[0] = newFixD;
                    newFixD->edges[1] = portalEdgeData;
                    newFixD->edges[0] = nextEdgeData->edges[0];
                    if (nextEdgeData->edges[0])
                    {
                        nextEdgeData->edges[0]->edges[1] = newFixD;
                    }

                    fillCdData(cdData, nextEdgeData);
                    destrData->datas[0] = cdData;

                    b2Vec2 diff = newLeftVertex - v2;
                    diff.Normalize();
                    b2Vec2 mid = v2 + size * 0.5f * diff;
                    b2Vec2 dir = rotateVec(diff, b2_pi * 0.5f);

                    Portal* portal = createPortal(mid, dir, size * 0.5, cdData->body);
                    portal->dumbFix->GetUserData().pointer = (uintptr_t)portalFixData;
                    portal->yFix[1]->GetUserData().pointer = (uintptr_t)portalFixData;
                    portal->yFix[0]->GetUserData().pointer = (uintptr_t)portalFixData;

                    portalEdgeData->fix = portal->dumbFix;

                    portal->destructionData = destrData;
                    //portal->body->SetLinearVelocity(nextBody->GetLinearVelocity());

                    return portal;
                }
            }

            remDist = size;

            leftPortalPoint = v2;

            break;
        }

        lastVertexPos = vertexPos;
    }

    bool forIn = true;

    if (!leftEdge)
    {
        forIn = false;
    }
    else if (leftEdge->wall != PORTALLABLE && leftEdge->gel != WHITE)
    {
        portalEdgeData->edges[0] = leftEdge;
        leftEdge->edges[1] = portalEdgeData;
        forIn = false;
    }

    for (;forIn;)
    {
        b2Fixture* fix = leftEdge->fix;
        b2Body* fixBody = fix->GetBody();
        b2Vec2 v1 = getVertexByIndex(leftEdge, 1);
        b2Vec2 v2 = getVertexByIndex(leftEdge, 0);
        b2Vec2 dir = v2 - v1;
        float len = dir.Length();

        fillCdData(cdData, leftEdge);

        cdData->datas[1] = lastCdData;
        if (lastCdData)
        {
            lastCdData->datas[0] = cdData;
        }

        remDist -= len;

        if (remDist < 0.0f)
        {
            dir.Normalize();
            b2Vec2 newVertex = v2 + remDist * dir;
            fixBody->DestroyFixture(fix);
            g_EdgeData* eData = createEdge(v2, newVertex, this, fixBody);
            eData->edges[1] = portalEdgeData;
            eData->edges[0] = leftEdge->edges[0];
            if (leftEdge->edges[0])
            {
                leftEdge->edges[0]->edges[1] = eData;
            }
            portalEdgeData->edges[0] = eData;

            eData->gel = leftEdge->gel;
            eData->wall = leftEdge->wall;

            lastVertexPos = newVertex;

            break;
        }
        else
        {
            fixBody->DestroyFixture(fix);
        }

        if (!leftEdge->edges[0])
        {
            lastVertexPos = v2;
            break;
        }
        if (leftEdge->edges[0]->wall != PORTALLABLE && leftEdge->edges[0]->gel != WHITE)
        {
            portalEdgeData->edges[0] = leftEdge->edges[0];
            leftEdge->edges[0]->edges[1] = portalEdgeData;
            lastVertexPos = v2;
            break;
        }

        lastCdData = cdData;
        cdData->datas[0] = (createDestrData*)malloc(sizeof(createDestrData));
        cdData = cdData->datas[0];

        leftEdge = leftEdge->edges[0];
    }

    if (!forIn)
    {
        cdData = cdData->datas[1];
    }

    cdData->datas[0] = nullptr;
    destrData->datas[0] = cdData;

    b2Vec2 diff = lastVertexPos - leftPortalPoint;
    diff.Normalize();
    b2Vec2 mid = leftPortalPoint + size * 0.5f * diff;
    dir = rotateVec(diff, b2_pi * 0.5f);

    Portal* portal = createPortal(mid, dir, size * 0.5, cdData->body);
    portal->dumbFix->GetUserData().pointer = (uintptr_t)portalFixData;
    portal->yFix[1]->GetUserData().pointer = (uintptr_t)portalFixData;
    portal->yFix[0]->GetUserData().pointer = (uintptr_t)portalFixData;

    portalEdgeData->fix = portal->dumbFix;

    portal->destructionData = destrData;
    //portal->body->SetLinearVelocity(cdData->body->GetLinearVelocity());

    return portal;
}

PortalRay::PortalRay(PortalWorld* pWorld){
    this->pWorld = pWorld;
}

float PortalRay::ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction){
    if (fixture->IsSensor()) return 1.0f;

    bool isPortal = false;
    fixtureData* fData = (fixtureData*)fixture->GetUserData().pointer;
    if (fData){
        if (fData->type == F_PORTAL){
            Portal* p = (Portal*)fData->data;
            int side = p->getPointSide(point + normal);
            if (p->connections[side].size() != 0){
                isPortal = true;
            }
        }
        else if (fData->type == F_PORTAL_BODY){
            bodyData* bData = (bodyData*)fixture->GetBody()->GetUserData().pointer;
            PortalBody* pb = (PortalBody*)bData->data;
            PBFixtureData* pbfData = (PBFixtureData*)fData->data;
            std::vector<portalCollision*> collidingPortals = pbfData->pCollisions;
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
