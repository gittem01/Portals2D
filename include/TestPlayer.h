#include <Portal.h>

class TestPlayer
{

public:

    std::vector<PortalBody*>* pBody;
    PortalWorld* pWorld;
    WindowPainter* wp;
    b2Vec2 size;

    b2Vec2 lastVelocity;
    bool haveContact;
    bool onPlatform;
    b2Body* contactBody;
    b2Body* platformBody;
    b2BodyType contactType;

    PortalBody* lastBody;
    bool bodySwitch;
    float totalTime = 0.1f;
    float switchTimer;
    b2Vec2 lastOriantation;
    b2Vec2 newOriantation;

    int lastKey;
    int keyBeforeSwitch;

    b2Vec2 lastDirs[2];

    TestPlayer(PortalWorld* pWorld, WindowPainter* wp, b2Vec2 pos=b2Vec2(0, 0), b2Vec2 size=b2Vec2(0.8f, 2.0f)){
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
    }

    void createBody(b2Vec2 pos, b2Vec2 size){
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = pos;
        bodyDef.fixedRotation = true;
        bodyDef.linearDamping = 0.1f;
        bodyDef.bullet = true;

        b2Body* body = pWorld->world->CreateBody(&bodyDef);

        b2PolygonShape boxShape;
        boxShape.SetAsBox(size.x / 2.0f, size.y / 2.0f);

        b2FixtureDef fDef;
        fDef.density = 1.0f;
        fDef.shape = &boxShape;
        fDef.friction = 0.0f;
        fDef.restitution = 0.0f;

        body->CreateFixture(&fDef);

        PortalBody* pb = pWorld->createPortalBody(body);
        pb->bodyColor = b2Color(1.0f, 0.0f, 0.0f, 0.5f);

        bodyData* bData = (bodyData*)body->GetUserData().pointer;
        bData->extraData = this;

        pBody = pb->worldIndex;
    }

    void handleForces(b2Contact* contact, b2Fixture* fix2){
        b2WorldManifold wManifold;
        contact->GetWorldManifold(&wManifold);
        b2Vec2 normal = wManifold.normal;

        contact->SetRestitution(0.0f);
    }

    void reportContact(b2Contact* contact, b2Fixture* fix2){
        if (!contact->IsEnabled() || fix2->IsSensor()){
            return;
        }

        haveContact = true;

        b2Vec2 lv = pBody->at(0)->body->GetLinearVelocity();

        b2WorldManifold wManifold;
        contact->GetWorldManifold(&wManifold);
        b2Vec2 normal = wManifold.normal;

        b2Vec2 cPos = wManifold.points[0];
        float tHold = 0.1f;
        for (PortalBody* pb : *pBody){
            b2Vec2 bodyPos = pb->body->GetPosition();
            float botY = bodyPos.y - size.y / 2.0f;
            if (cPos.y < botY + tHold && cPos.y > botY - tHold && abs(normal.x) <= 0.3f){
                contactType = fix2->GetBody()->GetType();
                contactBody = fix2->GetBody();
                if (fix2->GetBody()->GetType() == b2_dynamicBody){
                    contact->SetFriction(1.0f);
                    if (fix2->GetDensity() == 0.0f &&
                    (contact->GetFixtureA() == pBody->at(0)->body->GetFixtureList() || contact->GetFixtureB() == pBody->at(0)->body->GetFixtureList()))
                    {
                        onPlatform = true;
                        platformBody = fix2->GetBody();
                    }
                }
            }
        }
    }

    void update(float dt, int totalIter){
        const float maxSpeed = 10.0f;
        float speedOffset = 0.0f;
        PortalBody* pb = pBody->at(0);

        if (pb != lastBody && lastBody != nullptr){
            bodySwitch = true;
            switchTimer = glfwGetTime();
            float diff = abs(lastDirs[0].x + lastDirs[1].x);
            if (lastKey != -1 && diff > 1.99f){
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

        float cnst = pb->body->GetMass() * (pBody->size() / dt);
        b2Vec2 lv = pb->body->GetLinearVelocity();

        lastKey = -1;
        float mult = 1.0f;
        if ((wp->keyData[GLFW_KEY_D] || wp->keyData[GLFW_KEY_A]) && !(wp->keyData[GLFW_KEY_D] && wp->keyData[GLFW_KEY_A])){
            if (wp->keyData[GLFW_KEY_D]){
                if (keyBeforeSwitch == GLFW_KEY_D){
                    mult = -1.0f;
                }
                else{
                    keyBeforeSwitch = -1;
                }
            }
            else if (wp->keyData[GLFW_KEY_A]){
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
        if ((wp->keyData[GLFW_KEY_D] || wp->keyData[GLFW_KEY_A]) && 
            !(wp->keyData[GLFW_KEY_D] && wp->keyData[GLFW_KEY_A]))
        {
            if (wp->keyData[GLFW_KEY_D]){
                if (haveContact){
                    lv.x += 400.0f * dt * pBody->size() * mult;
                }
                else{
                    lv.x += 50.0f * dt * pBody->size() * mult;
                }
                lastKey = GLFW_KEY_D;
            }
            if (wp->keyData[GLFW_KEY_A]){
                if (haveContact){
                    lv.x -= 400.0f * dt * pBody->size() * mult;
                }
                else{
                    lv.x -= 50.0f * dt * pBody->size() * mult;
                }
                lastKey = GLFW_KEY_A;
            }
        }
        else if (!haveContact){
            lv.x = (1 - 0.05f / totalIter) * lv.x;
        }
        else if ((contactBody && contactBody->GetMass() != 0.0f)){
            lv.x = (1 - 0.95f / totalIter) * lv.x;
        }
        else if (contactBody && contactType == b2_staticBody){
            lv.x = (1 - 0.95f / totalIter) * lv.x;
        }
        if (onPlatform){
            b2Vec2 pVel = platformBody->GetLinearVelocity();
            speedOffset += pVel.x;
        }
        if (lv.x > +maxSpeed + speedOffset){
            lv.x = +maxSpeed + speedOffset;
        }
        if (lv.x < -maxSpeed + speedOffset){
            lv.x = -maxSpeed + speedOffset;
        }
        
        if (wp->keyData[GLFW_KEY_W] && contactBody && lv.y <= 0.01f){
            lv.y += 20.0f * pBody->size();
        }

        pb->body->SetLinearVelocity(lv);

        lastBody = pb;

        if (pBody->size() > 1){
            PortalBody* b1 = pBody->at(0);
            PortalBody* b2 = pBody->at(1);
            portalCollision* pc1 = (*b1->fixtureCollisions[b1->body->GetFixtureList()]->begin());
            portalCollision* pc2 = (*b2->fixtureCollisions[b2->body->GetFixtureList()]->begin());

            lastDirs[0] = pc1->side ? -pc1->portal->dir : pc1->portal->dir;
            lastDirs[1] = pc2->side ? -pc2->portal->dir : pc2->portal->dir;
        }

        haveContact = false;
        onPlatform = false;
        lastVelocity = lv;
        contactBody = nullptr;
        platformBody = nullptr;
        contactType = (b2BodyType)INT_MAX; // NULL
    }
};