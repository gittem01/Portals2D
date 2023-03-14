#pragma once

#include <WindowHandler.h>
#include <PortalGunRayCast.h>
#include <GameBase.h>
#include <Camera.h>
#include "PortalBody.h"
#include "Portal.h"

class TestPlayer
{

public:
    Portal* portalA;
    Portal* portalB;
    float lastPortalCreationTime;

    std::vector<PortalBody*>* pBody;
    std::vector<PortalBody*>* gun;
    PortalWorld* pWorld;
    WindowHandler* wh;
    b2RevoluteJoint* oldJoint;
    PortalBody* oldPortalBody;
    b2Vec2 size;

    bool haveContact;
    bool onPlatform;
    b2Body* contactBody;
    b2Body* platformBody;
    b2BodyType contactType;
    b2BodyType rightCollisionType = b2_dynamicBody;
    b2BodyType leftCollisionType = b2_dynamicBody;

    PortalBody* lastBody;
    bool bodySwitch;
    float totalTime = 0.1f;
    float switchTimer;

    int lastKey;
    int keyBeforeSwitch;

    bool right;
    bool left;
    int lastDoubleKey;

    float onAirFor;

    b2Vec2 lastDirs[2];

    TestPlayer(PortalWorld* pWorld, WindowHandler* wh, b2Vec2 pos=b2Vec2(0, 0), b2Vec2 size=b2Vec2(0.8f, 1.5f)){
        this->pWorld = pWorld;
        this->wh = wh;
        this->size = size;
        createBody(pos);

        portalA = nullptr;
        portalB = nullptr;
        lastPortalCreationTime = -1.0f;

        haveContact = false;
        onPlatform = false;
        contactBody = nullptr;
        platformBody = nullptr;
        oldJoint = nullptr;
        contactType = (b2BodyType)INT_MAX;

        bodySwitch = false;
        lastBody = nullptr;
        lastKey = -1;
        keyBeforeSwitch = -1;

        right = false;
        left = false;
        lastDoubleKey = -1;

        onAirFor = 0.0f;
    }

    void createBody(b2Vec2 pos){
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = pos;
        bodyDef.fixedRotation = true;
        bodyDef.linearDamping = 0.0f;
        bodyDef.bullet = true;
        bodyDef.allowSleep = false;

        b2Body* body = pWorld->CreateBody(&bodyDef);

        b2PolygonShape boxShape;
        boxShape.SetAsBox(size.x / 2.0f, size.y / 2.0f);

        b2FixtureDef fDef;
        fDef.density = 1.0f;
        fDef.shape = &boxShape;
        fDef.friction = 0.0f;
        fDef.restitution = 0.0f;

        body->CreateFixture(&fDef);

        PortalBody* pb = pWorld->createPortalBody(body);

        bodyData* bData = (bodyData*)body->GetUserData().pointer;
        g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
        physicsData->type = TEST_PLAYER;
        physicsData->data = this;
        bData->extraData = physicsData;

        pBody = &pb->worldIndex->pbIndices;

        glm::vec2 gunSize(0.3f, 1.1f);

        bodyDef.type = b2_dynamicBody;
        bodyDef.position = pos;
        bodyDef.fixedRotation = false;
        bodyDef.linearDamping = 0.0f;
        bodyDef.bullet = true;
        bodyDef.allowSleep = false;

        body = pWorld->CreateBody(&bodyDef);

        boxShape.SetAsBox(gunSize.x / 2.0f, gunSize.y / 2.0f);

        fDef.density = 0.1f;
        fDef.shape = &boxShape;
        fDef.friction = 0.0f;
        fDef.restitution = 0.0f;

        body->CreateFixture(&fDef);

        pb = pWorld->createPortalBody(body);

        gun = &pb->worldIndex->pbIndices;

        b2RevoluteJointDef jDef;
        jDef.bodyA = pBody->at(0)->body;
        jDef.bodyB = gun->at(0)->body;
        jDef.collideConnected = false;
        jDef.localAnchorA = b2Vec2(0.0f, 0.35f);
        jDef.localAnchorB = b2Vec2(0.0f, -0.35f);
        jDef.calculateAnchorHelpers();

        pWorld->CreateJoint(&jDef);
    }

    void handleForces(b2Contact* contact, b2Fixture* fix2){
        b2WorldManifold wManifold;
        contact->GetWorldManifold(&wManifold);
        b2Vec2 normal = wManifold.normal;

        contact->SetRestitution(0.0f);
    }

    void reportContact(b2Contact* contact, b2Body* contactingBody, b2Fixture* fix2){
        if (!contact->IsEnabled() || fix2->IsSensor())
        {
            return;
        }

        PortalBody* contactingPBody = (PortalBody*)((bodyData*)contactingBody->GetUserData().pointer)->data;

        haveContact = true;

        b2WorldManifold wManifold;
        contact->GetWorldManifold(&wManifold);
        b2Vec2 normal = wManifold.normal;

        b2Vec2 cPos = wManifold.points[0];

        if (contact->GetManifold()->pointCount > 1)
        {
            cPos += wManifold.points[1];
            cPos *= 0.5f;
        }

        float tHold = 0.1f;
        b2Vec2 bodyPos = contactingPBody->body->GetPosition();
        b2Vec2 botVector = b2Vec2(0.0f, -1.0f);
        botVector = pWorld->rotateVec(botVector, contactingPBody->offsetAngle + contactingBody->GetAngle());

        float botY = bodyPos.y + (size.y / 2.0f) * botVector.y;
        if (cPos.y < botY + tHold && cPos.y > botY - tHold)
        {
            if (abs(normal.x) <= 0.6f)
            {
                contactType = fix2->GetBody()->GetType();
                contactBody = fix2->GetBody();
                if (fix2->GetBody()->GetType() == b2_dynamicBody)
                {
                    if (fix2->GetDensity() == 0.0f &&
                        (contact->GetFixtureA() == pBody->at(0)->body->GetFixtureList() || contact->GetFixtureB() == pBody->at(0)->body->GetFixtureList()))
                    {
                        onPlatform = true;
                        platformBody = fix2->GetBody();
                    }
                }
            }
            else if (normal.x > 0.6f)
            {
                leftCollisionType = fix2->GetBody()->GetType();
            }
            else
            {
                rightCollisionType = fix2->GetBody()->GetType();
            }
        }
    }

    void camHandle()
    {
        if (pBody->size() > 1)
        {
            b2Vec2 p1 = pBody->at(0)->body->GetPosition();
            b2Vec2 p2 = pBody->at(1)->body->GetPosition();

            glm::vec2 diff = glm::vec2(abs(p2.x - p1.x), abs(p2.y - p1.y)) / 2.0f + 5.0f;
            glm::vec2 mid = glm::vec2(p2.x + p1.x, p2.y + p1.y) / 2.0f;
            
            glm::vec2 posDiff = mid - glm::vec2(wh->cam->pos.x, wh->cam->pos.y);

            wh->cam->pos += glm::vec3(posDiff.x, posDiff.y, 0.0f) * 0.01f;
        }
        else
        {
            b2Vec2 bp = pBody->at(0)->body->GetPosition();
            glm::vec2 diff = glm::vec2(bp.x, bp.y) - glm::vec2(wh->cam->pos.x, wh->cam->pos.y);
            wh->cam->pos += glm::vec3(diff.x, diff.y, 0.0f) * 0.01f;
        }

        wh->cam->updateOrtho();
    }

    void handleBodyRotation()
    {
        PortalBody* pb = pBody->at(0);
        float ang = PortalWorld::normalizeAngle(pb->offsetAngle + pb->body->GetAngle());
        if (ang > 0.03f)
        {
            for (PortalBody* l_portalBody : *pBody)
            {
                l_portalBody->body->SetAngularVelocity(0.0f);
            }
            pb->body->SetAngularVelocity(-10.0f * (abs(ang) / b2_pi));
        }
        else if (ang < -0.03f)
        {
            for (PortalBody* l_portalBody : *pBody)
            {
                l_portalBody->body->SetAngularVelocity(0.0f);
            }
            pb->body->SetAngularVelocity(+10.0f * (abs(ang) / b2_pi));
        }
        else
        {
            pb->body->SetTransform(pb->body->GetPosition(), pb->body->GetAngle() - ang);
            pb->body->SetAngularVelocity(0.0f);
        }
    }

    void handleGunRotation()
    {
        float ang;
        float mult = 1.0f;
        PortalBody* pb = pBody->at(0);
        PortalBody* gunBody = nullptr;
        PortalBody* playerBody = nullptr;
        b2RevoluteJoint* rj = nullptr;
        bool found = false;
        for (b2JointEdge* edge = pb->body->GetJointList(); edge != nullptr; edge = edge->next)
        {
            if (edge->joint->GetType() != e_revoluteJoint) continue;

            b2Body* otherBody;
            if (edge->joint->GetBodyA() == pb->body)
            {
                otherBody = edge->joint->GetBodyB();
            }
            else
            {
                otherBody = edge->joint->GetBodyA();
            }
            bodyData* data = (bodyData*)otherBody->GetUserData().pointer;
            if (data->type != B_PORTAL_BODY) continue;

            PortalBody* possibleGun = (PortalBody*)data->data;
            if (&possibleGun->worldIndex->pbIndices == gun)
            {
                found = true;
                gunBody = possibleGun;
                rj = (b2RevoluteJoint*)edge->joint;

                b2Vec2 gunCenter = pb->body->GetPosition() + b2Vec2(0.0f, 0.35f);
                glm::vec2 mousePos = wh->cam->getMouseCoords();
                b2Vec2 diff = b2Vec2(mousePos.x - gunCenter.x, mousePos.y - gunCenter.y);
                
                ang = gunBody->offsetAngle + gunBody->body->GetAngle() - PortalWorld::calcAngle2(diff) + b2_pi * 0.5f;
                ang = PortalWorld::normalizeAngle(ang);

                playerBody = pb;
                break;
            }
        }

        if (!found)
        {
            b2RevoluteJoint* jointFound;
            gunBody = gun->at(0);
            for (b2JointEdge* edge = gunBody->body->GetJointList(); edge != nullptr; edge = edge->next)
            {
                if (edge->joint->GetType() != e_revoluteJoint) continue;

                b2Body* otherBody;
                if (edge->joint->GetBodyA() == gunBody->body)
                {
                    otherBody = edge->joint->GetBodyB();
                }
                else
                {
                    otherBody = edge->joint->GetBodyA();
                }

                bodyData* data = (bodyData*)otherBody->GetUserData().pointer;
                if (data->type == B_PORTAL_BODY && data->extraData != nullptr)
                {
                    playerBody = (PortalBody*)data->data;
                    jointFound = (b2RevoluteJoint*)edge->joint;
                    break;
                }
            }

            if (playerBody)
            {
                portalConnection* pConnection = nullptr;
                for (bodyCollisionStatus* stat : *pb->bodyMaps)
                {
                    if (stat->body == playerBody)
                    {
                        pConnection = stat->connection;
                        break;
                    }
                }

                if (pConnection)
                {
                    b2Vec2 gunCenter = pb->body->GetPosition() + b2Vec2(0.0f, 0.35f);
                    glm::vec2 mousePos = wh->cam->getMouseCoords();

                    b2Vec2 diff = b2Vec2(mousePos.x - gunCenter.x, mousePos.y - gunCenter.y);
                    b2Vec2 relPos = PortalWorld::getRelativePortalPosition_L(diff, pConnection);
                    
                    ang = gunBody->offsetAngle + gunBody->body->GetAngle() - PortalWorld::calcAngle2(relPos) - b2_pi * 0.5f;
                    ang = PortalWorld::normalizeAngle(ang);

                    rj = jointFound;
                }
            }
        }

        if (rj)
        {
            const float threshold = 0.0f;
            const float speedMult = 30.0f;
            rj->EnableMotor(true);
            rj->SetMaxMotorTorque(300.0f * gunBody->body->GetMass());

            if (playerBody->isMirrored)
            {
                mult = -1.0f;
            }

            if (ang > threshold)
            {
                rj->SetMotorSpeed(-speedMult * +ang * mult);
            }
            else
            {
                rj->SetMotorSpeed(+speedMult * -ang * mult);
            }
        }

        if (oldJoint != rj && oldJoint != nullptr)
        {
            for (PortalBody* l_portalBody : *pBody)
            {
                if (l_portalBody == oldPortalBody)
                {
                    for (b2JointEdge* edge = oldPortalBody->body->GetJointList(); edge != nullptr; edge = edge->next)
                    {
                        if (edge->joint == oldJoint)
                        {
                            oldJoint->EnableMotor(false);
                            break;
                        }
                    }
                    break;
                }
            }
        }

        if (rj)
        {
            oldJoint = rj;
            oldPortalBody = playerBody;
        }
    }

    bool canCreatePortal(b2Fixture* edgeFix, b2Vec2 pos, float size, Portal* oldPortal)
    {
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
                b2Fixture* fix = rightEdgeData->fix;
                b2EdgeShape* eShape = (b2EdgeShape*)fix->GetShape();
                b2Vec2 vertexPos = fix->GetBody()->GetWorldPoint(eShape->m_vertex1);
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
                        rightEdgeData = eData;
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
                b2Fixture* fix = leftEdgeData->fix;
                b2EdgeShape* eShape = (b2EdgeShape*)fix->GetShape();
                b2Vec2 vertexPos = fix->GetBody()->GetWorldPoint(eShape->m_vertex2);
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
                        leftEdgeData = eData;
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

        if (leftEdgeData && leftEdgeData->wall == PORTAL_ITSELF)
        {
            b2Fixture* nextFix = leftEdgeData->fix->GetNext();
            fixtureData* fData = (fixtureData*)nextFix->GetUserData().pointer;
            if ((Portal*)fData->data == oldPortal)
            {
                return true;
            }
        }
        if (rightEdgeData && rightEdgeData->wall == PORTAL_ITSELF)
        {
            b2Fixture* nextFix = rightEdgeData->fix->GetNext();
            fixtureData* fData = (fixtureData*)nextFix->GetUserData().pointer;
            if ((Portal*)fData->data == oldPortal)
            {
                return true;
            }
        }

        float totalLen = rightSize + leftSize;

        if (totalLen < size - 0.000001f)
        {
            return false;
        }

        return true;
    }

    void handlePortalCD()
    {
        const float portalSize = 2.5f;
        const float rayLength = 100.0f;
        PortalBody* portalGun = gun->at(0);
        float vecAngle = b2_pi * 0.5f;

        vecAngle += portalGun->body->GetAngle() + portalGun->offsetAngle;
        b2Vec2 vec = b2Vec2(cos(vecAngle) * rayLength, sin(vecAngle) * rayLength);
        b2Vec2 unitVec = b2Vec2(cos(vecAngle), sin(vecAngle));

        PortalGunRayCast rayCaster = PortalGunRayCast(pWorld);
        b2Vec2 rayPoint1 = portalGun->body->GetPosition();
        b2Vec2 rayPoint2 = portalGun->body->GetPosition() + vec;

        rayCaster.cast(rayPoint1, rayPoint2);

        if (rayCaster.found)
        {
            if (glfwGetTime() - lastPortalCreationTime > -0.002f)
            {
                if (wh->keyData[GLFW_KEY_Q] == 2.0f)
                {
                    if (canCreatePortal(rayCaster.edgeFix, rayCaster.rayPoint, portalSize, portalA))
                    {
                        b2Body* fixBody = rayCaster.edgeFix->GetBody();
                        if (portalA)
                        {
                            pWorld->destroyPortal(portalA);
                            b2Vec2 dir = rayPoint2 - rayPoint1;
                            dir.Normalize();
                            dir *= 0.1f;
                            rayCaster.cast(rayCaster.rayPoint - dir, rayCaster.rayPoint + dir);
                            portalA = nullptr;
                        }

                        Portal* portal = pWorld->createPortalWithChecks(rayCaster.edgeFix, rayCaster.rayPoint, rayCaster.rayNormal, portalSize);

                        if (portal)
                        {
                            portal->reviveAround();
                            portalA = portal;

                            if (portalB)
                            {
                                portalA->connect(portalB);
                                portalB->reviveAround();
                            }
                        }
                    }

                    lastPortalCreationTime = glfwGetTime();
                }
                else if (wh->keyData[GLFW_KEY_E] == 1.0f)
                {
                    if (canCreatePortal(rayCaster.edgeFix, rayCaster.rayPoint, portalSize, portalB))
                    {
                        b2Body* fixBody = rayCaster.edgeFix->GetBody();
                        if (portalB)
                        {
                            pWorld->destroyPortal(portalB);
                            b2Vec2 dir = rayPoint2 - rayPoint1;
                            dir.Normalize();
                            dir *= 0.1f;
                            rayCaster.cast(rayCaster.rayPoint - dir, rayCaster.rayPoint + dir);
                            portalB = nullptr;
                        }

                        Portal* portal = pWorld->createPortalWithChecks(rayCaster.edgeFix, rayCaster.rayPoint, rayCaster.rayNormal, portalSize);

                        if (portal)
                        {
                            portal->reviveAround();
                            portalB = portal;

                            if (portalA)
                            {
                                portalB->connect(portalA);
                                portalA->reviveAround();
                            }
                        }
                    }
                    lastPortalCreationTime = glfwGetTime();
                }
            }
        }
    }

    void updateReset()
    {
        haveContact = false;
        onPlatform = false;
        contactBody = nullptr;
        platformBody = nullptr;
        right = false;
        left = false;
        contactType = (b2BodyType)INT_MAX; // NULL
        rightCollisionType = b2_dynamicBody;
        leftCollisionType = b2_dynamicBody;
        lastBody = pBody->at(0);
    }

    void update(float dt, int totalIter)
    {
        const float maxSpeed = 5.0f;
        float speedOffset = 0.0f;
        PortalBody* pb = pBody->at(0);

        if (pb != lastBody && lastBody != nullptr)
        {
            bodySwitch = true;
            switchTimer = glfwGetTime();
            float diff = abs(lastDirs[0].x + lastDirs[1].x);
            if (lastKey != -1 && diff > 1.0f){
                keyBeforeSwitch = lastKey;
            }
        }
        if (bodySwitch){
            if (glfwGetTime() - switchTimer > totalTime){
                bodySwitch = false;
                lastBody = nullptr;
            }
        }

        if (onPlatform)
        {
            speedOffset = platformBody->GetLinearVelocity().x;
        }

        if (wh->keyData[GLFW_KEY_D] && wh->keyData[GLFW_KEY_A]){
            if (lastDoubleKey == -1){
                right = true;
            }
            else if (lastDoubleKey == GLFW_KEY_A){
                right = true;
            }
            else if (lastDoubleKey == GLFW_KEY_D){
                left = true;
            }
        }
        else if (wh->keyData[GLFW_KEY_D]){
            right = true;
        }
        else if (wh->keyData[GLFW_KEY_A]){
            left = true;
        }

        if (pBody->size() > 1)
        {
            PortalBody* b1 = pBody->at(0);
            PortalBody* b2 = pBody->at(1);
            fixtureData* fixData1 = (fixtureData*)b1->body->GetFixtureList()->GetUserData().pointer;
            PBFixtureData* pbfData1 = (PBFixtureData*)fixData1->data;
            fixtureData* fixData2 = (fixtureData*)b2->body->GetFixtureList()->GetUserData().pointer;
            PBFixtureData* pbfData2 = (PBFixtureData*)fixData2->data;

            if (pbfData1->pCollisions.size() > 0 && pbfData2->pCollisions.size() > 0)
            {
                portalCollision* pc1 = pbfData1->pCollisions.at(0);
                portalCollision* pc2 = pbfData2->pCollisions.at(0);

                lastDirs[0] = pc1->side ? -pc1->portal->dir : pc1->portal->dir;
                lastDirs[1] = pc2->side ? -pc2->portal->dir : pc2->portal->dir;
            }
        }

        if (!(wh->keyData[GLFW_KEY_A] && wh->keyData[GLFW_KEY_D]))
        {
            if (wh->keyData[GLFW_KEY_A]){
                lastDoubleKey = GLFW_KEY_A;
            }
            else if (wh->keyData[GLFW_KEY_D]){
                lastDoubleKey = GLFW_KEY_D;
            }
            else{
                lastDoubleKey = -1;
            }
        }

        float mult = 1.0f;
        if (left || right)
        {
            if (right)
            {
                if (keyBeforeSwitch == GLFW_KEY_D)
                {
                    mult = -1.0f;
                }
                else
                {
                    keyBeforeSwitch = -1;
                }
            }
            else if (left)
            {
                if (keyBeforeSwitch == GLFW_KEY_A)
                {
                    mult = -1.0f;
                }
                else
                {
                    keyBeforeSwitch = -1;
                }
            }
        }
        else
        {
            keyBeforeSwitch = -1;
        }

        lastKey = -1;
        b2Vec2 lv = pb->body->GetLinearVelocity();
        //printf("%f, %f\n", lv.x, lv.y);
        if (!left && !right && onPlatform)
        {
            float diff = platformBody->GetLinearVelocity().x - lv.x;
            lv.x += diff * dt * 15.0f * pBody->size();
        }
        if (left || right)
        {
            const float groundMult = 90.0f;
            const float airMult = 20.0f;
            const float fwdGrndSlw = 5.0f;
            if (right)
            {
                float addition;
                if (contactBody != nullptr)
                {
                    addition = groundMult * dt * pBody->size() * mult;
                }
                else if (rightCollisionType == b2_dynamicBody)
                {
                    addition = airMult * dt * pBody->size() * mult;
                }
                else
                {
                    addition = 0.0f;
                }

                if (addition != 0.0f)
                {
                    lv.x += addition;
                    if (lv.x > maxSpeed + speedOffset)
                    {
                        if (contactBody != nullptr)
                        {
                            if (lv.x - addition > maxSpeed + speedOffset)
                            {
                                lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * fwdGrndSlw);
                            }
                            else
                            {
                                lv.x = maxSpeed + speedOffset;
                            }
                        }
                        lv.x = fmax(maxSpeed + speedOffset, lv.x - addition);
                    }
                    else if (lv.x < -maxSpeed + speedOffset)
                    {
                        if (rightCollisionType == b2_dynamicBody && mult > 0.0f)
                        {
                            lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * 5.0f);
                            lv.x = lv.x - addition;
                        }
                        else
                        {
                            if (contactBody != nullptr)
                            {
                                if (lv.x - addition < -maxSpeed + speedOffset)
                                {
                                    lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * fwdGrndSlw);
                                }
                                else
                                {
                                    lv.x = -maxSpeed + speedOffset;
                                }
                            }
                            lv.x = fmin(-maxSpeed + speedOffset, lv.x - addition);
                        }
                    }
                }
                
                lastKey = GLFW_KEY_D;
            }
            if (left)
            {
                float subtraction;
                if (contactBody != nullptr)
                {
                    subtraction = groundMult * dt * pBody->size() * mult;
                }
                else if (leftCollisionType == b2_dynamicBody)
                {
                    subtraction = airMult * dt * pBody->size() * mult;
                }
                else
                {
                    subtraction = 0.0f;
                }

                if (subtraction != 0.0f)
                {
                    lv.x -= subtraction;
                    if (lv.x > maxSpeed + speedOffset)
                    {
                        if (rightCollisionType == b2_dynamicBody && mult > 0.0f)
                        {
                            lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * 5.0f);
                            lv.x = lv.x + subtraction;
                        }
                        else
                        {
                            if (contactBody != nullptr && mult < 0.0f)
                            {
                                if (lv.x + subtraction > maxSpeed + speedOffset)
                                {
                                    lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * fwdGrndSlw);
                                }
                                else
                                {
                                    lv.x = maxSpeed + speedOffset;
                                }
                            }
                            lv.x = fmax(maxSpeed + speedOffset, lv.x + subtraction);
                        }
                    }
                    else if (lv.x < -maxSpeed + speedOffset)
                    {
                        if (contactBody != nullptr)
                        {
                            if (lv.x + subtraction < -maxSpeed + speedOffset)
                            {
                                lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * fwdGrndSlw);
                            }
                            else
                            {
                                lv.x = -maxSpeed + speedOffset;
                            }
                        }

                        lv.x = fmin(-maxSpeed + speedOffset, lv.x + subtraction);
                    }
                }

                lastKey = GLFW_KEY_A;
            }
        }
        else if (!haveContact)
        {
            lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * 0.5f);
        }
        else if (contactBody && contactBody->GetMass() != 0.0f && onAirFor == 0.0f)
        {
            // kinematic body
            lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * 15.0f);
        }
        else if (contactBody && contactType != b2_dynamicBody && onAirFor == 0.0f)
        {
            lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * 15.0f);
        }

        if (!contactBody) onAirFor += dt;
        else onAirFor = 0.0f;

        if (wh->keyData[GLFW_KEY_W] == 2 && ((contactBody && lv.y <= 5.0f) ||
            (lv.y < 0.0f && onAirFor < 0.2f && onAirFor != 0.0f)))
        {
            lv.y = 10.0f * (float)pBody->size();
        }

        if (lv.y < -20.0f)
        {
            lv.y = -20.0f;
        }
        pb->body->SetLinearVelocity(lv);

        handleBodyRotation();

        handleGunRotation();

        handlePortalCD();

        updateReset();
    }
};