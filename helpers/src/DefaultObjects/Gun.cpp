#include <DefaultObjects/Gun.h>
#include <ThinDrawer.h>

Gun::Gun(PortalWorld* portalWorld, b2Vec2 pos, b2Vec2 gunSize) : DefaultObject(portalWorld)
{
    this->pWorld = portalWorld;
    ALuint onHitSoundBuffer = Sounder::object->loadSound("cubeImpact.wav");
    onHitSoundBuffers.push_back(onHitSoundBuffer);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(gunSize.x / 2.0f, gunSize.y / 2.0f);

    b2FixtureDef fDef;
    fDef.density = 0.1f;
    fDef.shape = &boxShape;
    fDef.friction = 0.0f;
    fDef.restitution = 0.0f;

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = pos;
    bodyDef.fixedRotation = false;
    bodyDef.linearDamping = 0.0f;
    bodyDef.bullet = true;
    bodyDef.allowSleep = false;

    b2Body* gBody = pWorld->CreateBody(&bodyDef);

    gBody->CreateFixture(&fDef);

    PortalBody* pb = pWorld->createPortalBody(gBody);
    this->pBody = &pb->worldIndex->pbIndices;

    bodyData* bData = (bodyData*)gBody->GetUserData().pointer;
    g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
    physicsData->type = DEFAULT_OBJECT;
    physicsData->data = this;
    bData->extraData = physicsData;

    ThinDrawer::object->debug_addPortalBody(pb);
    ThinDrawer::object->addPortalBody(pb, { "gun.png" });
}

void Gun::preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2)
{
    b2Body* body = fix2->GetBody();
    if (body->GetType() == b2_dynamicBody && body->GetMass() > 0.0f)
    {
        contact->SetEnabled(false);
    }
}