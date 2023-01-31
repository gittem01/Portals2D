#pragma once

#include <WindowPainter.h>

class TestPlayer
{

public:

    std::vector<PortalBody*>* pBody;
    std::vector<PortalBody*>* gun;
    PortalWorld* pWorld;
    WindowPainter* wp;
    b2Vec2 size;

    b2Vec2 lastVelocity;
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
    b2Vec2 lastOriantation;
    b2Vec2 newOriantation;

    int lastKey;
    int keyBeforeSwitch;

    bool right;
    bool left;
    int lastDoubleKey;

    float onAirFor;

    b2Vec2 lastDirs[2];

    TestPlayer(PortalWorld* pWorld, WindowPainter* wp, b2Vec2 pos=b2Vec2(0, 0), b2Vec2 size=b2Vec2(0.8f, 1.5f)){
        this->pWorld = pWorld;
        this->wp = wp;
        this->size = size;
        createBody(pos, size);

        lastVelocity = pBody->at(0)->body->GetLinearVelocity();
        haveContact = false;
        onPlatform = false;
        contactBody = nullptr;
        platformBody = nullptr;
        contactType = (b2BodyType)10;

        bodySwitch = false;
        lastBody = nullptr;
        lastKey = -1;
        keyBeforeSwitch = -1;

        right = false;
        left = false;
        lastDoubleKey = -1;

        onAirFor = 0.0f;
    }

    void createBody(b2Vec2 pos, b2Vec2 size){
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
        bData->extraData = this;

        pBody = pb->worldIndex;

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

        gun = pb->worldIndex;

        b2RevoluteJointDef jDef;
        jDef.bodyA = pBody->at(0)->body;
        jDef.bodyB = gun->at(0)->body;
        jDef.collideConnected = false;
        jDef.localAnchorA = b2Vec2(0.0f, 0.35f);
        jDef.localAnchorB = b2Vec2(0.0f, -0.35f);

        pWorld->CreateJoint(&jDef);
    }

    void handleForces(b2Contact* contact, b2Fixture* fix2){
        b2WorldManifold wManifold;
        contact->GetWorldManifold(&wManifold);
        b2Vec2 normal = wManifold.normal;

        contact->SetRestitution(0.0f);
    }

    void reportContact(b2Contact* contact, b2Body* contactingBody, b2Fixture* fix2){
        if (!contact->IsEnabled() || fix2->IsSensor()){
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

    void camHandle(){
        if (pBody->size() > 1){
            b2Vec2 p1 = pBody->at(0)->body->GetPosition();
            b2Vec2 p2 = pBody->at(1)->body->GetPosition();

            glm::vec2 diff = glm::vec2(abs(p2.x - p1.x), abs(p2.y - p1.y)) / 2.0f + 5.0f;
            glm::vec2 mid = glm::vec2(p2.x + p1.x, p2.y + p1.y) / 2.0f;
            
            glm::vec2 posDiff = mid - wp->cam->pos;

            wp->cam->pos += posDiff / 50.0f;
            float reqZoom = sqrt(abs(wp->cam->defaultXSides.x / diff.x));
            if (reqZoom < 0.65f){
                wp->cam->zoom += (reqZoom - wp->cam->zoom) / 10.0f;
            }
            else{
                wp->cam->zoom += (0.65f - wp->cam->zoom) / 50.0f;
            }
        }
        else{
            b2Vec2 bp = pBody->at(0)->body->GetPosition();
            glm::vec2 diff = glm::vec2(bp.x, bp.y) - wp->cam->pos;
            wp->cam->pos += diff / 50.0f;
            wp->cam->zoom += (0.65f - wp->cam->zoom) / 50.0f;
        }
    }

    void update(float dt, int totalIter){
        const float maxSpeed = 5.0f;
        float speedOffset = 0.0f;
        PortalBody* pb = pBody->at(0);

        if (pb != lastBody && lastBody != nullptr){
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

        for (PortalBody* portalBody : *pBody){
            bodyData* bd = (bodyData*)portalBody->body->GetUserData().pointer;
            if (bd && !bd->extraData){
                 bd->extraData = (void*)this;
            }  
        }

        b2Vec2 lv = pb->body->GetLinearVelocity();

        if (onPlatform){
            b2Vec2 pVel = platformBody->GetLinearVelocity();
            speedOffset = pVel.x;
        }

        if (wp->keyData[GLFW_KEY_D] && wp->keyData[GLFW_KEY_A]){
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
        else if (wp->keyData[GLFW_KEY_D]){
            right = true;
        }
        else if (wp->keyData[GLFW_KEY_A]){
            left = true;
        }

        lastKey = -1;
        float mult = 1.0f;
        if (!left && !right && onPlatform){
            lv.x = platformBody->GetLinearVelocity().x;
        }
        if (left || right){
            if (right){
                if (keyBeforeSwitch == GLFW_KEY_D){
                    mult = -1.0f;
                }
                else{
                    keyBeforeSwitch = -1;
                }
            }
            else if (left){
                if (keyBeforeSwitch == GLFW_KEY_A){
                    mult = -1.0f;
                }
                else{
                    keyBeforeSwitch = -1;
                }
            }
        }
        else{
            keyBeforeSwitch = -1;
        }
        
        if (left || right)
        {
            const float mass = pb->body->GetMass();
            const float groundMult = 200.0f;
            const float airMult = 30.0f;
            if (right)
            {
                if (contactBody != nullptr)
                {
                    lv.x += groundMult * dt * pBody->size() * mult;
                }
                else if (rightCollisionType == b2_dynamicBody)
                {
                    if (mult > 0.0f)
                    {
                        if (lv.x < maxSpeed + speedOffset)
                        {
                            lv.x += airMult * dt * pBody->size() * mult;
                        }
                    }
                    else
                    {
                        if (lv.x > -maxSpeed + speedOffset)
                        {
                            lv.x += airMult * dt * pBody->size() * mult;
                        }
                    }
                }
                
                lastKey = GLFW_KEY_D;
            }
            if (left)
            {
                if (contactBody != nullptr)
                {
                    lv.x -= groundMult * dt * pBody->size() * mult;
                }
                else if (leftCollisionType == b2_dynamicBody)
                {
                    if (mult > 0.0f)
                    {
                        if (lv.x > -maxSpeed + speedOffset)
                        {
                            lv.x -= airMult * dt * pBody->size() * mult;
                        }
                    }
                    else
                    {
                        if (lv.x < maxSpeed + speedOffset)
                        {
                            lv.x -= airMult * dt * pBody->size() * mult;
                        }
                    }
                }
                
                lastKey = GLFW_KEY_A;
            }
        }
        else if (!haveContact)
        {
            lv.x = (1 - 0.1f / (totalIter * dt * 100.0f)) * lv.x;
        }
        else if (contactBody && contactBody->GetMass() != 0.0f && onAirFor == 0.0f)
        {
            lv.x = (1 - 0.95f / (totalIter * dt * 100.0f)) * lv.x;
        }
        else if (contactBody && contactType == b2_staticBody && onAirFor == 0.0f)
        {
            lv.x = (1 - 0.95f / (totalIter * dt * 100.0f)) * lv.x;
        }

        if (haveContact)
        {
            if (lv.x > +maxSpeed + speedOffset)
            {
                lv.x = +maxSpeed + speedOffset;
            }
            if (lv.x < -maxSpeed + speedOffset)
            {
                lv.x = -maxSpeed + speedOffset;
            }
        }

        if (!contactBody) onAirFor += dt;
        else onAirFor = 0.0f;

        if (wp->keyData[GLFW_KEY_W] && ((contactBody && lv.y <= 5.0f) ||
            (lv.y < 0.0f && onAirFor < 0.2f && onAirFor != 0.0f))){
            lv.y = 10.0f * pBody->size();
        }

        if (lv.y < -40.0f){
            lv.y = -40.0f;
        }

        pb->body->SetLinearVelocity(lv);

        lastBody = pb;

        if (pBody->size() > 1){
            PortalBody* b1 = pBody->at(0);
            PortalBody* b2 = pBody->at(1);
            fixtureData* fixData1 = (fixtureData*)b1->body->GetFixtureList()->GetUserData().pointer;
            fixtureData* fixData2 = (fixtureData*)b2->body->GetFixtureList()->GetUserData().pointer;
            portalCollision* pc1 = fixData1->pCollisions.at(0);
            portalCollision* pc2 = fixData2->pCollisions.at(0);

            lastDirs[0] = pc1->side ? -pc1->portal->dir : pc1->portal->dir;
            lastDirs[1] = pc2->side ? -pc2->portal->dir : pc2->portal->dir;
        }

        if (!(wp->keyData[GLFW_KEY_A] && wp->keyData[GLFW_KEY_D])){
            if (wp->keyData[GLFW_KEY_A]){
                lastDoubleKey = GLFW_KEY_A;
            }
            else if (wp->keyData[GLFW_KEY_D]){
                lastDoubleKey = GLFW_KEY_D;
            }
            else{
                lastDoubleKey = -1;
            }
        }

        // body rotation
        float ang = fmod(pb->offsetAngle + pb->body->GetAngle(), 2 * b2_pi);
        if (ang < -b2_pi)
            ang += 2 * b2_pi;
        else if (ang > b2_pi)
            ang -= 2 * b2_pi;
        if (ang > 0.03f){
            pb->body->SetAngularVelocity(-10.0f * (abs(ang) / b2_pi));
        }
        else if (ang < -0.03f){
            pb->body->SetAngularVelocity(+10.0f * (abs(ang) / b2_pi));
        }
        else{
            pb->body->SetTransform(pb->body->GetPosition(), pb->body->GetAngle() - ang);
            pb->body->SetAngularVelocity(0.0f);
        }

        // gun rotation
        b2Vec2 gunCenter = pb->body->GetPosition() + b2Vec2(0.0f, 0.35f);
        glm::vec2 mousePos = wp->cam->getMouseCoords();

        b2Vec2 diff = b2Vec2(mousePos.x - gunCenter.x, mousePos.y - gunCenter.y);
        ang = fmod(gun->at(0)->offsetAngle + gun->at(0)->body->GetAngle() - pWorld->calcAngle2(diff) + b2_pi * 0.5f, 2 * b2_pi);
        if (ang < -b2_pi)
            ang += 2 * b2_pi;
        else if (ang > b2_pi)
            ang -= 2 * b2_pi;

        if (ang > 0.03f)
        {
            gun->at(0)->body->SetAngularVelocity(-20.0f);
        }
        else if (ang < -0.03f)
        {
            gun->at(0)->body->SetAngularVelocity(+20.0f);
        }
        else
        {
            gun->at(0)->body->SetTransform(gun->at(0)->body->GetPosition(), gun->at(0)->body->GetAngle() - ang);
            gun->at(0)->body->SetAngularVelocity(0.0f);
        }

        haveContact = false;
        onPlatform = false;
        lastVelocity = lv;
        contactBody = nullptr;
        platformBody = nullptr;
        right = false;
        left = false;
        contactType = (b2BodyType)INT_MAX; // NULL
        rightCollisionType = b2_dynamicBody;
        leftCollisionType = b2_dynamicBody;
    }
};
