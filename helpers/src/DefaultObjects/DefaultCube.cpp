#include <DefaultObjects/DefaultCube.h>
#include <ThinDrawer.h>

b2Body* createPortalCube2(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density = 1.0f) {
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
        fDef2.friction = 1.0f;
        body->CreateFixture(&fDef2);
    }

    return body;
}

DefaultCube::DefaultCube(PortalWorld* portalWorld) : DefaultObject(portalWorld)
{
	this->pWorld = portalWorld;
	ALuint onHitSoundBuffer = Sounder::object->loadSound("cubeImpact.wav");
	onHitSoundBuffers.push_back(onHitSoundBuffer);

	b2Body* body = createPortalCube2(b2Vec2(0.0, 0.0), b2Vec2(0.4, 0.4), pWorld, b2_dynamicBody);
	PortalBody* portalBody = portalWorld->createPortalBody(body);
	
    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
    physicsData->type = DEFAULT_OBJECT;
    physicsData->data = this;
    bData->extraData = physicsData;
    
    this->pBody = &portalBody->worldIndex->pbIndices;

    ThinDrawer::object->addPortalBody(portalBody, {"cube.png"});
    ThinDrawer::object->debug_addPortalBody(portalBody);
}

void DefaultCube::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    if (impulse->normalImpulses[0] > 3.0 &&
        (contactingBody->GetLinearVelocity().Length() > 1.0 || contactingBody->GetAngularVelocity() > 0.5) && ThinDrawer::object->gameTime - lastSoundTime > 0.1)
    {
        lastSoundTime = ThinDrawer::object->gameTime;
        ALuint source = Sounder::object->playSound(onHitSoundBuffers[0], fmax(impulse->normalImpulses[0] * 0.03, 0.2));
        Sounder::object->setSoundPosition(source, glm::vec3(pBody->at(0)->body->GetPosition().x, pBody->at(0)->body->GetPosition().y, 0.0));
    }
}