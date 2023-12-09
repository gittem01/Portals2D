#include <DefaultObjects/Cyber.h>
#include <ThinDrawer.h>


Cyber::Cyber(PortalWorld* portalWorld) : DefaultObject(portalWorld)
{
	this->pWorld = portalWorld;
	ALuint onHitSoundBuffer = Sounder::object->loadSound("cubeImpact.wav");
	onHitSoundBuffers.push_back(onHitSoundBuffer);

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = b2Vec2(0.0, 0.0);
    
	b2Body* body = portalWorld->CreateBody(&bodyDef);
    b2Body* mainBody = body;
    b2PolygonShape pShape;
    b2Vec2 points[5] = {
        b2Vec2(-0.765754, 0.3955),
        b2Vec2(-1.4425, 0.17025),
        b2Vec2(-1.423145, -0.063438),
        b2Vec2(0.226169, -0.063448),
        b2Vec2(0.19986, 0.262395),
    };
    float minX = -1.4425;
    float minY = -0.063448;
    float maxX = 0.226169;
    float maxY = 0.3955;

    float midX = (minX + maxX) / 2.0;
    float midY = (minY + maxY) / 2.0;
    for (int i = 0; i < 5; i++)
    {
        points[i].x -= midX;
        points[i].y -= midY;
        points[i] *= 2.0f;
    }

    pShape.Set(points, 5);
    body->CreateFixture(&pShape, 1.0f);

	PortalBody* portalBody = portalWorld->createPortalBody(body);
	
    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
    physicsData->type = DEFAULT_OBJECT_O;
    physicsData->data = this;
    bData->extraData = physicsData;
    
    this->pBody = &portalBody->worldIndex->pbIndices;

    ThinDrawer::object->addPortalBody(portalBody, {"cyber.png"});
    ThinDrawer::object->debug_addPortalBody(portalBody);

    bodyDef.position = b2Vec2(-1.18, -0.3);
    bodyDef.angularDamping = 0.1f;
    body = portalWorld->CreateBody(&bodyDef);
    b2CircleShape cShape;
    cShape.m_radius = 0.27f;
    body->CreateFixture(&cShape, 0.5f);
    portalBody = portalWorld->createPortalBody(body);
    ThinDrawer::object->addPortalBody(portalBody, {"cyberWheel.png"});
    ThinDrawer::object->debug_addPortalBody(portalBody);

    b2RevoluteJointDef jointDef;
    jointDef.Initialize(mainBody, body, body->GetPosition());
    jointDef.calculateAnchorHelpers();
    portalWorld->CreateJoint(&jointDef);

    bodyDef.position = b2Vec2(+1.0, -0.3);
    body = portalWorld->CreateBody(&bodyDef);
    body->CreateFixture(&cShape, 0.5f);
    portalBody = portalWorld->createPortalBody(body);
    ThinDrawer::object->addPortalBody(portalBody, {"cyberWheel.png"});
    ThinDrawer::object->debug_addPortalBody(portalBody);

    jointDef.Initialize(mainBody, body, body->GetPosition());
    jointDef.calculateAnchorHelpers();
    portalWorld->CreateJoint(&jointDef);
}

void Cyber::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    
}