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

    bool right;
    bool left;
    int lastDoubleKey;

    int onAirFor;

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

        onAirFor = -1;
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
        float l = normal.Length();
        normal.x /= l;
        normal.y /= l;

        b2Vec2 cPos = wManifold.points[0];
        float tHold = 0.1f;
        for (PortalBody* pb : *pBody){
            b2Vec2 bodyPos = pb->body->GetPosition();
            float botY = bodyPos.y - size.y / 2.0f;
            // TODO_01 : check slope manualy
            if (cPos.y < botY + tHold && cPos.y > botY - tHold && abs(normal.x) <= 0.6f){
                contactType = fix2->GetBody()->GetType();
                contactBody = fix2->GetBody();
                if (fix2->GetBody()->GetType() == b2_dynamicBody){
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

        //printf("Speed : %f\n", lv.x);

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
            if (right){
                if (haveContact){
                    lv.x += 400.0f * dt * pBody->size() * mult;
                }
                else if (mult > 0.0f){
                    if (lv.x < maxSpeed + speedOffset){
                        lv.x += 50.0f * dt * pBody->size() * mult;
                    }
                }
                else{
                    if (lv.x > -maxSpeed + speedOffset){
                        lv.x += 50.0f * dt * pBody->size() * mult;
                    }
                }
                lastKey = GLFW_KEY_D;
            }
            if (left){
                if (haveContact){
                    lv.x -= 400.0f * dt * pBody->size() * mult;
                }
                else if (mult > 0.0f){
                    if (lv.x > -maxSpeed + speedOffset){
                        lv.x -= 50.0f * dt * pBody->size() * mult;
                    }
                }
                else{
                    if (lv.x < maxSpeed + speedOffset){
                        lv.x -= 50.0f * dt * pBody->size() * mult;
                    }
                }
                
                lastKey = GLFW_KEY_A;
            }
        }
        else if (!haveContact){
            lv.x = (1 - 0.01f / totalIter) * lv.x;
        }
        else if (contactBody && contactBody->GetMass() != 0.0f && onAirFor == -1){
            lv.x = (1 - 0.95f / totalIter) * lv.x;
        }
        else if (contactBody && contactType == b2_staticBody && onAirFor == -1){
            lv.x = (1 - 0.95f / totalIter) * lv.x;
        }

        if (haveContact){
            if (lv.x > +maxSpeed + speedOffset){
                lv.x = +maxSpeed + speedOffset;
            }
            if (lv.x < -maxSpeed + speedOffset){
                lv.x = -maxSpeed + speedOffset;
            }
        }

        if (!contactBody) onAirFor++;
        else onAirFor = -1;

        if (wp->keyData[GLFW_KEY_W] && ((contactBody && lv.y <= 0.01f) || 
            (lv.y < 0.0f && onAirFor < 5 * totalIter && onAirFor != -1))){
            lv.y = 20.0f * pBody->size();
        }

        if (lv.y < -40.0f){
            lv.y = -40.0f;
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

#if 0
        if (pBody->size() == 1){
            PortalBody* pb = pBody->at(0);
            printf("%f - %f\n", pb->body->GetAngle(), pb->offsetAngle);
            float ang1 = fmod(pb->offsetAngle, 2 * b2_pi);
            float ang2 = fmod(pb->body->GetAngle(), 2 * b2_pi);
            if (ang1 > b2_pi) ang1 = ang1 - 2 * b2_pi;
            if (ang2 > b2_pi) ang2 = ang2 - 2 * b2_pi;

            float ang = ang1 + ang2;

            if (ang > 0.01f){
                pb->body->SetAngularVelocity(-20.0f * (abs(ang) / b2_pi));
            }
            else if (ang < -0.01f){
                pb->body->SetAngularVelocity(+20.0f * (abs(ang) / b2_pi));
            }
            else{
                pb->body->SetAngularVelocity(0.0f);
            }
        }
#endif
        haveContact = false;
        onPlatform = false;
        lastVelocity = lv;
        contactBody = nullptr;
        platformBody = nullptr;
        right = false;
        left = false;
        contactType = (b2BodyType)INT_MAX; // NULL
    }
};