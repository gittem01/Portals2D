#include <Portal.h>

class TestPlayer
{

public:

    std::vector<PortalBody*>* pBody;
    PortalWorld* pWorld;
    WindowPainter* wp;
    b2Vec2 size;

    b2Vec2 lastVelocity;
    bool canJump;
    bool jumpedLastFrame;
    bool haveContact;
    b2Body* contactBody;
    b2BodyType contactType;

    PortalBody* lastBody;
    bool bodySwitch;
    float totalTime = 1.0f;
    float switchTimer;
    b2Vec2 lastOriantation;

    TestPlayer(PortalWorld* pWorld, WindowPainter* wp, b2Vec2 pos=b2Vec2(0, 0), b2Vec2 size=b2Vec2(0.8f, 2.0f)){
        this->pWorld = pWorld;
        this->wp = wp;
        this->size = size;
        createBody(pos, size);

        lastVelocity = pBody->at(0)->body->GetLinearVelocity();
        canJump = false;
        jumpedLastFrame = false;
        haveContact = false;
        contactBody = nullptr;
        contactType = (b2BodyType)10;

        bodySwitch = false;
        lastBody = nullptr;
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

        if (abs(normal.x) > 0.3f){
            contact->SetFriction(0.0f);
        }
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
            if (cPos.y < botY + tHold && cPos.y > botY - tHold && abs(normal.x) <= 0.3f && pb->body->GetAngle() < 0.1f){
                canJump = true;
                contactType = fix2->GetBody()->GetType();
                contactBody = fix2->GetBody();
                if (fix2->GetBody()->GetType() == b2_dynamicBody){
                    contact->SetFriction(1.0f);
                }
            }
        }
    }

    void update(float dt, int totalIter){
        const float maxSpeed = 7.5f;
        PortalBody* pb = pBody->at(0);

        if (pb != lastBody && lastBody != nullptr){
            bodySwitch = true;
            switchTimer = glfwGetTime();
        }
        if (bodySwitch){
            printf("%f , (%f, %f)\n", glfwGetTime() - switchTimer, lastOriantation.x, lastOriantation.y);
            if (glfwGetTime() - switchTimer > this->totalTime){
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

        if (wp->keyData[GLFW_KEY_D] || wp->keyData[GLFW_KEY_A]){
            if (wp->keyData[GLFW_KEY_D]){
                if (haveContact || lv.x > 0){
                    pb->body->ApplyForceToCenter(cnst * b2Vec2(7.0f, 0.0f), true);
                }
                else{
                    pb->body->ApplyForceToCenter(cnst * b2Vec2(0.02f, 0.0f), true);
                }
                
            }
            if (wp->keyData[GLFW_KEY_A]){
                if (haveContact || lv.x < 0){
                    pb->body->ApplyForceToCenter(cnst * b2Vec2(-7.0f, 0.0f), true);
                }
                else{
                    pb->body->ApplyForceToCenter(cnst * b2Vec2(-0.7f, 0.0f), true);
                }
            }
        }
        else if (!haveContact){
            lv.x = (1 - 0.05f / totalIter) * lv.x;
        }
        else if ((contactBody && abs(contactBody->GetLinearVelocity().x - lv.x) > 1.0f)){
            lv.x = (1 - 0.95f / totalIter) * lv.x;
        }
        if (contactBody && contactType == b2_staticBody){
            lv.x = (1 - 0.95f / totalIter) * lv.x;
        }
        if (lv.x > +maxSpeed){
            lv.x = +maxSpeed;
        }
        if (lv.x < -maxSpeed){
            lv.x = -maxSpeed;
        }
        if (wp->keyData[GLFW_KEY_W] && canJump && !jumpedLastFrame){
            b2Vec2 force(cnst * b2Vec2(0, 20));
            pb->body->ApplyForceToCenter(force, true);
            jumpedLastFrame = true;
        }
        else{
            jumpedLastFrame = false;
        }

        pb->body->SetLinearVelocity(lv);

        lastBody = pb;
        if (pBody->size() > 1 && !bodySwitch){
            portalCollision* pc = (*pb->fixtureCollisions[pb->body->GetFixtureList()]->begin());
            if (pc->side == 0){
                lastOriantation = pc->portal->dir;
            }
            else{
                lastOriantation = -pc->portal->dir;
            }
        }

        canJump = false;
        haveContact = false;
        lastVelocity = lv;
        contactBody = nullptr;
        contactType = (b2BodyType)10; // NULL
    }
};