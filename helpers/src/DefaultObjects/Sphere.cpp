#include <DefaultObjects/Sphere.h>
#include <GameObjects/Dropper.h>
#include <ThinDrawer.h>


Sphere::Sphere(PortalWorld* portalWorld, b2Vec2 pos, Dropper* linkedDropper, bool mirrored) : DefaultObject(portalWorld)
{
    startMirrored = mirrored;
	pWorld = portalWorld;
	ALuint onHitSoundBuffer = Sounder::object->loadSound("cubeImpact.wav");
	onHitSoundBuffers.push_back(onHitSoundBuffer);

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = pos;
    
    initialPos = pos;

	b2Body* body = portalWorld->CreateBody(&bodyDef);
    b2CircleShape cShape;
    cShape.m_radius = 0.5f;
    b2Fixture* fix = body->CreateFixture(&cShape, 1.0f);
    fix->SetRestitution(0.1);

	PortalBody* portalBody = portalWorld->createPortalBody(body);
    portalBody->isMirrored = mirrored;

    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    pData.type = (g_DataType)(DEFAULT_OBJECT_O | SPHERE_CUBE_O);
    pData.data = this;
    bData->extraData = &pData;
    
    this->wData = portalBody->worldIndex;

    renderGroup = ThinDrawer::object->addPortalBody(portalBody, {"sphere.png", "sphereEnabled.png"});
    ThinDrawer::object->debug_addPortalBody(portalBody);

    this->linkedDropper = linkedDropper;
}

void Sphere::recreate(bool mirrored)
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
        DefaultObject::recreate(startMirrored);
    }
}

void Sphere::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    DefaultObject::reportContact(contact, impulse, contactingBody, fix2);
}