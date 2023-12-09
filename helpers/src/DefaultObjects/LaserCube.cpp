#include <DefaultObjects/DefaultCube.h>
#include <DefaultObjects/LaserCube.h>
#include <ThinDrawer.h>

LaserCube::LaserCube(PortalWorld* portalWorld) : DefaultObject(portalWorld)
{
    this->pWorld = portalWorld;
    ALuint onHitSoundBuffer = Sounder::object->loadSound("cubeImpact.wav");
    onHitSoundBuffers.push_back(onHitSoundBuffer);

    b2Body* body = createPortalCube2(b2Vec2(0.0, 0.0), b2Vec2(0.4, 0.4), pWorld, b2_dynamicBody);
    PortalBody* portalBody = portalWorld->createPortalBody(body);

    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
    physicsData->type = (g_DataType)(DEFAULT_OBJECT_O | LASER_CUBE_O);
    physicsData->data = this;
    bData->extraData = physicsData;

    this->pBody = &portalBody->worldIndex->pbIndices;

    ThinDrawer::object->addPortalBody(portalBody, { "laserCube.png" });
    ThinDrawer::object->debug_addPortalBody(portalBody);
}

void LaserCube::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    if (impulse->normalImpulses[0] > 3.0 &&
        (contactingBody->GetLinearVelocity().Length() > 1.0 || contactingBody->GetAngularVelocity() > 0.5) && ThinDrawer::object->gameTime - lastSoundTime > 0.1)
    {
        lastSoundTime = ThinDrawer::object->gameTime;
        ALuint source = Sounder::object->playSound(onHitSoundBuffers[0], fmax(impulse->normalImpulses[0] * 0.03, 0.2));
        Sounder::object->setSoundPosition(source, glm::vec3(pBody->at(0)->body->GetPosition().x, pBody->at(0)->body->GetPosition().y, 0.0));
    }
}