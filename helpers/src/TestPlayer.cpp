#include <TestPlayer.h>

TestPlayer* TestPlayer::object;

TestPlayer::TestPlayer(Game* game, b2Vec2 pos, b2Vec2 size) : DefaultObject(game->pWorld)
{
    TestPlayer::object = this;

    this->game = game;
    this->wh = game->thinDrawer->wh;
    bodySize = size;
    createBody(pos);

    gameTime = 0.0;

    holdBody = nullptr;
    holdGun = nullptr;
    destrPortal = nullptr;
    holdConnection = nullptr;
    lastBodyHoldTime = 0.0f;
    releaseRequestTime = -1.0f;
    holdERequestTime = -1.0f;
    holdAngle = 0.0f;
    angVal = b2_pi * 0.1f;
    shouldRelease = false;
    holdColliding = false;

    playerShader = ThinDrawer::object->playerShader;
    gData.positions = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    gData.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    gData.portal.x = NAN;
    gData.data.x = 0.4f;

    portalA = nullptr;
    portalB = nullptr;
    lastPortalCreationTime = -1.0f;

    grounded = false;
    contactBody = nullptr;
    platformBody = nullptr;
    l_contactBody = nullptr;
    l_platformBody = nullptr;
    oldJoint = nullptr;
    contactType = (b2BodyType)INT_MAX;

    bodySwitch = false;
    lastBody = nullptr;
    lastKey = -1;
    keyBeforeSwitch = -1;

    right = false;
    left = false;
    lastDoubleKey = -1;

    onAirFor = 0.0f;
    avgNormalX = 0.0;
    numNormals = 0;
    normXToWalk = 0.6;
    setFriction = 0.0;

    legAng = 0.0;
    rotSpeed = 0.0;

    jumpBuffer = Sounder::object->bufferMap["jumpTest.wav"];
    blueShootBuffer = Sounder::object->bufferMap["blueShoot.wav"];
    orangeShootBuffer = Sounder::object->bufferMap["orangeShoot.wav"];

    pWorld->portalDestroyCallbacks.push_back(portalDestrCallBack);

    renderInit();
}

void TestPlayer::portalDestrCallBack(Portal* portal)
{
    object->destrPortal = portal;
    if (object->holdConnection && (object->holdConnection->portal1 == portal || object->holdConnection->portal2 == portal))
    {
        object->releaseB();
    }
}

float TestPlayer::springMovement(float value, float vel, float springFactor, float dampingFactor, float deltaTime)
{
    float force = value * springFactor;

    vel -= (force + dampingFactor * vel) * deltaTime;

    return vel;
}

void TestPlayer::memMap(void* ptr)
{
    static b2Vec2 lastLinVel = b2Vec2(0.0, 0.0);
    static float aimedRatio = 0.5;
    static float bodyPos = 0.0;
    static float bodyVel = 0.0;

    TestPlayer* tp = (TestPlayer*)ptr;
    uint32_t imNum = ThinDrawer::object->lastSwapChainImageIndex;

    int i = 0;
    for (PortalBody* pb : *tp->pBody)
    {
        b2Body* body = pb->body;

        s_dRenderPlayerUbo* ru2;
        vkMapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[0]->memory, tp->playerShader->alignedSize * (i + imNum * 8),
            tp->playerShader->alignedSize, 0, (void**)&ru2);

        b2Vec2 pos = body->GetPosition();
        ru2->posRotD = glm::vec4(pos.x, pos.y, body->GetAngle(), 0.0);

        std::set<portalCollision*> pcs;
        for (b2Fixture* f : pb->fixtures)
        {
            fixtureData* fixData = (fixtureData*)f->GetUserData().pointer;
            PBFixtureData* data = (PBFixtureData*)fixData->data;
            for (portalCollision* pc : data->pCollisions)
            {
                pcs.insert(pc);
            }
        }

        int j = 0;
        for (portalCollision* pc : pcs)
        {
            ru2->portals[j] = glm::vec4(pc->portal->points[1 ^ pc->side].x, pc->portal->points[1 ^ pc->side].y,
                pc->portal->points[pc->side].x, pc->portal->points[pc->side].y);
            j += 1;
        }
        ru2->numPortals = j;

        vkUnmapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[0]->memory);

        i += 1;
    }

    const float maxSpeed = 100.0f;
    PortalBody* pb0 = tp->pBody->at(0);

    b2Body* body = pb0->body;
    b2Vec2 linVel = body->GetLinearVelocity();
    if (tp->l_platformBody)
    {
        linVel -= tp->l_platformBody->GetLinearVelocity();
    }
    else if (tp->grounded && tp->l_contactBody && tp->l_contactBody->GetType() == b2_dynamicBody)
    {
        linVel -= tp->l_contactBody->GetLinearVelocity();
    }

    b2Vec2 diff = linVel - lastLinVel;

    bodyVel += diff.y;

    bodyVel = springMovement(bodyPos, bodyVel, 150.0, 3.0, Game::object->deltaTime * 5.0);
    bodyPos += bodyVel * Game::object->deltaTime;

    lastLinVel = linVel;

    if (tp->grounded)
    {

        float sign = (linVel.x > 0) ? -1 : +1;
        float len = linVel.Length();
        tp->rotSpeed += len * Game::object->deltaTime * 10.0 * sign;
        tp->rotSpeed = fmin(tp->rotSpeed, +len * 1.0 * b2_pi * 0.75);
        tp->rotSpeed = fmax(tp->rotSpeed, -len * 1.0 * b2_pi * 0.75);
    }

    if ((diff.Length() > 0.2 || linVel.Length() > 1.0) && tp->grounded)
    {
        aimedRatio = 0.5;
    }
    else
    {
        const float m = 0.1;
        aimedRatio = 0.25;
        float roundedAngle = fmod(tp->legAng, 2.0 * b2_pi);
        if (roundedAngle < 0.0) roundedAngle += 2.0 * b2_pi;

        if (roundedAngle < b2_pi * 0.5)
        {
            // to 0.0
            tp->rotSpeed = -maxSpeed * roundedAngle * m;
        }
        else if (roundedAngle < b2_pi)
        {
            // to pi
            tp->rotSpeed = maxSpeed * (b2_pi - roundedAngle) * m;
        }
        else if (roundedAngle > b2_pi * 1.5f)
        {
            // to 2.0 * pi
            tp->rotSpeed = maxSpeed * (2.0 * b2_pi - roundedAngle) * m;
        }
        else if (roundedAngle > b2_pi)
        {
            // to pi
            tp->rotSpeed = -maxSpeed * (roundedAngle - b2_pi) * m;
        }
    }

    tp->legAng += tp->rotSpeed * Game::object->deltaTime;
    tp->legAng = fmod(tp->legAng, 2.0 * b2_pi);
    if (tp->legAng < 0.0) tp->legAng += 2.0 * b2_pi;

    tp->renderData.legRatio += (aimedRatio - tp->renderData.legRatio) * Game::object->deltaTime * 3.0;
    tp->renderData.bodyRatio = fmax((1.0 - abs(bodyPos)) - tp->renderData.headRatio - tp->renderData.legRatio, 0.2);
    float marginLeft = (1.0 - tp->renderData.headRatio - tp->renderData.bodyRatio - tp->renderData.legRatio);
    if (bodyPos > 0.0)
    {
        tp->renderData.margin1 = marginLeft;
        tp->renderData.margin2 = 0.0;
    }
    else
    {
        tp->renderData.margin1 = 0.0;
        tp->renderData.margin2 = marginLeft;
    }

    float ratio01 = 1.0 - (tp->renderData.legRatio - 0.25) / 0.25;
    ratio01 = pow(ratio01, 15.0);

    tp->renderData.ld.legThick = 0.12;
    tp->renderData.ld.circR = tp->renderData.legRatio * 0.5 * (1.0 - ratio01) + 10.0 * ratio01;
    tp->renderData.ld.legLen = tp->renderData.legRatio * 0.5;
    tp->renderData.ld.legOuterCircAng = tp->legAng;
    tp->renderData.ld.legAngle = tp->legAng;

    s_renderPlayerUbo* ru;
    vkMapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[1]->memory, tp->playerShader->alignedSize2 * imNum,
        tp->playerShader->alignedSize2, 0, (void**)&ru);
    memcpy((void*)ru, &tp->renderData, sizeof(s_renderPlayerUbo));
    vkUnmapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[1]->memory);
}

void TestPlayer::draw(VkCommandBuffer cmdBuff, void* ptr)
{
    TestPlayer* tp = (TestPlayer*)ptr;

    uint32_t imNum = ThinDrawer::object->lastSwapChainImageIndex;
    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * imNum;
    uint32_t dynOff2 = tp->playerShader->alignedSize2 * imNum;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, tp->playerShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &tp->playerShader->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        tp->playerShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    int i = 0;
    for (PortalBody* pb : *tp->pBody)
    {
        uint32_t dynOff1 = tp->playerShader->alignedSize * (i + imNum * 8);
        uint32_t dynOffsets[] = { dynOff1, dynOff2 };
        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            tp->playerShader->pipelineLayout, 1, 1, &tp->playerShader->descriptorSets[1], 2, dynOffsets);

        vkCmdDraw(cmdBuff, 6, 1, 0, 0);

        i += 1;
    }
}

void TestPlayer::drawGunEffect(VkCommandBuffer cmdBuff, void* ptr)
{
    TestPlayer* tp = (TestPlayer*)ptr;

    GunEffect* gunEffectShader = ThinDrawer::object->gunEffect;

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * ThinDrawer::object->lastSwapChainImageIndex;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, gunEffectShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &tp->playerShader->buffers[0]->buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        gunEffectShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    if (!tp->holdGun)
    {
        vkCmdPushConstants(cmdBuff, gunEffectShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gunData), (void*)&tp->gData);

        vkCmdDraw(cmdBuff, 6, 1, 0, 0);
    }
    else
    {
        b2Vec2 startPos = tp->holdGun->body->GetWorldPoint(b2Vec2(0.0f, 0.0f));
        b2Vec2 endPos = tp->holdGun->body->GetWorldPoint(b2Vec2(0.0f, 0.9f));

        s_gunData gData2 = { { startPos.x, startPos.y, endPos.x, endPos.y }, { 1.0f, 1.0f, 1.0f, 1.0f },
                             { NAN, 0.0f, 0.0f, 0.0f },                      { 2.5f, 0.0f, 0.0f, 0.0f } };

        portalConnection* pc = nullptr;
        if (tp->holdConnection)
        {
            pc = tp->holdConnection;
        }
        else if (tp->holdBody->bodyMaps->size() > 0)
        {
            pc = tp->holdBody->bodyMaps->at(0)->connection;
        }

        if (pc)
        {
            b2Vec2 p0 = pc->portal1->points[pc->side1 ^ 1];
            b2Vec2 p1 = pc->portal1->points[pc->side1];
            gData2.portal = glm::vec4(p0.x, p0.y, p1.x, p1.y);

            vkCmdPushConstants(cmdBuff, gunEffectShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gunData), (void*)&gData2);
            vkCmdDraw(cmdBuff, 6, 1, 0, 0);

            startPos = PortalWorld::getRelativePortalPosition_W(startPos, pc);
            endPos = PortalWorld::getRelativePortalPosition_W(endPos, pc);
            gData2.positions = glm::vec4(startPos.x, startPos.y, endPos.x, endPos.y);
            p0 = pc->portal2->points[pc->side2 ^ 1];
            p1 = pc->portal2->points[pc->side2];
            gData2.portal = glm::vec4(p0.x, p0.y, p1.x, p1.y);

            vkCmdPushConstants(cmdBuff, gunEffectShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gunData), (void*)&gData2);
            vkCmdDraw(cmdBuff, 6, 1, 0, 0);
        }
        else
        {
            vkCmdPushConstants(cmdBuff, gunEffectShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gunData), (void*)&gData2);
            vkCmdDraw(cmdBuff, 6, 1, 0, 0);
        }
    }

    for (int i = tp->shootDatas.size() - 1; i > -1; i--)
    {
        s_gunData* shootData = &tp->shootDatas.at(i);
        shootData->color.w -= Game::object->deltaTime * 3.0f;
        if (shootData->color.w <= 0.0)
        {
            tp->shootDatas.erase(tp->shootDatas.begin() + i);
            continue;
        }

        vkCmdPushConstants(cmdBuff, gunEffectShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gunData), (void*)shootData);
        vkCmdDraw(cmdBuff, 6, 1, 0, 0);
    }
}

void TestPlayer::renderInit()
{
    ThinDrawer::object->memMapFuncs.push_back({ memMap, this });
    ThinDrawer::object->drawFuncs.push_back({ draw, this });

    ThinDrawer::object->drawFuncs.push_back({ drawGunEffect, this });

    renderData.edgeColour = glm::vec4(1.0, 1.0, 1.0, 1.0);
    renderData.bodyColour1 = glm::vec4(0.0, 0.0, 0.0, 1.0);
    renderData.bodyColour2 = glm::vec4(0.04, 0.04, 0.04, 1.0);

    renderData.topMargin = 0.0;
    renderData.headRatio = 0.25;
    renderData.bodyRatio = 0.4;
    renderData.legRatio = 0.25;
    renderData.margin1 = (1.0 - renderData.headRatio - renderData.bodyRatio - renderData.legRatio) * 0.5;
    renderData.margin2 = (1.0 - renderData.headRatio - renderData.bodyRatio - renderData.legRatio) * 0.5;
    renderData.lineSize = 0.035;
    renderData.headCircles = glm::vec2(0.07, 0.07);
    renderData.bodyCircles = 0.1;

    renderData.ld.legThick = 0.1;
    renderData.ld.circR = renderData.legRatio * 1000.5;
    renderData.ld.legLen = 0.25 * 0.5;
    renderData.ld.legOuterCircAng = 0.0;
    renderData.ld.legAngle = renderData.ld.legOuterCircAng;
    renderData.ld.legCircleSize = 0.025;
    renderData.size = glm::vec2(0.75, 1.5);
}

void TestPlayer::createBody(b2Vec2 pos)
{
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = pos;
    bodyDef.fixedRotation = true;
    bodyDef.linearDamping = 0.0f;
    bodyDef.bullet = true;
    bodyDef.allowSleep = false;

    b2Body* body = pWorld->CreateBody(&bodyDef);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(bodySize.x * 0.5, bodySize.y * 0.5 - bodySize.x * 0.5);

    b2FixtureDef fDef;
    fDef.density = 1.0f;
    fDef.shape = &boxShape;
    fDef.friction = 0.0f;
    fDef.restitution = 0.0f;

    body->CreateFixture(&fDef);

    b2CircleShape cShape;
    cShape.m_radius = bodySize.x * 0.5;
    cShape.m_p = b2Vec2(0.0, -bodySize.y * 0.5 + bodySize.x * 0.5);
    tireFixture = body->CreateFixture(&cShape, 1.0);
    tireFixture->SetFriction(0.0f);

    cShape.m_p = b2Vec2(0.0, +bodySize.y * 0.5 - bodySize.x * 0.5);
    b2Fixture* fix = body->CreateFixture(&cShape, 1.0);
    fix->SetFriction(0.0f);

    PortalBody* pb = pWorld->createPortalBody(body);
    ThinDrawer::object->debug_addPortalBody(pb);

    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
    physicsData->type = DEFAULT_OBJECT;
    physicsData->data = this;
    bData->extraData = physicsData;

    pBody = &pb->worldIndex->pbIndices;

    gunSize = b2Vec2(0.3f, 0.8f);
    Gun* dGun = new Gun(pWorld, pos, gunSize);

    this->gun = dGun->pBody;

    b2RevoluteJointDef jDef;
    jDef.bodyA = pBody->at(0)->body;
    jDef.bodyB = gun->at(0)->body;
    jDef.collideConnected = false;
    jDef.localAnchorA = b2Vec2(0.0f, +0.35f);
    jDef.localAnchorB = b2Vec2(0.0f, -0.35f);
    jDef.calculateAnchorHelpers();

    pWorld->CreateJoint(&jDef);
}

void TestPlayer::preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2)
{
    if (holdBody)
    {
        bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
        if (bData && bData->type == B_PORTAL_BODY)
        {
            PortalBody* pb = (PortalBody*)bData->data;
            if (pb->worldIndex == holdBody->worldIndex)
            {
                contact->SetEnabled(false);
                holdColliding = true;
                return;
            }
        }
    }

    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);
    b2Vec2 normal = wManifold.normal;

    PortalBody* contactingPBody = (PortalBody*)((bodyData*)thisBody->GetUserData().pointer)->data;
    float tHold = 0.2f;
    b2Vec2 bodyPos = contactingPBody->body->GetPosition();
    b2Vec2 botVector = b2Vec2(0.0f, -1.0f);
    botVector = pWorld->rotateVec(botVector, thisBody->GetAngle());

    float botY = bodyPos.y + (bodySize.y / 2.0f) * botVector.y;
    if (wManifold.points[0].y < botY + tHold && wManifold.points[0].y > botY - tHold)
    {
        if (abs(normal.x) <= normXToWalk)
        {
            contact->SetFriction(setFriction);
        }
    }
    contact->SetRestitution(0.0f);
}

void TestPlayer::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    if (!contact->IsEnabled() || fix2->IsSensor())
    {
        return;
    }

    PortalBody* contactingPBody = (PortalBody*)((bodyData*)contactingBody->GetUserData().pointer)->data;

    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);
    b2Vec2 normal = wManifold.normal;

    b2Vec2 cPos = wManifold.points[0];

    if (contact->GetManifold()->pointCount > 1)
    {
        cPos += wManifold.points[1];
        cPos *= 0.5f;
    }

    float tHold = 0.25f;
    b2Vec2 bodyPos = contactingPBody->body->GetPosition();
    b2Vec2 botVector = b2Vec2(0.0f, -1.0f);
    botVector = pWorld->rotateVec(botVector, contactingBody->GetAngle());

    b2Vec2 botY = bodyPos + (bodySize.y / 2.0f) * botVector;
    ThinDrawer::object->registerDebugLine(glm::vec4(botY.x, botY.y, botY.x, botY.y), glm::vec4(1.0, 1.0, 1.0, 1.0), 0.1, 0.5);
    ThinDrawer::object->registerDebugLine(glm::vec4(cPos.x, cPos.y, cPos.x, cPos.y), glm::vec4(1.0, 0.0, 0.0, 1.0), 0.07, 0.5);
    if ((cPos - botY).LengthSquared() < tHold)
    {
        numNormals++;
        avgNormalX += normal.x;

        if (abs(normal.x) <= normXToWalk)
        {
            contactType = fix2->GetBody()->GetType();
            contactBody = fix2->GetBody();
            if (contactType == b2_dynamicBody)
            {
                if (fix2->GetDensity() == 0.0f &&
                    (contact->GetFixtureA()->GetBody() == pBody->at(0)->body || contact->GetFixtureB()->GetBody() == pBody->at(0)->body))
                {
                    platformBody = fix2->GetBody();
                }
            }
        }
        else if (normal.x > normXToWalk)
        {
            leftCollisionType = fix2->GetBody()->GetType();
        }
        else
        {
            rightCollisionType = fix2->GetBody()->GetType();
        }
    }
}

void TestPlayer::camHandle()
{
    WindowHandler* wh = game->thinDrawer->wh;

    glm::f64vec2 mp = wh->cam->getMouseCoords();
    glm::f64vec2 camDiff = glm::f64vec2(wh->mouseData[0] - wh->windowSizes.x * 0.5, wh->mouseData[1] - wh->windowSizes.y * 0.5);

    b2Vec2 bp = pBody->at(0)->body->GetPosition();
    glm::f64vec2 diff1 = (glm::f64vec2(bp.x, bp.y) - glm::f64vec2(wh->cam->pos.x, -wh->cam->pos.y)) * game->deltaTime * 4.0;
    glm::f64vec2 diff2 = (!holdBody ? (game->deltaTime * 0.04 / wh->cam->zoom) * glm::f64vec2(camDiff.x, -camDiff.y) : glm::f64vec2(0.0f, 0.0f));
    diff1.x += diff2.x;
    diff1.y += diff2.y;
    wh->cam->pos += glm::f64vec2(diff1.x, -diff1.y);

    wh->cam->updateOrtho();
}

void TestPlayer::handleBodyRotation()
{
    PortalBody* pb = pBody->at(0);
    float ang = PortalWorld::normalizeAngle(pb->body->GetAngle());
    if (ang > 0.0f)
    {
        for (PortalBody* l_portalBody : *pBody)
        {
            l_portalBody->body->SetAngularVelocity(0.0f);
        }
        pb->body->SetAngularVelocity(-18.0f * (abs(ang) / b2_pi));
    }
    else
    {
        for (PortalBody* l_portalBody : *pBody)
        {
            l_portalBody->body->SetAngularVelocity(0.0f);
        }
        pb->body->SetAngularVelocity(+18.0f * (abs(ang) / b2_pi));
    }
}

void TestPlayer::handleGunRotation()
{
    float ang;
    float mult = 1.0f;
    PortalBody* pb = pBody->at(0);
    PortalBody* gunBody = nullptr;
    PortalBody* playerBody = nullptr;
    b2RevoluteJoint* rj = nullptr;
    bool found = false;
    for (b2JointEdge* edge = pb->body->GetJointList(); edge != nullptr; edge = edge->next)
    {
        if (edge->joint->GetType() != e_revoluteJoint) continue;

        b2Body* otherBody;
        if (edge->joint->GetBodyA() == pb->body)
        {
            otherBody = edge->joint->GetBodyB();
        }
        else
        {
            otherBody = edge->joint->GetBodyA();
        }
        bodyData* data = (bodyData*)otherBody->GetUserData().pointer;
        if (data->type != B_PORTAL_BODY) continue;

        PortalBody* possibleGun = (PortalBody*)data->data;
        if (&possibleGun->worldIndex->pbIndices == gun)
        {
            found = true;
            gunBody = possibleGun;
            rj = (b2RevoluteJoint*)edge->joint;

            b2Vec2 gunCenter = pb->body->GetPosition() + b2Vec2(0.0f, 0.35f);
            glm::vec2 mousePos = wh->cam->getMouseCoords();
            b2Vec2 diff = b2Vec2(mousePos.x - gunCenter.x, mousePos.y - gunCenter.y);

            ang = gunBody->body->GetAngle() - PortalWorld::calcAngle2(diff) + b2_pi * 0.5f;
            if (gunBody->isMirrored) gunBody->body->GetAngle() - PortalWorld::calcAngle2(diff) - b2_pi * 0.5f;
            ang = PortalWorld::normalizeAngle(ang);

            playerBody = pb;
            break;
        }
    }

    if (!found)
    {
        b2RevoluteJoint* jointFound;
        gunBody = gun->at(0);
        for (b2JointEdge* edge = gunBody->body->GetJointList(); edge != nullptr; edge = edge->next)
        {
            if (edge->joint->GetType() != e_revoluteJoint) continue;

            b2Body* otherBody;
            if (edge->joint->GetBodyA() == gunBody->body)
            {
                otherBody = edge->joint->GetBodyB();
            }
            else
            {
                otherBody = edge->joint->GetBodyA();
            }

            bodyData* data = (bodyData*)otherBody->GetUserData().pointer;
            if (data->type == B_PORTAL_BODY && data->extraData != nullptr)
            {
                playerBody = (PortalBody*)data->data;
                jointFound = (b2RevoluteJoint*)edge->joint;
                break;
            }
        }

        if (playerBody)
        {
            portalConnection* pConnection = nullptr;
            for (bodyCollisionStatus* stat : *pb->bodyMaps)
            {
                if (stat->body == playerBody)
                {
                    pConnection = stat->connection;
                    break;
                }
            }

            if (pConnection)
            {
                b2Vec2 gunCenter = pb->body->GetPosition() + b2Vec2(0.0f, 0.35f);
                glm::vec2 mousePos = wh->cam->getMouseCoords();

                b2Vec2 diff = b2Vec2(mousePos.x - gunCenter.x, mousePos.y - gunCenter.y);
                b2Vec2 relPos = PortalWorld::getRelativePortalPosition_L(diff, pConnection);

                ang = gunBody->body->GetAngle() - PortalWorld::calcAngle2(relPos) - b2_pi * 0.5f;
                ang = PortalWorld::normalizeAngle(ang);

                rj = jointFound;
            }
        }
    }

    if (rj)
    {
        const float threshold = 0.0f;
        const float speedMult = 40.0f;
        rj->EnableMotor(true);
        rj->SetMaxMotorTorque(600.0f * gunBody->body->GetMass());

        if (rj->GetBodyA() == gunBody->body)
        {
            mult = -1.0f;
        }

        rj->SetMotorSpeed(speedMult * -ang * mult);
    }

    if (oldJoint != rj && oldJoint != nullptr)
    {
        for (PortalBody* l_portalBody : *pBody)
        {
            if (l_portalBody == oldPortalBody)
            {
                for (b2JointEdge* edge = oldPortalBody->body->GetJointList(); edge != nullptr; edge = edge->next)
                {
                    if (edge->joint == oldJoint)
                    {
                        oldJoint->EnableMotor(false);
                        break;
                    }
                }
                break;
            }
        }
    }

    if (rj)
    {
        oldJoint = rj;
        oldPortalBody = playerBody;
    }
}

b2Fixture* TestPlayer::canCreatePortal(b2Fixture* edgeFix, b2Vec2 pos, float size, Portal* oldPortal)
{
    fixtureData* fixData = (fixtureData*)edgeFix->GetUserData().pointer;

    b2Fixture* checkFixture;

    if (!fixData) return nullptr;
    else if (fixData->data == (void*)oldPortal)
    {
		return oldPortal->midFixture;
	}
    else if (fixData->type == F_PORTAL)
    {
        Portal* p = (Portal*)fixData->data;
        if (p->getPointSideVertical(pos) == 1)
        {
            checkFixture = p->sideFixtures[0];
        }
        else
        {
			checkFixture = p->sideFixtures[1];
		}

        if (!checkFixture) return nullptr;
    }
    else if (fixData->type == F_EDGE)
    {
        checkFixture = edgeFix;
    }
    else
    {
        return nullptr;
    }

    fixData = (fixtureData*)checkFixture->GetUserData().pointer;
    s_edgeLocalData* eLocalData = (s_edgeLocalData*)fixData->data;

    s_edgeGlobalData* eGlobalData = eLocalData->globalData;
    int rangeSize = eLocalData->globalRange[1] - eLocalData->globalRange[0];

    int requiredRangeSize = std::ceil(size / eGlobalData->perWallSize);
    requiredRangeSize += requiredRangeSize % 2;

    b2EdgeShape* eShape = (b2EdgeShape*)checkFixture->GetShape();
    pos = checkFixture->GetBody()->GetLocalPoint(pos);
    float dist = (pos - eGlobalData->localPoints[0]).Length() / eGlobalData->perWallSize;

    int startIndex = std::min(std::max((int)std::floor(dist), eLocalData->globalRange[0]), eLocalData->globalRange[1]);

    if (startIndex > -1 && startIndex < eGlobalData->unitWalls.size())
    {
        s_unitWallData ud = eGlobalData->unitWalls[startIndex];

        if (ud.wallType != PORTALLABLE && (ud.gelAmount <= 3 || ud.gelType != WHITE))
        {
            return nullptr;
        }
    }

    int checkRange[2] = { eLocalData->globalRange[0], eLocalData->globalRange[1] };

    for (int i = 0; i < 2; i++)
    {
        if (eLocalData->prevNextFixtures[i])
        {
            fixtureData* fData = (fixtureData*)eLocalData->prevNextFixtures[i]->GetUserData().pointer;
            if (fData && fData->data == oldPortal)
            {
                checkRange[i] = i == 0 ? std::min(checkRange[0], oldPortal->range[0]) : std::max(checkRange[1], oldPortal->range[1]);
                checkFixture = oldPortal->midFixture;
                break;
            }
        }
    }

    int leftCount = 0;
    int rightCount = 0;
    int lastSum = 0;

    while (true)
    {
        if (startIndex + leftCount + 1 <= checkRange[1])
        {
            s_unitWallData& uData = eGlobalData->unitWalls[startIndex + leftCount + 1];
            if (uData.wallType == PORTALLABLE || (uData.gelAmount > 3 && uData.gelType & WHITE))
            {
                leftCount++;

                if (leftCount + rightCount == requiredRangeSize)
                {
                    break;
                }
            }
        }

        if (startIndex - rightCount - 1 >= checkRange[0])
        {
            s_unitWallData& uData = eGlobalData->unitWalls[startIndex - rightCount - 1];
            if (uData.wallType == PORTALLABLE || (uData.gelAmount > 3 && uData.gelType & WHITE))
            {
                rightCount++;

                if (leftCount + rightCount == requiredRangeSize)
                {
                    break;
                }
            }
        }

        if (lastSum == rightCount + leftCount)
        {
            return nullptr;
        }

        lastSum = leftCount + rightCount;
    }

    return checkFixture;
}

void TestPlayer::getBodyOut()
{
    PortalBody::getBodyOut(pBody->at(0)->worldIndex);
    PortalBody::getBodyOut(gun->at(0)->worldIndex);

    b2Body* playerBody = pBody->at(0)->body;
    b2Body* gunBody = gun->at(0)->body;

    bool jointFound = false;
    for (b2JointEdge* edge = gunBody->GetJointList(); edge; edge = edge->next)
    {
        if (edge->joint->GetBodyA() == playerBody || edge->joint->GetBodyB() == playerBody)
        {
            jointFound = true;
            break;
        }
    }

    if (!jointFound)
    {
        gunBody->SetTransform(playerBody->GetPosition(), b2_pi + playerBody->GetAngle());

        b2RevoluteJointDef jDef;
        jDef.bodyA = playerBody;
        jDef.bodyB = gunBody;
        jDef.collideConnected = false;
        jDef.localAnchorA = b2Vec2(0.0f, +0.35f);
        jDef.localAnchorB = b2Vec2(0.0f, -0.35f);
        jDef.calculateAnchorHelpers();

        pWorld->CreateJoint(&jDef);
    }
}

PortalBody* TestPlayer::getOuterGun()
{
    PortalBody* portalGun = gun->at(0);
    for (PortalBody* gb : *gun)
    {
        bool ok = true;
        b2Vec2 p = gb->body->GetWorldPoint(b2Vec2(0.0, -0.35));
        for (bodyCollisionStatus* bcs : *gb->bodyMaps)
        {
            if (bcs->connection->portal1->getPointSide(p) != bcs->connection->side1)
            {
                ok = false;
                break;
            }
        }

        if (ok)
        {
            portalGun = gb;
            break;
        }
    }

    return portalGun;
}

void TestPlayer::setXColor(bool canBlue, bool canRed)
{
    if (canBlue && canRed)
    {
        gData.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else if (canBlue)
    {
        gData.color = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
    }
    else if (canRed)
    {
        gData.color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    }
    else
    {
        gData.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

void TestPlayer::handlePortalCD(float dt)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    const float portalSize = 2.5f;
    const float rayLength = 100.0f;

    PortalBody* portalGun = getOuterGun();
    b2Vec2 p0 = portalGun->body->GetWorldPoint(b2Vec2(0.0, -0.35));

    b2Vec2 vec;

    float vecAngle = b2_pi * 0.5f;
    vecAngle += portalGun->body->GetAngle();
    vec = b2Vec2(cos(vecAngle) * rayLength, sin(vecAngle) * rayLength);

    PortalGunRayCast rayCaster = PortalGunRayCast(pWorld);

    b2Vec2 rayPoint1 = p0;
    b2Vec2 rayPoint2 = p0 + vec;

    handleObjectHold(portalGun, p0, vec, dt);
    if (holdBody)
    {
        return;
    }

    thinDrawer->registerDebugLine(glm::vec4(rayPoint1.x, rayPoint1.y, rayPoint2.x, rayPoint2.y), glm::vec4(1.0, 1.0, 1.0, 1.0), 0.03, 0.5);

    rayCaster.cast(rayPoint1, rayPoint2);

    if (rayCaster.found && !wh->keyData[GLFW_KEY_LEFT_CONTROL])
    {
        b2Fixture* canCreateBlue = canCreatePortal(rayCaster.edgeFix, rayCaster.rayPoint, portalSize, portalA);
        b2Fixture* canCreateRed = canCreatePortal(rayCaster.edgeFix, rayCaster.rayPoint, portalSize, portalB);

        setXColor(canCreateBlue, canCreateRed);

        glm::vec2 np1 = glm::vec2(rayCaster.rayPoint.x, rayCaster.rayPoint.y);
        gData.positions.z = np1.x;
        gData.positions.w = np1.y;

        glm::vec2 diff = np1 - glm::vec2(p0.x, p0.y);
        float minLen = (rayCaster.rayPoint - p0).Length();
        float len = glm::length(diff);
        len = fmin(fmax(minLen, len), 1.0f);
        glm::vec2 np0 = np1 - glm::normalize(diff) * len;

        gData.positions.x = np0.x;
        gData.positions.y = np0.y;

        if (wh->mouseData[2] > 0 && gameTime - lastPortalCreationTime >= 0.0f)
        {
            if (canCreateBlue)
            {
                if (portalA)
                {
                    getBodyOut();
                    b2Fixture* remFix = portalA->getPreDestructionFixture();
                    if (canCreateBlue == portalA->midFixture)
                    {
                        canCreateBlue = remFix;
                    }

                    pWorld->destroyPortal(portalA);
                    portalA = nullptr;
                }

                Portal* portal = pWorld->createPortalWithChecks(canCreateBlue, rayCaster.rayPoint, portalSize);

                shootDatas.push_back({  { p0.x, p0.y, portal->pos.x, portal->pos.y },   { 0.0, 0.5f, 1.0f, 1.0f },
                                        { NAN, 0.0f, 0.0f, 0.0f },                      { 1.0f, 0.0f, 0.0f, 0.0f } });

                ALuint source = Sounder::object->playSound(blueShootBuffer, 1.0);
                b2Vec2 pPos = rayCaster.rayPoint;
                Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));

                portal->reviveAround();
                portalA = portal;

                auto iter = thinDrawer->renderPortalData.find(portal);
                for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
                {
                    s_portalUbo* pUbo;
                    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->portalShader->uniformBuffers[0]->memory,
                        iter->second.dynamicOffsets[i], thinDrawer->portalShader->alignedSize, 0, (void**)&pUbo);
                    pUbo->color = glm::vec4(0.0f, 0.5f, 1.0f, 0.0f);
                    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->portalShader->uniformBuffers[0]->memory);
                }

                if (portalB)
                {
                    portalA->connect(portalB, true);
                    portalB->reviveAround();

                    auto iter2 = thinDrawer->renderPortalData.find(portalB);

                    for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
                    {
                        s_portalParticleUbo* ppUbo;
                        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->portalParticleShader->uniformBuffers[0]->memory,
                            iter->second.particleDynamicOffsets[i], thinDrawer->portalParticleShader->alignedSize, 0, (void**)&ppUbo);
                        ppUbo->data0.z = thinDrawer->gameTime;
                        ppUbo->data1.z = thinDrawer->gameTime;
                        ppUbo->color = glm::vec4(0.0f, 0.5f, 1.0f, 0.0f);
                        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->portalParticleShader->uniformBuffers[0]->memory);

                        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->portalParticleShader->uniformBuffers[0]->memory,
                            iter2->second.particleDynamicOffsets[i], thinDrawer->portalParticleShader->alignedSize, 0, (void**)&ppUbo);
                        ppUbo->data0.z = thinDrawer->gameTime;
                        ppUbo->data1.z = thinDrawer->gameTime;
                        ppUbo->color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
                        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->portalParticleShader->uniformBuffers[0]->memory);
                    }
                }
            }

            lastPortalCreationTime = gameTime;
        }
        else if (wh->mouseData[3] > 0 && gameTime - lastPortalCreationTime >= 0.0f)
        {
            if (canCreateRed)
            {
                if (portalB)
                {
                    getBodyOut();
                    b2Fixture* remFix = portalB->getPreDestructionFixture();
                    if (canCreateRed == portalB->midFixture)
                    {
                        canCreateRed = remFix;
                    }
                    pWorld->destroyPortal(portalB);
                    portalB = nullptr;
                }

                Portal* portal = pWorld->createPortalWithChecks(canCreateRed, rayCaster.rayPoint, portalSize);


                shootDatas.push_back({  { p0.x, p0.y, portal->pos.x, portal->pos.y },   { 1.0, 0.5f, 0.0f, 1.0f },
                                        { NAN, 0.0f, 0.0f, 0.0f },                      { 1.0f, 0.0f, 0.0f, 0.0f } });

                ALuint source = Sounder::object->playSound(orangeShootBuffer, 1.0);
                b2Vec2 pPos = rayCaster.rayPoint;
                Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));

                portal->reviveAround();
                portalB = portal;

                auto iter = thinDrawer->renderPortalData.find(portal);
                for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
                {
                    s_portalUbo* pUbo;
                    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->portalShader->uniformBuffers[0]->memory,
                        iter->second.dynamicOffsets[i], thinDrawer->portalShader->alignedSize, 0, (void**)&pUbo);
                    pUbo->color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
                    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->portalShader->uniformBuffers[0]->memory);
                }

                if (portalA)
                {
                    portalB->connect(portalA);
                    portalA->reviveAround();

                    auto iter2 = thinDrawer->renderPortalData.find(portalA);

                    for (int i = 0; i < thinDrawer->swapChain->imageCount; i++)
                    {
                        s_portalParticleUbo* ppUbo;
                        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->portalParticleShader->uniformBuffers[0]->memory,
                            iter->second.particleDynamicOffsets[i], thinDrawer->portalParticleShader->alignedSize, 0, (void**)&ppUbo);
                        ppUbo->data0.z = thinDrawer->gameTime;
                        ppUbo->data1.z = thinDrawer->gameTime;
                        ppUbo->color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
                        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->portalParticleShader->uniformBuffers[0]->memory);

                        vkMapMemory(thinDrawer->logicalDevice, thinDrawer->portalParticleShader->uniformBuffers[0]->memory,
                            iter2->second.particleDynamicOffsets[i], thinDrawer->portalParticleShader->alignedSize, 0, (void**)&ppUbo);
                        ppUbo->data0.z = thinDrawer->gameTime;
                        ppUbo->data1.z = thinDrawer->gameTime;
                        ppUbo->color = glm::vec4(0.0f, 0.5f, 1.0f, 0.0f);
                        vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->portalParticleShader->uniformBuffers[0]->memory);
                    }
                }
            }

            lastPortalCreationTime = gameTime;
        }
    }
}

void TestPlayer::gunDestrCallback(PortalBody* gb, void* ptr)
{
    TestPlayer* tp = (TestPlayer*)ptr;

    if (gb != tp->holdGun)
    {
        return;
    }

    bodyCollisionStatus* bcs = gb->bodyMaps->at(0);
    tp->holdGun = bcs->body;

    if (tp->holdConnection == nullptr)
    {
        tp->holdConnection = bcs->connection;
    }
    else if (tp->holdConnection->portal1 == bcs->connection->portal1 || tp->holdConnection->portal2 == bcs->connection->portal1)
    {
        tp->holdConnection = nullptr;
    }
    else
    {
        tp->shouldRelease = true;
    }
}

void TestPlayer::objectDestrCallback(PortalBody* pb, void* ptr)
{
    TestPlayer* tp = (TestPlayer*)ptr;

    if (pb != tp->holdBody)
    {
        return;
    }

    bodyCollisionStatus* bcs = pb->bodyMaps->at(0);
    tp->holdBody = bcs->body;
    if (bcs->connection->portal1 == tp->destrPortal || bcs->connection->portal2 == tp->destrPortal)
    {
        tp->shouldRelease = true;
        return;
    }

    if (tp->holdConnection == nullptr)
    {
        tp->holdConnection = bcs->connection;
    }
    else if (tp->holdConnection->portal1 == bcs->connection->portal1 || tp->holdConnection->portal2 == bcs->connection->portal1)
    {
        tp->holdConnection = nullptr;
    }
    else
    {
        tp->shouldRelease = true;
    }
}

void TestPlayer::holdB(PortalBody* pb, PortalBody* gunBody)
{
    *pb->gravityScale = 0.0f;
    holdBody = pb;
    holdGun = gunBody;
    lastHoldPos = pb->body->GetPosition();

    holdBody->worldIndex->destroyCallbacks.push_back(std::make_pair(objectDestrCallback, (void*)this));
    holdGun->worldIndex->destroyCallbacks.push_back(std::make_pair(gunDestrCallback, (void*)this));

    shouldRelease = false;

    int i = 0;
    holdRestitutions.resize(pb->numFixtures);
    holdFrictions.resize(pb->numFixtures);
    for (b2Fixture* fix : pb->fixtures)
    {
        holdRestitutions[i] = fix->GetRestitution();
        holdFrictions[i++] = fix->GetFriction();
    }

    for (PortalBody* pbA : pb->worldIndex->pbIndices)
    {
        for (b2ContactEdge* edge = pbA->body->GetContactList(); edge; edge = edge->next)
        {
            edge->contact->SetRestitution(0.0f);
            edge->contact->SetFriction(0.0f);
        }

        for (b2Fixture* fix : pbA->fixtures)
        {
            fix->SetRestitution(0.0f);
            fix->SetFriction(0.0f);
        }
    }

    holdERequestTime = -1.0f;
}

void TestPlayer::releaseB()
{
    pbWorldData* indx = holdBody->worldIndex;
    for (auto iter = indx->destroyCallbacks.rbegin(); iter != indx->destroyCallbacks.rend(); iter++)
    {
        if ((*iter).first == objectDestrCallback)
        {
            indx->destroyCallbacks.erase(iter.base() - 1);
            break;
        }
    }

    for (PortalBody* pb : indx->pbIndices)
    {
        b2Vec2 vel = pb->body->GetLinearVelocity();
        if (vel.Length() > 10.0f)
        {
            vel.Normalize();
            pb->body->SetLinearVelocity(10.0f * vel);
        }
    }

    indx = holdGun->worldIndex;
    for (auto iter = indx->destroyCallbacks.rbegin(); iter != indx->destroyCallbacks.rend(); iter++)
    {
        if ((*iter).first == gunDestrCallback)
        {
            indx->destroyCallbacks.erase(iter.base() - 1);
            break;
        }
    }

    for (PortalBody* pbA : holdBody->worldIndex->pbIndices)
    {
        for (b2ContactEdge* edge = pbA->body->GetContactList(); edge; edge = edge->next)
        {
            edge->contact->SetRestitution(holdRestitutions[0]);
            edge->contact->SetFriction(holdFrictions[0]);
        }

        int i = 0;
        for (b2Fixture* fix : pbA->fixtures)
        {
            fix->SetRestitution(holdRestitutions[i]);
            fix->SetFriction(holdFrictions[i++]);
        }
    }
    holdRestitutions.clear();
    holdFrictions.clear();

    *holdBody->gravityScale = 1.0f;
    holdBody = nullptr;
    holdGun = nullptr;
    holdConnection = nullptr;
    releaseRequestTime = -1.0f;
    holdERequestTime = -1.0f;
}

void TestPlayer::handleObjectHold(PortalBody* gunBody, b2Vec2 pos, b2Vec2 dir, float dt)
{
    object->destrPortal = nullptr;

    if (holdERequestTime < 0.0f && wh->keyData[GLFW_KEY_E] == 2 && gameTime - lastBodyHoldTime > 0.3)
    {
        holdERequestTime = gameTime;
        lastBodyHoldTime = gameTime;
    }
    else if (holdERequestTime > 0.0f && gameTime - holdERequestTime > 0.3f)
    {
        holdERequestTime = -1.0f;
    }

    if (holdERequestTime > 0.0f && gameTime - holdERequestTime < 0.3f)
    {
        if (holdBody)
        {
            if (!holdColliding)
            {
                releaseB();
            }
        }
        else
        {
            pWorld->rayHandler->sendRay(pos, dir, 1.5f, pBody);
            if (pWorld->rayHandler->rayResult.size() < 3 && pWorld->rayHandler->closestFixture)
            {
                bodyData* bData = (bodyData*)pWorld->rayHandler->closestFixture->GetBody()->GetUserData().pointer;
                bodyData* bData2;
                if (contactBody)
                {
                    bData2 = (bodyData*)contactBody->GetUserData().pointer;
                }
                else
                {
                    bData2 = nullptr;
                }

                if (bData && bData->extraData && bData->type == B_PORTAL_BODY && bData2 != bData)
                {
                    g_PhysicsData* pData = (g_PhysicsData*)bData->extraData;
                    if (pData && pData->type == DEFAULT_OBJECT)
                    {
                        PortalBody* pb = (PortalBody*)bData->data;
                        holdB(pb, gunBody);

                        if (pWorld->rayHandler->rayResult.size() == 2 && pWorld->rayHandler->rayResult[0].portal)
                        {
                            portalConnection* revConnection = pWorld->rayHandler->rayResult[0].portal->connections[0][0];
                            holdConnection = revConnection->portal2->connections[0][0];
                        }
                    }
                }
            }
        }
    }

    if (holdBody)
    {
        if (shouldRelease)
        {
            releaseB();
            return;
        }
        else if (releaseRequestTime > 0.0f && gameTime - releaseRequestTime > 0.15f)
        {
            releaseB();
            return;
        }
        else
        {
            if (holdGun != gunBody)
            {
                if (gunBody->bodyMaps->size() == 0)
                {
                    releaseB();
                    return;
                }

                portalConnection* newConnection = gunBody->bodyMaps->at(0)->connection;
                if (holdConnection == nullptr)
                {
                    holdConnection = newConnection;
                }
                else if (holdConnection->portal1 == newConnection->portal1 || holdConnection->portal2 == newConnection->portal1)
                {
                    holdConnection = nullptr;
                }
                else
                {
                    releaseB();
                    return;
                }

                holdGun = gunBody;
            }

            dir.Normalize();
            b2Vec2 desiredPos = pos + 1.25f * dir;
            if (holdConnection)
            {
                desiredPos = pWorld->getRelativePortalPosition_W(desiredPos, holdConnection);
            }

            ThinDrawer::object->registerDebugLine(glm::vec4(pos.x, pos.y, desiredPos.x, desiredPos.y), glm::vec4(1.0, 0.0, 1.0, 1.0), 0.15, 0.5);
            b2Vec2 diff = desiredPos - holdBody->body->GetPosition();
            float diffLen = diff.Length();
            diffLen = pow(diffLen, 2.0f);
            diffLen = fmin(diffLen, 5.0f);

            diff.Normalize();
            diff = diffLen * diff;

            holdAngle += angVal;
            if (holdAngle > b2_pi / 8.0f)       angVal = -b2_pi * 0.02f;
            else if (holdAngle < -b2_pi / 8.0f) angVal = +b2_pi * 0.02f;


            if (!holdColliding)
            {
                b2Vec2 newDir = PortalWorld::rotateVec(dir, holdAngle);
                pWorld->rayHandler->sendRay(pos, newDir, 1.5f, pBody);
                if (pWorld->rayHandler->closestFixture)
                {
                    bool rel = true;
                    bodyData* bData = (bodyData*)pWorld->rayHandler->closestFixture->GetBody()->GetUserData().pointer;
                    if (bData && bData->extraData && bData->type == B_PORTAL_BODY)
                    {
                        PortalBody* pb = (PortalBody*)bData->data;
                        if (pb->worldIndex == holdBody->worldIndex)
                        {
                            rel = false;
                        }
                    }

                    if (rel)
                    {
                        if (releaseRequestTime < 0.0) releaseRequestTime = gameTime;
                    }
                    else
                    {
                        releaseRequestTime = -1.0f;
                    }
                }
                else
                {
                    if (releaseRequestTime < 0.0) releaseRequestTime = gameTime;
                }
            }
            else
            {
                releaseRequestTime = -1.0f;
            }

            b2Vec2 relVel = gunBody->body->GetLinearVelocity();
            if (holdConnection)
            {
                relVel = -pWorld->getRelativePortalPosition_L(relVel, holdConnection);
            }

            b2Vec2 vv = holdBody->body->GetLinearVelocity();
            float vvLen = vv.Length();
            float m = 0.9f;
            b2Vec2 newVel = m * vv + 1.0f * diff + dt * 30.0f * relVel;
            //newVel = PortalWorld::project(newVel, diff);

            holdBody->body->SetLinearVelocity(newVel);

            float rotRatio = 1.0f - dt * 0.8f;
            holdBody->body->SetAngularVelocity(fmin(holdBody->body->GetAngularVelocity() * rotRatio, b2_pi));
        }

        lastHoldPos = holdBody->body->GetPosition();
    }
}

void TestPlayer::updateReset()
{
    l_contactBody = contactBody;
    l_platformBody = platformBody;

    avgNormalX = 0.0;
    numNormals = 0;
    contactBody = nullptr;
    platformBody = nullptr;
    right = false;
    left = false;
    contactType = (b2BodyType)INT_MAX; // NULL
    rightCollisionType = b2_dynamicBody;
    leftCollisionType = b2_dynamicBody;
    lastBody = pBody->at(0);
    holdColliding = false;
}

void TestPlayer::update(float dt, int totalIter)
{
    gameTime = Game::object->gameTime;

    const float maxSpeed = 6.0f;
    float speedOffset = 0.0f;
    avgNormalX /= numNormals;
    setFriction = 0.0;
    PortalBody* pb = pBody->at(0);

    if (pb != lastBody && lastBody != nullptr)
    {
        bodySwitch = true;
        switchTimer = gameTime;
        float diff = abs(lastDirs[0].x + lastDirs[1].x);
        if (lastKey != -1 && diff > 1.0f)
        {
            keyBeforeSwitch = lastKey;
        }
    }
    if (bodySwitch)
    {
        if (gameTime - switchTimer > totalTime)
        {
            bodySwitch = false;
            lastBody = nullptr;
        }
    }

    if (platformBody)
    {
        speedOffset = platformBody->GetLinearVelocity().x;
    }

    if (wh->keyData[GLFW_KEY_D] && wh->keyData[GLFW_KEY_A])
    {
        if (lastDoubleKey == -1)
        {
            right = true;
        }
        else if (lastDoubleKey == GLFW_KEY_A)
{
            right = true;
        }
        else if (lastDoubleKey == GLFW_KEY_D)
{
            left = true;
        }
    }
    else if (wh->keyData[GLFW_KEY_D])
{
        right = true;
    }
    else if (wh->keyData[GLFW_KEY_A])
{
        left = true;
    }

    if (pBody->size() > 1)
    {
        PortalBody* b1 = pBody->at(0);
        PortalBody* b2 = pBody->at(1);
        fixtureData* fixData1 = (fixtureData*)b1->body->GetFixtureList()->GetUserData().pointer;
        PBFixtureData* pbfData1 = (PBFixtureData*)fixData1->data;
        fixtureData* fixData2 = (fixtureData*)b2->body->GetFixtureList()->GetUserData().pointer;
        PBFixtureData* pbfData2 = (PBFixtureData*)fixData2->data;

        if (pbfData1->pCollisions.size() > 0 && pbfData2->pCollisions.size() > 0)
        {
            portalCollision* pc1 = pbfData1->pCollisions.at(0);
            portalCollision* pc2 = pbfData2->pCollisions.at(0);

            lastDirs[0] = pc1->side ? -pc1->portal->dir : pc1->portal->dir;
            lastDirs[1] = pc2->side ? -pc2->portal->dir : pc2->portal->dir;
        }
    }

    if (!(wh->keyData[GLFW_KEY_A] && wh->keyData[GLFW_KEY_D]))
    {
        if (wh->keyData[GLFW_KEY_A])
        {
            lastDoubleKey = GLFW_KEY_A;
        }
        else if (wh->keyData[GLFW_KEY_D])
        {
            lastDoubleKey = GLFW_KEY_D;
        }
        else
        {
            lastDoubleKey = -1;
        }
    }

    float mult = 1.0f;
    if (left || right)
    {
        if (right)
        {
            if (keyBeforeSwitch == GLFW_KEY_D)
            {
                mult = -1.0f;
            }
            else
            {
                keyBeforeSwitch = -1;
            }
        }
        else if (left)
        {
            if (keyBeforeSwitch == GLFW_KEY_A)
            {
                mult = -1.0f;
            }
            else
            {
                keyBeforeSwitch = -1;
            }
        }
    }
    else
    {
        keyBeforeSwitch = -1;
    }

    lastKey = -1;
    b2Vec2 lv = pb->body->GetLinearVelocity();

    if (!left && !right && platformBody)
    {
        //printf("1\n");
        float diff = platformBody->GetLinearVelocity().x - lv.x;
        lv.x += diff * dt * 15.0f * pBody->size();
        setFriction = 2.0;
    }
    if (left || right)
    {
        //printf("2\n");
        const float groundMult = 40.0f;
        const float airMult = 10.0f;
        const float fwdGrndSlw = 5.0f;
        if (right)
        {
            float addition;
            if (contactBody != nullptr)
            {
                addition = groundMult * dt * pBody->size() * mult;
            }
            else if (rightCollisionType == b2_dynamicBody)
            {
                addition = airMult * dt * pBody->size() * mult;
                if (lv.x < -maxSpeed * 2.0)
                {
                    addition = 0.0;
                }
            }
            else
            {
                addition = 0.0f;
                setFriction = 2.0;
            }

            if (addition != 0.0f)
            {
                lv.x += addition;
                if (lv.x > maxSpeed + speedOffset)
                {
                    if (contactBody != nullptr)
                    {
                        if (lv.x - addition > maxSpeed + speedOffset)
                        {
                            lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * fwdGrndSlw);
                        }
                        else
                        {
                            lv.x = maxSpeed + speedOffset;
                        }
                    }
                    lv.x = fmax(maxSpeed + speedOffset, lv.x - addition);
                }
                else if (lv.x < -maxSpeed + speedOffset)
                {
                    if (rightCollisionType == b2_dynamicBody && mult > 0.0f)
                    {
                        lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * 5.0f);
                        lv.x = lv.x - addition;
                    }
                    else
                    {
                        if (contactBody != nullptr)
                        {
                            if (lv.x - addition < -maxSpeed + speedOffset)
                            {
                                lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * fwdGrndSlw);
                            }
                            else
                            {
                                lv.x = -maxSpeed + speedOffset;
                            }
                        }
                        lv.x = fmin(-maxSpeed + speedOffset, lv.x - addition);
                    }
                }
            }

            lastKey = GLFW_KEY_D;
        }
        if (left)
        {
            float subtraction;
            if (contactBody != nullptr)
            {
                subtraction = groundMult * dt * pBody->size() * mult;
            }
            else if (leftCollisionType == b2_dynamicBody)
            {
                subtraction = airMult * dt * pBody->size() * mult;
                if (lv.x > maxSpeed * 2.0)
                {
                    subtraction = 0.0;
                }
            }
            else
            {
                subtraction = 0.0f;
                setFriction = 2.0;
            }

            if (subtraction != 0.0f)
            {
                lv.x -= subtraction;
                if (lv.x > maxSpeed + speedOffset)
                {
                    if (rightCollisionType == b2_dynamicBody && mult > 0.0f)
                    {
                        lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * 5.0f);
                        lv.x = lv.x + subtraction;
                    }
                    else
                    {
                        if (contactBody != nullptr && mult < 0.0f)
                        {
                            if (lv.x + subtraction > maxSpeed + speedOffset)
                            {
                                lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * fwdGrndSlw);
                            }
                            else
                            {
                                lv.x = maxSpeed + speedOffset;
                            }
                        }
                        lv.x = fmax(maxSpeed + speedOffset, lv.x + subtraction);
                    }
                }
                else if (lv.x < -maxSpeed + speedOffset)
                {
                    if (contactBody != nullptr)
                    {
                        if (lv.x + subtraction < -maxSpeed + speedOffset)
                        {
                            lv.x *= fmax(0.0f, 1.0f - dt * (float)pBody->size() * fwdGrndSlw);
                        }
                        else
                        {
                            lv.x = -maxSpeed + speedOffset;
                        }
                    }

                    lv.x = fmin(-maxSpeed + speedOffset, lv.x + subtraction);
                }
            }

            lastKey = GLFW_KEY_A;
        }
    }
    else if (!contactBody)
    {
        //printf("3\n");
        setFriction = 0.0;
    }
    else if (contactBody && contactBody->GetMass() != 0.0f && onAirFor == 0.0f)
    {
        //printf("4\n");
        // kinematic body
        setFriction = 2.0;
    }
    else if (contactBody && contactType != b2_dynamicBody && onAirFor == 0.0f)
    {
        //printf("5\n");
        setFriction = 2.0;
    }

    if (!contactBody) onAirFor += dt;
    else onAirFor = 0.0f;

    static double jumpPressTime = 0.0;

    grounded = ((contactBody || abs(avgNormalX) < normXToWalk) && lv.y <= 5.0f) || (lv.y < 0.0f && onAirFor < 0.2f && onAirFor != 0.0f);

    if (wh->keyData[GLFW_KEY_W] == 2)
    {
        jumpPressTime = gameTime;
    }

    if (grounded && gameTime < jumpPressTime + 0.04)
    {
        lv.y = 7.5f * (float)pBody->size();

        jumpPressTime = 0.0;

        ALuint source = Sounder::object->playSound(jumpBuffer, 1.0);
        b2Vec2 pPos = pb->body->GetPosition();
        Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));
    }

    lv.y = std::fmax(lv.y, -34.0);
    //printf("%f, %f\n", lv.x, lv.y);

    pb->body->SetLinearVelocity(lv);

    handleBodyRotation();

    handleGunRotation();
    
    handlePortalCD(dt);

    updateReset();
}