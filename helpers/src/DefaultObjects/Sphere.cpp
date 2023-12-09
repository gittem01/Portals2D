#include <DefaultObjects/Sphere.h>
#include <ThinDrawer.h>


Sphere::Sphere(PortalWorld* portalWorld) : DefaultObject(portalWorld)
{
	this->pWorld = portalWorld;
	ALuint onHitSoundBuffer = Sounder::object->loadSound("cubeImpact.wav");
	onHitSoundBuffers.push_back(onHitSoundBuffer);

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = b2Vec2(0.0, 0.0);
    
	b2Body* body = portalWorld->CreateBody(&bodyDef);
    b2CircleShape cShape;
    cShape.m_radius = 0.5f;
    body->CreateFixture(&cShape, 1.0f);

	PortalBody* portalBody = portalWorld->createPortalBody(body);
	
    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
    physicsData->type = (g_DataType)(DEFAULT_OBJECT_O | SPHERE_CUBE_O);
    physicsData->data = this;
    bData->extraData = physicsData;
    
    this->pBody = &portalBody->worldIndex->pbIndices;

    ThinDrawer::object->addPortalBody(portalBody, {"sphere.png"});
    ThinDrawer::object->debug_addPortalBody(portalBody);
}

void Sphere::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    
}