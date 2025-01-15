#include <DefaultObjects/DefaultCube.h>
#include <ThinDrawer.h>
#include "GameObjects/Dropper.h"

b2Body* createPortalCube2(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density)
{
    const float mult = (1.0f / 1.2f);
    std::vector<b2Vec2> vertices =
    {
        mult * 2 * b2Vec2(-0.1f * size.x, +0.5f * size.y),
        mult * 2 * b2Vec2(-0.2f * size.x, +0.6f * size.y),
        mult * 2 * b2Vec2(-0.5f * size.x, +0.6f * size.y),
        mult * 2 * b2Vec2(-0.6f * size.x, +0.5f * size.y),
        mult * 2 * b2Vec2(-0.6f * size.x, +0.2f * size.y),
        mult * 2 * b2Vec2(-0.5f * size.x, +0.1f * size.y)
    };
    b2PolygonShape boxShape;
    boxShape.SetAsBox(size.x * mult, size.y * mult);

    b2FixtureDef fDef1;
    fDef1.shape = &boxShape;
    fDef1.density = density;
    fDef1.restitution = 0.0f;
    fDef1.friction = 0.1f;

    b2BodyDef def;
    def.type = type;
    def.position = pos;
    def.linearDamping = 0.1f;

    b2Body* body = world->CreateBody(&def);
    body->CreateFixture(&fDef1);

    bool mirrorStatus = false;
    for (int i = 0; i < 4; i++) {
        b2PolygonShape cornerShape;
        cornerShape.Set(vertices.data(), (int)vertices.size());
        for (auto& vertex : vertices) {
            if (!mirrorStatus) {
                vertex.x *= -1;
            }
            else {
                vertex.y *= -1;
            }
        }
        mirrorStatus ^= 1;
        b2FixtureDef fDef2;
        fDef2.shape = &cornerShape;
        fDef2.density = density;
        fDef2.restitution = 0.2f;
        fDef2.friction = 3.0f;
        body->CreateFixture(&fDef2);
    }

    return body;
}

DefaultCube::DefaultCube(PortalWorld* portalWorld, b2Vec2 pos, Dropper* linkedDropper) : DefaultObject(portalWorld)
{
	this->pWorld = portalWorld;
	ALuint onHitSoundBuffer = Sounder::object->loadSound("cubeImpact.wav");
	onHitSoundBuffers.push_back(onHitSoundBuffer);

	b2Body* body = createPortalCube2(pos, b2Vec2(0.4, 0.4), pWorld, b2_dynamicBody);
	PortalBody* portalBody = portalWorld->createPortalBody(body);
	
    initialPos = pos;

    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    pData.type = (g_DataType)(DEFAULT_OBJECT_O | DEFAULT_CUBE_O);
    pData.data = this;
    bData->extraData = &pData;
    
    this->wData = portalBody->worldIndex;

    renderGroup = ThinDrawer::object->addPortalBody(portalBody, {"cube.png"});
    ThinDrawer::object->debug_addPortalBody(portalBody);

    this->linkedDropper = linkedDropper;
}

void DefaultCube::recreate(bool mirrored)
{
    if (linkedDropper)
    {
        int i = wData->pbIndices.size() - 1;
        for (; i >= 0; i--)
        {
            PortalBody* portalBody = wData->pbIndices[i];

            for (auto pair : wData->destroyCallbacks)
            {
                pair.first(portalBody, pair.second);
            }

            delete portalBody;
        }
    }
    else
    {
        DefaultObject::recreate(mirrored);
    }
}

void DefaultCube::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    float sumImpulse = impulse->normalImpulses[0];
    if (impulse->count > 1) sumImpulse += impulse->normalImpulses[1];

	DefaultObject::reportContact(contact, impulse, contactingBody, fix2);

    if (wData->destrTime < -10.0f && sumImpulse > 2.0 &&
        (contactingBody->GetLinearVelocity().Length() > 1.0 || contactingBody->GetAngularVelocity() > 0.5) && ThinDrawer::object->gameTime - lastSoundTime > 0.2)
    {
        lastSoundTime = ThinDrawer::object->gameTime;
        ALuint source = Sounder::object->playSound(onHitSoundBuffers[0], fmax(sumImpulse * 0.02, 0.075));
        Sounder::object->setSoundPosition(source, glm::vec3(wData->pbIndices[0]->body->GetPosition().x, wData->pbIndices[0]->body->GetPosition().y, 0.0));
    }
}