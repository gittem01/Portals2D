#include <DefaultObjects/Gun.h>
#include <ThinDrawer.h>

Gun::Gun(PortalWorld* portalWorld, b2Vec2 pos, b2Vec2 gunSize) : DefaultObject(portalWorld)
{
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
    bodyDef.bullet = false;
    bodyDef.allowSleep = false;

    b2Body* gBody = pWorld->CreateBody(&bodyDef);

    initialPos = pos;

    gBody->CreateFixture(&fDef);

    PortalBody* pb = pWorld->createPortalBody(gBody);
    this->wData = pb->worldIndex;

    bodyData* bData = (bodyData*)gBody->GetUserData().pointer;
    pData.type = (g_DataType)(DEFAULT_OBJECT_O | GUN_O);
    pData.data = this;
    bData->extraData = &pData;

    renderGroup = ThinDrawer::object->addPortalBody(pb, { "gun2.png" });
    ThinDrawer::object->debug_addPortalBody(pb);
}

void Gun::beginContact(b2Contact* contact, b2Fixture* otherFix, b2Fixture* thisFix)
{
	
}

void Gun::preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2)
{
    if (fix2->GetShape()->GetType() != b2Shape::e_edge)
    {
        contact->SetEnabled(false);
    }
}

void Gun::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{

}