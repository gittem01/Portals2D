#include <DefaultObjects/DefaultCube.h>
#include <DefaultObjects/LaserCube.h>
#include <ThinDrawer.h>

LaserCube::LaserCube(PortalWorld* portalWorld, b2Vec2 pos) : DefaultObject(portalWorld)
{
    this->pWorld = portalWorld;
    ALuint onHitSoundBuffer = Sounder::object->loadSound("cubeImpact.wav");
    onHitSoundBuffers.push_back(onHitSoundBuffer);

    b2Body* body = createPortalCube2(pos, b2Vec2(0.4, 0.4), pWorld, b2_dynamicBody);
    PortalBody* portalBody = portalWorld->createPortalBody(body);

    initialPos = pos;

    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    pData.type = (g_DataType)(DEFAULT_OBJECT_O | LASER_CUBE_O);
    pData.data = this;
    bData->extraData = &pData;

    this->wData = portalBody->worldIndex;

    renderGroup = ThinDrawer::object->addPortalBody(portalBody, { "laserCube.png" });
    ThinDrawer::object->debug_addPortalBody(portalBody);
}

void LaserCube::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    DefaultObject::reportContact(contact, impulse, contactingBody, fix2);

    if (impulse->normalImpulses[0] > 3.0 &&
        (contactingBody->GetLinearVelocity().Length() > 1.0 || contactingBody->GetAngularVelocity() > 0.5) && ThinDrawer::object->gameTime - lastSoundTime > 0.2)
    {
        lastSoundTime = ThinDrawer::object->gameTime;
        ALuint source = Sounder::object->playSound(onHitSoundBuffers[0], fmax(impulse->normalImpulses[0] * 0.03, 0.2));
        Sounder::object->setSoundPosition(source, glm::vec3(wData->pbIndices[0]->body->GetPosition().x, wData->pbIndices[0]->body->GetPosition().y, 0.0));
    }
}