#include <DefaultObjects/Sphere.h>
#include <ThinDrawer.h>


Sphere::Sphere(PortalWorld* portalWorld, b2Vec2 pos) : DefaultObject(portalWorld)
{
	this->pWorld = portalWorld;
	ALuint onHitSoundBuffer = Sounder::object->loadSound("cubeImpact.wav");
	onHitSoundBuffers.push_back(onHitSoundBuffer);

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = pos;
    
    initialPos = pos;

	b2Body* body = portalWorld->CreateBody(&bodyDef);
    b2CircleShape cShape;
    cShape.m_radius = 0.5f;
    body->CreateFixture(&cShape, 1.0f);

	PortalBody* portalBody = portalWorld->createPortalBody(body);
	
    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    pData.type = (g_DataType)(DEFAULT_OBJECT_O | SPHERE_CUBE_O);
    pData.data = this;
    bData->extraData = &pData;
    
    this->wData = portalBody->worldIndex;

    renderGroup = ThinDrawer::object->addPortalBody(portalBody, {"sphere.png", "sphereEnabled.png"});
    ThinDrawer::object->debug_addPortalBody(portalBody);
}

void Sphere::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    
}