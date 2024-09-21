#include "PortalBody.h"
#include "Portal.h"
#include <RotationJoint.h>
#include <PositionJoint.h>
#include <GameBase.h>

PortalWorld::PortalWorld() : b2World(b2Vec2(0, -11.15f))
{
    this->rayHandler = new PortalRay(this);
}

PortalWorld::~PortalWorld()
{
    for (int i = bodyIndices.size() - 1; i > -1; i--)
    {
        pbWorldData* worldData = bodyIndices.at(i);
        for (int j = worldData->pbIndices.size() - 1; j > -1; j--)
        {
            delete worldData->pbIndices.at(j);
        }

        for (auto shape : worldData->shapes)
        {
            delete shape;
        }

        for (auto mirShape : worldData->mirroredShapes)
        {
            delete mirShape;
        }

        delete worldData;
    }

    portalDestroyCallbacks.clear();

    for (int i = 0; i < portals.size(); i++)
    {
        destroyPortal(portals[i]);
        i--;
    }

    for (Portal* portal : portals)
    {
        delete portal;
    }

    delete rayHandler;
}

Portal* PortalWorld::createPortal(b2Vec2 pos, b2Vec2 dir, float size, b2Body* onBody)
{
    Portal* portal = new Portal(pos, dir, size, this, onBody);
    portals.push_back(portal);

    for (auto callback : portalCreateCallbacks)
    {
        callback(portal);
    }

    return portal;
}

void PortalWorld::destroyPortal(Portal* portal)
{
    for (auto callback : portalDestroyCallbacks)
    {
        callback(portal);
    }

    for (int i = 0; i < portals.size(); i++)
    {
        Portal* p = portals.at(i);
        if (p == portal)
        {
            portals.erase(portals.begin() + i);
            delete portal;
            break;
        }
    }
}

PortalBody* PortalWorld::createCloneBody0(pbWorldData* wData, b2Vec2 pos, float angle, bool isMirrored)
{
    b2BodyDef bDef;
    bDef.type = b2_dynamicBody;
    bDef.position = pos;
    bDef.angle = angle;
    
    b2Body* body = CreateBody(&bDef);

    std::vector<b2Shape*> shapes = isMirrored ? wData->mirroredShapes : wData->shapes;

    for (int i = shapes.size() - 1; i >= 0; i--)
    {
        b2FixtureDef fDef;
        fDef.density = wData->baseDensities[i];
        fDef.shape = shapes[i];
        body->CreateFixture(&fDef, false);
    }

    PortalBody* pb = new PortalBody(body, this);

    wData->pbIndices.push_back(pb);

    pb->isMirrored = isMirrored;
    pb->worldIndex = wData;

    for (auto pair : wData->createCallbacks)
    {
        pair.first(pb, pair.second);
    }

    return pb;
}

PortalBody* PortalWorld::createPortalBody(b2Body* body)
{
    PortalBody* pBody = new PortalBody(body, this);

    createNewPortalBody_i(pBody);

    return pBody;
}

void PortalWorld::createNewPortalBody_i(PortalBody* pBody)
{
    pbWorldData* pbData = new pbWorldData();
    pbData->pbIndices.push_back(pBody);
    bodyIndices.push_back(pbData);
    pBody->worldIndex = pbData;

    b2MassData mData;
    pBody->body->GetMassData(&mData);

    pbData->massCenter = mData.center;
    pbData->mirroredMassCenter = b2Vec2(-mData.center.x, mData.center.y);

    pbData->gravityScale = 1.0f;

    for (b2Fixture* fix = pBody->body->GetFixtureList(); fix; fix = fix->GetNext())
    {
        pbData->baseDensities.push_back(fix->GetDensity());

        if (fix->GetType() == b2Shape::e_polygon)
        {
            b2PolygonShape* pShape = (b2PolygonShape*)fix->m_shape;
            
            b2PolygonShape* copyShape = new b2PolygonShape();
            pbData->shapes.push_back(copyShape);
            memcpy(copyShape, pShape, sizeof(b2PolygonShape));
            
            b2PolygonShape* mirroredCopyShape = new b2PolygonShape();
            pbData->mirroredShapes.push_back(mirroredCopyShape);

            b2Vec2 mirroredVertices[8];
            int i = 0;
            for (; i < pShape->m_count; i++)
            {
                b2Vec2 vertex = pShape->m_vertices[i];
                mirroredVertices[i] = b2Vec2(-vertex.x, vertex.y);
            }

            mirroredCopyShape->Set(mirroredVertices, i);
        }
        else
        {
            b2CircleShape* cShape = (b2CircleShape*)fix->m_shape;

            b2CircleShape* copyShape = new b2CircleShape();
            pbData->shapes.push_back(copyShape);
            memcpy(copyShape, cShape, sizeof(b2CircleShape));

            b2CircleShape* mirroredCopyShape = new b2CircleShape();
            pbData->mirroredShapes.push_back(mirroredCopyShape);

            mirroredCopyShape->m_p = b2Vec2(-cShape->m_p.x, cShape->m_p.y);
            mirroredCopyShape->m_radius = cShape->m_radius;
        }
    }

    calculateOuterEdges(pbData->shapes, pbData->outerVertices, pbData->outerEdges, pbData->angles);
    calculateOuterEdges(pbData->mirroredShapes, pbData->mirroredOuterVertices, pbData->mirroredOuterEdges, pbData->mirroredAngles);
}

void PortalWorld::createPortalBody_i(PortalBody* pBody, PortalBody* baseBody)
{
    baseBody->worldIndex->pbIndices.push_back(pBody);
    pBody->worldIndex = baseBody->worldIndex;

    handledBodies.insert(pBody->worldIndex);

    for (auto pair : baseBody->worldIndex->createCallbacks)
    {
        pair.first(pBody, pair.second);
    }
}

void PortalWorld::PortalStep(float timeStep, int32 velocityIterations, int32 positionIterations)
{
    Step(timeStep, velocityIterations, positionIterations);
    
    for (pbWorldData* indices : bodyIndices)
    {
        // use indexing, creation happens in postHandle
        int size1 = indices->pbIndices.size();
        for (int j = 0; j < size1; j++)
        {
            PortalBody* body = indices->pbIndices.at(j);
            body->postHandle();
        }        
    }
    
    // for moving portals
    for (Portal* p : portals)
    {
        p->calculatePoints();
    }

    globalPostHandle();
}

void PortalWorld::globalPostHandle()
{
    for (PortalBody* portalBody : destroyBodies)
    {
        for (auto pair : portalBody->worldIndex->destroyCallbacks)
        {
            pair.first(portalBody, pair.second);
        }

        delete portalBody;
    }

    for (auto& hBody : handledBodies)
    {
        int numBodies = hBody->pbIndices.size();
        for (PortalBody* portBody : hBody->pbIndices)
        {
            int i = 0;
            for (b2Fixture* fix = portBody->body->GetFixtureList(); fix; fix = fix->GetNext())
            {
                fix->SetDensity(portBody->worldIndex->baseDensities[i++] / numBodies);
            }
            portBody->body->ResetMassData();
        }
    }

    handledBodies.clear();
    destroyBodies.clear();
}

bool PortalWorld::createCloneBody(Portal* collPortal, int side, PortalBody* pBody)
{
    bool retVal = false;

    pbWorldData* pBodyData = pBody->worldIndex;
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
        def.angle = body1->GetAngle() + angleRot;
        if (c->isReversed)
        {
            float pAngle = calcAngle2(collPortal->dir);
            float newAng = body1->GetAngle();
            newAng = newAng - pAngle;
            newAng = -newAng + pAngle + angleRot - b2_pi;

            def.angle = newAng;
        }
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

        npb->isMirrored = pBody->isMirrored ^ c->isReversed;

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
            for (bodyCollisionStatus* bcs : pbB->bodyMaps)
            {
                if (bcs->connection == c)
                {
                    b2RevoluteJointDef jDef;

                    float angle1 = body2->GetAngle();
                    float angle2 = bcs->body->body->GetAngle();

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

                        jDef.referenceAngle = 0.0;
                    }
                    else
                    {
                        jDef.bodyA = bcs->body->body;
                        jDef.bodyB = body2;

                        jDef.localAnchorA = length2 * b2Vec2(cos(angle2), sin(angle2));
                        jDef.localAnchorA = bcs->body->body->GetLocalPoint(bcs->body->body->GetPosition() + jDef.localAnchorA);

                        jDef.localAnchorB = length1 * b2Vec2(cos(angle1), sin(angle1));
                        jDef.localAnchorB = npb->body->GetLocalPoint(npb->body->GetPosition() + jDef.localAnchorB);

                        jDef.referenceAngle = 0.0;
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

        bodyCollisionStatus* bcs = new bodyCollisionStatus;
        bcs->body = pBody;

        for (portalConnection* conn : portal2->connections[c->side2])
        {
            if (conn->portal2 == collPortal && conn->side2 == c->side1)
            {
                bcs->connection = conn;
                conn->portal1->collidingBodies[conn->side1].insert(npb);
                break;
            }
        }

        bodyCollisionStatus* t_bcs = new bodyCollisionStatus;
        t_bcs->body = npb;
        npb->fixtures.resize(pBody->fixtures.size());
        t_bcs->connection = c;

        bool allOut = true;
        int fixMargin = 0;
        for (int i = pBody->fixtures.size() - 1; i > -1; i--)
        {
            b2Fixture* fix = pBody->fixtures.at(i);
            b2Fixture* f;
            b2FixtureDef fDef;
            fDef.density = fix->GetDensity();
            fDef.restitution = fix->GetRestitution();
            fDef.friction = fix->GetFriction();
            fDef.isSensor = fix->IsSensor();
            if (c->isReversed ^ pBody->isMirrored)
            {
                fDef.shape = pBodyData->mirroredShapes[i];
            }
            else
            {
                fDef.shape = pBodyData->shapes[i];
            }
            f = body2->CreateFixture(&fDef, false);

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
            if (status == 1)
            {
                int fixSide = collPortal->getFixtureSide(fix);
                
                bool rayCheck = false;
                bool release = false;
                if (fixSide == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                else
                {
                    bool check = collPortal->rayCheck(fix);
                    if (!check)
                    {
                        release = true;
                    }
                    else
                    {
                        rayCheck = true;
                    }
                }

                if (rayCheck)
                {
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
                else
                {
                    free(col);
                }
            }
            else if (status == 2)
            {
                fixMargin -= 2;
                if (status2 == 1)
                {
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = -2;
                    pbfData2->pCollisions.push_back(col);

                    bool rayCheck;
                    int fixSide = collPortal->getFixtureSide(fix);
                    if (fixSide == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                    else rayCheck = 1;

                    if (!rayCheck)
                    {
                        col->status++;
                    }
                }
                else if (status2 < 0){
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = status2 - 2;
                    pbfData2->pCollisions.push_back(col);

                    if (status2 % 2 == 0)
                    {
                        bool rayCheck;
                        int fixSide = collPortal->getFixtureSide(fix);
                        if (fixSide == (1 ^ c->side1)) rayCheck = collPortal->rayCheck(fix);
                        else rayCheck = 1;

                        if (!rayCheck)
                        {
                            col->status++;
                        }
                    }
                }
                else
                {
                    col->portal = portal2;
                    col->side = c->side2;
                    col->status = -1;
                    pbfData2->pCollisions.push_back(col);
                }
            }
            else
            {
                free(col);
            }

            if (status != 1 && status2 != 1)
            {
                //allOut = false;
            }
        }

        npb->numFixtures = pBody->numFixtures;
        npb->outFixtures[c->side2][portal2] = npb->numFixtures + fixMargin;
        pBody->bodyMaps.push_back(t_bcs);
        npb->bodyMaps.push_back(bcs);

        if (allOut)
        {
            retVal = true;
        }

        createPortalBody_i(npb, pBody);

        connectBodies(pBody, npb, c);

        b2Vec2 lvToSet = rotateVec(speed, angleRot);
        if (c->isReversed)
        {
            body2->SetAngularVelocity(-body1->GetAngularVelocity() /*+ collPortal->getAngVel() + portal2->getAngVel()*/);
            lvToSet = mirror(dir2, lvToSet);
        }
        else
        {
            body2->SetAngularVelocity(body1->GetAngularVelocity() /*- collPortal->getAngVel() + portal2->getAngVel()*/);
        }

        b2Vec2 relativePortalPos2 = c->portal2->pos - c->portal2->body->GetPosition();
        b2Vec2 linSpeedAddition2 = c->portal2->body->GetAngularVelocity() * PortalWorld::rotateVec(relativePortalPos2, b2_pi * 0.5f);
        b2Vec2 portalLinSpeed2 = c->portal2->getLinVel() + linSpeedAddition2;

        lvToSet += portalLinSpeed2;

        body2->SetLinearVelocity(lvToSet);
    }

    return retVal;
}

void PortalWorld::connectBodies(PortalBody* body1, PortalBody* body2, portalConnection* connection)
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

Portal* PortalWorld::createPortalWithChecks(b2Fixture* edgeFix, b2Vec2 pos, float size)
{
    fixtureData* fixData = (fixtureData*)edgeFix->GetUserData().pointer;
    s_edgeLocalData* eLocalData = (s_edgeLocalData*)fixData->data;
    s_edgeGlobalData* eGlobalData = eLocalData->globalData;

    int rangeSize = eLocalData->globalRange[1] - eLocalData->globalRange[0];
    int requiredRangeSize = std::round(size * 10.0f);
    requiredRangeSize += requiredRangeSize % 2 - 1;

    b2EdgeShape* eShape = (b2EdgeShape*)edgeFix->GetShape();
    pos = edgeFix->GetBody()->GetLocalPoint(pos);
    float dist = (pos - eGlobalData->localPoints[0]).Length() * 10.0f;

    int startIndex = std::min(std::max((int)std::floor(dist), eLocalData->globalRange[0]), eLocalData->globalRange[1]);

    int leftCount = 0;
    int rightCount = 0;

    while (true)
    {
        if (startIndex + leftCount + 1 <= eLocalData->globalRange[1])
        {
            s_unitWallData& uData = eGlobalData->unitWalls[startIndex + leftCount + 1];
            if (uData.wallType == PORTALLABLE || (uData.gelAmount > 0.5 && uData.gelType & WHITE))
            {
                leftCount++;

                if (leftCount + rightCount == requiredRangeSize)
                {
                    break;
                }
            }
        }

        if (startIndex - rightCount - 1 >= eLocalData->globalRange[0])
        {
			s_unitWallData& uData = eGlobalData->unitWalls[startIndex - rightCount - 1];
            if (uData.wallType == PORTALLABLE || (uData.gelAmount > 0.5 && uData.gelType & WHITE))
            {
				rightCount++;

                if (leftCount + rightCount == requiredRangeSize)
                {
                    break;
                }
			}
		}
    }

    int range[2] = { startIndex - rightCount, startIndex + leftCount };

    int prevRange[2] = { eLocalData->globalRange[0], eLocalData->globalRange[1] };
    b2Fixture* prevPNFixs[2] = { eLocalData->prevNextFixtures[0], eLocalData->prevNextFixtures[1] };

    b2Vec2 portalCreatePos;
    if (leftCount == rightCount)
    {
        portalCreatePos = pos;
    }
    else
    {
        if (range[0] == eLocalData->globalRange[0])
        {
            float oneDimensionLen = range[0] * 0.1f + size * 0.5;
            portalCreatePos = eGlobalData->localPoints[0] + oneDimensionLen * eGlobalData->dir;
        }
        else if (range[1] == eLocalData->globalRange[1])
        {
            float oneDimensionLen = (range[1] + 1.0f) * 0.1f - size * 0.5;
            portalCreatePos = eGlobalData->localPoints[0] + oneDimensionLen * eGlobalData->dir;
        }
        else
        {
            float oneDimensionLen = range[0] * 0.1f + size * 0.5;
            portalCreatePos = eGlobalData->localPoints[0] + oneDimensionLen * eGlobalData->dir;
        }
    }

    portalCreatePos = edgeFix->GetBody()->GetWorldPoint(portalCreatePos);

    Portal* portal = createPortal(portalCreatePos, rotateVec(eGlobalData->perpDir, edgeFix->GetBody()->GetAngle()), size * 0.5f, edgeFix->GetBody());
    portal->range[0] = range[0];
    portal->range[1] = range[1];

    {
        b2EdgeShape newShape;
        newShape.m_oneSided = false;
        newShape.m_vertex0 = eShape->m_vertex0;
        newShape.m_vertex1 = eShape->m_vertex1;
        newShape.m_vertex2 = ((b2EdgeShape*)portal->yFix[0]->GetShape())->m_vertex1;
        //newShape.m_vertex3 = eShape->m_vertex1;

        b2Fixture* fix = edgeFix->GetBody()->CreateFixture(&newShape, 0.0f);
        
        eLocalData->globalRange[0] = prevRange[0];
        eLocalData->globalRange[1] = range[0] - 1;
        
        fix->GetUserData().pointer = (uintptr_t)fixData;

        eLocalData->prevNextFixtures[1] = portal->midFixture;
        portal->sideFixtures[0] = fix;

        if (eLocalData->prevNextFixtures[0])
        {
            fixtureData* nextFData = (fixtureData*)eLocalData->prevNextFixtures[0]->GetUserData().pointer;
            if (nextFData && nextFData->type == F_PORTAL)
            {
                Portal* p = (Portal*)nextFData->data;
                p->sideFixtures[1] = fix;
                p->range[1] = range[0];
            }
            // else if (nextFData && nextFData->type == F_EDGE)
            // {
            //     s_edgeLocalData* nextELocal = (s_edgeLocalData*)nextFData->data;
            //     nextELocal->prevNextFixtures[1] = fix;
            //     nextELocal->globalRange[1] = range[0];
            // }
        }

        fixData = nullptr;
    }

    {
        if (!fixData)
        {
            fixData = new fixtureData();
            fixData->type = F_EDGE;
            eLocalData = new s_edgeLocalData(eGlobalData);
            fixData->data = eLocalData;
        }

        b2EdgeShape newShape;
        newShape.m_oneSided = false;
        //newShape.m_vertex0 = eShape->m_vertex2;
        newShape.m_vertex1 = ((b2EdgeShape*)portal->yFix[1]->GetShape())->m_vertex1;
        newShape.m_vertex2 = eShape->m_vertex2;
        newShape.m_vertex3 = eShape->m_vertex3;

        b2Fixture* fix = edgeFix->GetBody()->CreateFixture(&newShape, 0.0f);

        eLocalData->globalRange[0] = range[1] + 1;
        eLocalData->globalRange[1] = prevRange[1];

        fix->GetUserData().pointer = (uintptr_t)fixData;

        eLocalData->prevNextFixtures[0] = portal->midFixture;
        eLocalData->prevNextFixtures[1] = prevPNFixs[1];
        portal->sideFixtures[1] = fix;

        if (eLocalData->prevNextFixtures[1])
        {
            fixtureData* nextFData = (fixtureData*)eLocalData->prevNextFixtures[1]->GetUserData().pointer;
            if (nextFData && nextFData->type == F_PORTAL)
            {
                Portal* p = (Portal*)nextFData->data;
                p->sideFixtures[0] = fix;
                p->range[0] = range[1];
            }
            // else if (nextFData && nextFData->type == F_EDGE)
            // {
            //     printf("herere\n");
            //     s_edgeLocalData* nextELocal = (s_edgeLocalData*)nextFData->data;
            //     nextELocal->prevNextFixtures[0] = fix;
            //     nextELocal->globalRange[0] = range[1];
            // }
        }
    }

    edgeFix->GetBody()->DestroyFixture(edgeFix);

    return portal;
}