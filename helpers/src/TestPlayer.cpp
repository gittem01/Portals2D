#include <TestPlayer.h>
#include <GameObjects/ForceField.h>
#include "GameObjects/UseButton.h"
#include "GameObjects/DownButton.h"
#include "GameObjects/CubeButton.h"
#include "GameObjects/CircleButton.h"
#include "GameObjects/LaserEmitter.h"
#include "GameObjects/GndLaserReceiver.h"
#include "GameObjects/WallLaserReceiver.h"
#include "GameObjects/Funnel.h"
#include "GameObjects/Catapult.h"
#include "GameObjects/Dropper.h"
#include "GameObjects/LightBridge.h"
#include "GameObjects/EndCapsule.h"

TestPlayer* TestPlayer::object;

static splashData sData;

TestPlayer::TestPlayer(Game* game, b2Vec2 pos, b2Vec2 size) : DefaultObject(game->pWorld)
{
    TestPlayer::object = this;

    initialPos = pos;

    this->game = game;
    wh = game->thinDrawer->wh;
    bodySize = size;
    createBody(pos);

    gameTime = 0.0;

    init();

    jumpBuffer = Sounder::object->bufferMap["jumpTest.wav"];
    blueJumpBuffer = Sounder::object->bufferMap["blueGelJump.wav"];
    blueShootBuffer = Sounder::object->bufferMap["blueShoot.wav"];
    orangeShootBuffer = Sounder::object->bufferMap["orangeShoot.wav"];
    blueShootBufferJ = Sounder::object->bufferMap["blueShootJ.wav"];
    orangeShootBufferJ = Sounder::object->bufferMap["orangeShootJ.wav"];
    fizzleBuffer = Sounder::object->bufferMap["portalFizzle.wav"];
    walkBuffer = Sounder::object->bufferMap["walkloop.wav"];

    pWorld->portalDestroyCallbacks.push_back(portalDestrCallBack);
}

void TestPlayer::init()
{
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

    won = false;
    walkPlaying = false;

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
    contactFixture = nullptr;
    platformBody = nullptr;
    l_contactBody = nullptr;
    l_platformBody = nullptr;
    oldJoint = nullptr;
    contactType = (b2BodyType)INT_MAX;

    health = 100;

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

    shootDatas.clear();
    endSplashes.clear();

    sData.posSpeed.z = (float)rand() / RAND_MAX;
    sData.posSpeed.w = 1.0;
    sData.data.x = 0.075;
    sData.data.y = ThinDrawer::object->gameTime;
    sData.data.z = b2_pi * 2.0;
    sData.data.w = 0.0;

    sData.color.w = 0.4;

    sData.data2.z = 10.0;
    sData.data2.x = 2.0;
    sData.data2.y = 10.0;
    sData.data2.w = 0.3;

    sData.data3.x = 0.1;
    sData.data3.y = 0.75;
    sData.data3.z = 1000000.0;
    sData.data3.w = 0.01;
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

void write0(TestPlayer* tp)
{
    uint32_t imNum = ThinDrawer::object->frameIndex;

	bool initialMirrored = tp->wData->pbIndices[0]->isMirrored;

    int i = 0;
    for (PortalBody* pb : tp->wData->pbIndices)
    {
        b2Body* body = pb->body;

        s_dRenderPlayerUbo* ru2;
        vkMapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[0].memory, tp->playerShader->alignedSize * (i + imNum * 8),
            tp->playerShader->alignedSize, 0, (void**)&ru2);

        b2Vec2 pos = body->GetPosition();
        ru2->posRotD = glm::vec4(pos.x, pos.y, body->GetAngle(), pb->isMirrored ? -1.0f : 1.0f);

        std::set<portalCollision*> pcs;
        for (b2Fixture* f : pb->fixtures)
        {
            fixtureData* fixData = (fixtureData*)f->GetUserData().pointer;
            PBFixtureData* data = (PBFixtureData*)fixData->data;
            for (portalCollision& pc : data->pCollisions)
            {
                pcs.insert(&pc);
            }
        }

        int j = 0;
        for (portalCollision* pc : pcs)
        {
            ru2->portals[j] = glm::vec4(pc->portal->points[1 ^ pc->side].x, pc->portal->points[1 ^ pc->side].y,
                pc->portal->points[pc->side].x, pc->portal->points[pc->side].y);
            j += 1;
            if (j == 4) break;
        }
        ru2->numPortals = j;

        vkUnmapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[0].memory);

        i += 1;
    }
}

void TestPlayer::memMap(void* ptr)
{
    static b2Vec2 lastLinVel = b2Vec2(0.0, 0.0);
    static float aimedRatio = 0.5;
    static float bodyPos = 0.0;
    static float bodyVel = 0.0;

    TestPlayer* tp = (TestPlayer*)ptr;

    if (tp->won)
    {
        tp->winanim();
    }

    write0(tp);

    const float maxSpeed = 100.0f;
    PortalBody* pb0 = tp->wData->pbIndices[0];

    b2Body* body = pb0->body;
    b2Vec2 linVel = body->GetLinearVelocity();

    float desiredValue = 0.06 - std::min(std::max(linVel.x, -8.0f), 8.0f) * 0.01;
    desiredValue = std::max(std::min(desiredValue, 0.12f), 0.01f);

    if (pb0->isMirrored)
    {
        tp->renderData.headCircles[1] -= (tp->renderData.headCircles[1] - desiredValue) * Game::object->deltaTime * 6.0f;
        tp->renderData.headCircles[0] = 0.12 - tp->renderData.headCircles[1];
    }
    else
    {
        tp->renderData.headCircles[0] -= (tp->renderData.headCircles[0] - desiredValue) * Game::object->deltaTime * 6.0f;
        tp->renderData.headCircles[1] = 0.12 - tp->renderData.headCircles[0];
    }

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
        tp->rotSpeed = fmin(tp->rotSpeed, +len * b2_pi * 1.0);
        tp->rotSpeed = fmax(tp->rotSpeed, -len * b2_pi * 1.0);
    }

    if ((diff.Length() > 0.2 || linVel.Length() > 1.0) && tp->grounded)
    {
        aimedRatio = 0.5;
    }
    else
    {
        const float m = 0.1;
        aimedRatio = 0.25;
        float ang = tp->legAng;
        if (pb0->isMirrored) ang = b2_pi - ang;
        float roundedAngle = fmod(ang, 2.0 * b2_pi);
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

    tp->legAng += tp->rotSpeed * Game::object->deltaTime * (pb0->isMirrored ? -1.0f : 1.0f);
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

    tp->renderData.ld.legCircleSize = 0.025 * ratio01 + 0.055 * (1.0 - ratio01);
    tp->renderData.ld.circR = tp->renderData.legRatio * 0.5 * (1.0 - ratio01) + 10.0 * ratio01;
    tp->renderData.ld.legLen = tp->renderData.legRatio * 0.5;
    tp->renderData.ld.legOuterCircAng = tp->legAng;
    tp->renderData.ld.legAngle = tp->legAng;

    const float soundAngle = 0.67f;
    const float soundEndAngle = b2_pi - soundAngle;
    const float currentAngle = fmod(tp->legAng, b2_pi);

    if (currentAngle > soundAngle && currentAngle < soundEndAngle)
    {
        if (!tp->walkPlaying)
        {
            tp->walkSound = Sounder::object->playSound(tp->walkBuffer, 1.0f);
            tp->walkPlaying = true;
            b2Vec2 pos = body->GetPosition();
            Sounder::object->setSoundPosition(tp->walkSound, glm::vec3(pos.x, pos.y, 0.0f));
        }
    }
    else if (tp->walkPlaying)
    {
        tp->walkPlaying = false;
    }

    s_renderPlayerUbo* ru;
    vkMapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[1].memory, tp->playerShader->alignedSize2 * ThinDrawer::object->frameIndex,
        tp->playerShader->alignedSize2, 0, (void**)&ru);
    memcpy((void*)ru, &tp->renderData, sizeof(s_renderPlayerUbo));
    vkUnmapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[1].memory);
}

void TestPlayer::restartWorld()
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    health = 100;
    won = false;

    thinDrawer->wh->cam->pos = glm::vec2(initialPos.x, -initialPos.y - bodySize.y * 0.4f);
    thinDrawer->wh->cam->updateOrtho();

    renderData.topMargin = 0.0;
    renderData.headRatio = 0.25;
    renderData.bodyRatio = 0.4;
    renderData.legRatio = 0.25;
    renderData.margin1 = (1.0 - renderData.headRatio - renderData.bodyRatio - renderData.legRatio) * 0.5;
    renderData.margin2 = (1.0 - renderData.headRatio - renderData.bodyRatio - renderData.legRatio) * 0.5;
    renderData.lineSize = 0.035;
    renderData.headCircles = glm::vec2(0.07, 0.07);
    renderData.bodyCircles = 0.1;

    renderData.ld.legThick = 0.12;
    renderData.ld.circR = renderData.legRatio * 1000.5;
    renderData.ld.legLen = 0.25 * 0.5;
    renderData.ld.legOuterCircAng = 0.0;
    renderData.ld.legAngle = renderData.ld.legOuterCircAng;
    renderData.ld.legCircleSize = 0.025;
    renderData.size = glm::vec2(0.75, 1.5);

    for (auto iter = thinDrawer->memMapFuncs.begin(); iter != thinDrawer->memMapFuncs.end(); iter++)
    {
        if ((*iter).first == memMapDeathByElectrocution)
        {
            (*iter).first = zoomOut;
            break;
        }
    }

    getBodyOut();

    wData->pbIndices[0]->body->SetTransform(b2Vec2(initialPos.x, initialPos.y), 0.0f);
    wData->pbIndices[0]->body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
    gun->at(0)->body->SetTransform(b2Vec2(initialPos.x, initialPos.y), 0.0f);
    gun->at(0)->body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));

    if (portalA)
    {
        portalA->getPreDestructionFixture();
        pWorld->destroyPortal(portalA);
    }
    if (portalB)
    {
        portalB->getPreDestructionFixture();
        pWorld->destroyPortal(portalB);
    }

    portalA = nullptr; portalB = nullptr;

    for (DefaultObject* obj : DefaultObject::defaultObjects)
    {
        obj->recreate();
    }

    init();

    ForceField::destroyCList = {};

    UseButton::toDeath();
    ForceField::toDeath();
    HazardField::toDeath();
    Gel::toDeath();
    DownButton::toDeath();
    CubeButton::toDeath();
    CircleButton::toDeath();
    LaserEmitter::toDeath();
    GndLaserReceiver::toDeath();
    WallLaserReceiver::toDeath();
    Platform::toDeath();
    Dropper::toDeath();
}

void TestPlayer::winanim()
{
    double deltaTime = ThinDrawer::object->deltaTime;

    glm::vec2 camPos = ThinDrawer::object->wh->cam->pos;
    b2Vec2 headPosb = EndCapsule::object->pos;
    glm::vec2 headPos = glm::vec2(headPosb.x, -headPosb.y);
    glm::vec2 diff = headPos - camPos;
    float lenDiff = glm::length(diff);

    if (lenDiff < 0.05f)
    {
        ThinDrawer::object->wh->cam->zoom += 1.5f * deltaTime;
        ThinDrawer::object->wh->cam->zoom = std::min(ThinDrawer::object->wh->cam->zoom, 2.0f);

        if (pWorld->portals.size() == 2 && ThinDrawer::object->wh->cam->zoom > 1.9f)
        {
            Portal* p1 = pWorld->portals[0];

            b2Vec2 linVel = p1->body->GetLinearVelocity();

            b2Vec2 playerPos = TestPlayer::object->wData->pbIndices[0]->body->GetPosition();

            bool out = playerPos.x > 1000.0f && playerPos.y > 1000.0f && TestPlayer::object->wData->pbIndices.size() == 1;

            if (p1->pos.y < headPosb.y - 0.9f && linVel.y < -0.01f || out)
            {
                p1->body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));

                if (!game->pop.active && !Game::object->testCase->shouldQuit)
                {
                    s_menuButton initialButton = {
                    glm::vec4(0.0f, 0.0f, 300.0f, 300.0f), // posSize
                    glm::vec4(0.0f, 0.0f, 0.0f, 0.0), // clr
                    glm::vec4(5.0f, 5.5f, 0.1f, 0.1f) }; // data
                    game->pop.init(initialButton, "darkness", 0.5);

                    game->pop.endFunction = []
                    {
                        Game::object->singleTaskRegistry.push_back([]
                        {
                            Game::object->testCase->shouldQuit = true;
                        });
                    };
                }
            }
            else if (abs(linVel.y) < 0.01f)
            {
                p1->body->SetLinearVelocity(b2Vec2(0.0f, -0.5f));
            }
        }
    }
    else
    {
        diff /= (lenDiff + 0.001);

        glm::vec2 posAdd = (float)deltaTime * diff * lenDiff * 5.0f;
        camPos += posAdd;
    }

    ThinDrawer::object->wh->cam->pos = camPos;

    ThinDrawer::object->wh->cam->updateOrtho();
}

void TestPlayer::memMapDeathByElectrocution(void* ptr)
{
	double deltaTime = ThinDrawer::object->deltaTime;

    Game::object->dtMult = 0.0f;

    TestPlayer* tp = (TestPlayer*)ptr;

    tp->deathTimer += deltaTime;
    double time01 = glm::smoothstep(0.0, 0.5, tp->deathTimer);
    
    write0(tp);
    
    s_renderPlayerUbo* ru;
    vkMapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[1].memory,
        tp->playerShader->alignedSize2 * ThinDrawer::object->frameIndex, tp->playerShader->alignedSize2, 0, (void**)&ru);
    memcpy((void*)ru, &tp->renderData, sizeof(s_renderPlayerUbo));
    vkUnmapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[1].memory);

    glm::vec2 camPos = ThinDrawer::object->wh->cam->pos;
    b2Vec2 headPos1 = b2Vec2(0.0f, -tp->bodySize.y * 0.4f);
    headPos1 = PortalWorld::rotateVec(headPos1, -tp->wData->pbIndices[0]->body->GetAngle());
    headPos1 += b2Vec2(tp->wData->pbIndices[0]->body->GetPosition().x, -tp->wData->pbIndices[0]->body->GetPosition().y);
    glm::vec2 headPos = glm::vec2(headPos1.x, headPos1.y);
    glm::vec2 diff = headPos - camPos;
    float lenDiff = glm::length(diff);

    if (lenDiff < 0.05f)
    {
        ThinDrawer::object->wh->cam->zoom += 7.5f * deltaTime;

        if (!Game::object->pop.active)
        {
            s_menuButton initialButton = {
            glm::vec4(0.0f, 0.0f, 300.0f, 300.0f), // posSize
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0), // clr
            glm::vec4(5.0f, 5.5f, 0.1f, 0.1f) }; // data
            Game::object->pop.init(initialButton, "darkness", 1.0);

            Game::object->pop.endFunction = []
            {
                Game::object->singleTaskRegistry.push_back([]
                {
                    TestPlayer::object->restartWorld();

                    Game::object->pop.active = false;
                    Game::object->pop.closeOnEnd = true;
                    ThinDrawer::object->eraseFromRenderFunctions(ThinDrawer::object->lateDrawFuncs, &Game::object->pop);
                });
            };
        }
    }
    else
    {
        diff /= (lenDiff + 0.001);

        glm::vec2 posAdd = (float)deltaTime * diff * glm::min(lenDiff, 3.0f) * 15.0f;
        camPos += posAdd;
    }

    ThinDrawer::object->wh->cam->pos = camPos;

    ThinDrawer::object->wh->cam->updateOrtho();
}

void TestPlayer::zoomOut(void* ptr)
{
    TestPlayer* tp = (TestPlayer*)ptr;

    ThinDrawer::object->wh->cam->zoom -= ThinDrawer::object->deltaTime * 2.0f * ThinDrawer::object->wh->cam->zoom;

    write0(tp);

    s_renderPlayerUbo* ru;
    vkMapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[1].memory,
        tp->playerShader->alignedSize2 * ThinDrawer::object->frameIndex, tp->playerShader->alignedSize2, 0, (void**)&ru);
    memcpy((void*)ru, &tp->renderData, sizeof(s_renderPlayerUbo));
    vkUnmapMemory(tp->playerShader->logicalDevice, tp->playerShader->uniformBuffers[1].memory);

    if (ThinDrawer::object->wh->cam->zoom < 0.5f)
    {
        Game::object->dtMult = 1.0f;
        Game::object->isPaused = false;
        ThinDrawer::object->wh->cam->zoom = 0.5f;
        Game::object->singleTaskRegistry.push_back([tp]()
        {   
            for (auto iter = ThinDrawer::object->memMapFuncs.begin(); iter != ThinDrawer::object->memMapFuncs.end(); iter++)
            {
                if ((*iter).first == zoomOut)
                {
                    (*iter).first = memMap;
                    break;
                }
            }
        });
    }

    ThinDrawer::object->wh->cam->updateOrtho();
}

void TestPlayer::draw(VkCommandBuffer cmdBuff, void* ptr)
{
    TestPlayer* tp = (TestPlayer*)ptr;

    uint32_t imNum = ThinDrawer::object->frameIndex;
    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ThinDrawer::object->camOffset;
    uint32_t dynOff2 = tp->playerShader->alignedSize2 * imNum;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, tp->playerShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &tp->playerShader->buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        tp->playerShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    int i = 0;
    for (PortalBody* pb : tp->wData->pbIndices)
    {
        uint32_t dynOff1 = tp->playerShader->alignedSize * (i + imNum * 8);
        uint32_t dynOffsets[] = { dynOff1, dynOff2 };
        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            tp->playerShader->pipelineLayout, 1, 1, &tp->playerShader->descriptorSets[1], 2, dynOffsets);

        vkCmdDraw(cmdBuff, 6, 1, 0, 0);

        i++;
    }
}

void TestPlayer::drawGunEffect(VkCommandBuffer cmdBuff, void* ptr)
{
    TestPlayer* tp = (TestPlayer*)ptr;

    GunEffect* gunEffectShader = ThinDrawer::object->gunEffect;
	SplashShader* splashShader = ThinDrawer::object->splash;

    const VkDeviceSize offsets = 0;
    const uint32_t camOffset = ThinDrawer::object->camOffset;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, gunEffectShader->pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &tp->playerShader->buffers[0].buffer, &offsets);

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
        else if (tp->holdBody->bodyMaps.size() > 0)
        {
            pc = tp->holdBody->bodyMaps[0].connection;
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
        
        if (shootData->data.x < 0.9)
        {
            glm::vec2 shootDir = glm::vec2(shootData->positions.z - shootData->positions.x, shootData->positions.w - shootData->positions.y);
            shootData->positions.x += shootDir.x * Game::object->deltaTime * 10.0f;
            shootData->positions.y += shootDir.y * Game::object->deltaTime * 10.0f;

            shootDir = glm::normalize(shootDir);
            shootData->positions.z += shootDir.x * Game::object->deltaTime;
            shootData->positions.w += shootDir.y * Game::object->deltaTime;
        }
        
        vkCmdPushConstants(cmdBuff, gunEffectShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(s_gunData), (void*)shootData);
        vkCmdDraw(cmdBuff, 6, 1, 0, 0);
    }
    
    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, splashShader->pipeline);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        splashShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    for (int i = tp->endSplashes.size() - 1; i > -1; i--)
    {
        splashData* splsh = &tp->endSplashes.at(i);

        if (ThinDrawer::object->gameTime - splsh->data.y >= 1.0)
        {
            tp->endSplashes.erase(tp->endSplashes.begin() + i);
            continue;
        }

        vkCmdPushConstants(cmdBuff, splashShader->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(splashData), (void*)splsh);
        vkCmdDraw(cmdBuff, 6, 25, 0, 0);
    }
}

void TestPlayer::portalEffect(Portal* portal)
{
    s_edgeGlobalData* eGlobal = nullptr;
    if (portal->sideFixtures[0])
	{
		fixtureData* fData = (fixtureData*)portal->sideFixtures[0]->GetUserData().pointer;
        if (fData && fData->type & F_EDGE)
		    eGlobal = ((s_edgeLocalData*)fData->data)->globalData;
	}
    else if (portal->sideFixtures[1])
    {
        fixtureData* fData = (fixtureData*)portal->sideFixtures[1]->GetUserData().pointer;
        if (fData && fData->type & F_EDGE)
            eGlobal = ((s_edgeLocalData*)fData->data)->globalData;
    }

    if (eGlobal)
    {
        b2Vec2 diff = portal->pos - portal->body->GetWorldPoint(eGlobal->localPoints[0]);
        int index = (int)(b2Dot(diff, eGlobal->dir) * 10.0);
        int offset = eGlobal->renderOffset + portal->range[0];

        int memMapSize = portal->range[1] - portal->range[0] + 1;
        s_wallPerGelData* mapMem2;
        vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
            offset * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData) * memMapSize, 0, (void**)&mapMem2);

        for (int i = 0; i < memMapSize; i++)
        {
            s_unitWallData uData = eGlobal->unitWalls[portal->range[0] + i];
            if (uData.gelType & WHITE)
            {
                float dist = (float)abs(index - portal->range[0] - i);
                mapMem2[i].data.z = ThinDrawer::object->gameTime + dist * 0.0075 - 0.075;
                mapMem2[i].data.w = 0.3;
            }
        }

        vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);
    }
}

void TestPlayer::runEffect()
{
    fastRun = false;

    if (contactFixture)
    {
        s_edgeGlobalData* eGlobal = nullptr;
        fixtureData* fData = (fixtureData*)contactFixture->GetUserData().pointer;
        if (fData)
        {
            if (fData->type & (F_EDGE | F_LIGHT_BRIDGE))
            {
                s_edgeLocalData* eLocal = (s_edgeLocalData*)fData->data;
                eGlobal = eLocal->globalData;
            }
            else if (fData->type == F_PORTAL)
            {
                Portal* portal = (Portal*)fData->data;
                if (portal->sideFixtures[0])
				{
					fData = (fixtureData*)portal->sideFixtures[0]->GetUserData().pointer;
					if (fData && fData->type & F_EDGE)
						eGlobal = ((s_edgeLocalData*)fData->data)->globalData;
				}
				else if (portal->sideFixtures[1])
				{
					fData = (fixtureData*)portal->sideFixtures[1]->GetUserData().pointer;
					if (fData && fData->type & F_EDGE)
						eGlobal = ((s_edgeLocalData*)fData->data)->globalData;
				}
            }
        }

        if (eGlobal)
        {
            b2Vec2 v1;
            b2Vec2 dirVec;
            int extraOffset;

            if (fData && fData->type & F_LIGHT_BRIDGE)
            {
                b2EdgeShape* edge = (b2EdgeShape*)contactFixture->GetShape();
                dirVec = edge->m_vertex2 - edge->m_vertex1;
                dirVec.Normalize();
                v1 = edge->m_vertex1;
                s_edgeLocalData* eLocal = (s_edgeLocalData*)fData->data;
                extraOffset = eLocal->globalRange[0];
            }
            else
            {
                v1 = eGlobal->localPoints[0];
                extraOffset = 0;
                dirVec = eGlobal->dir;
            }

            b2Vec2 botPos = wData->pbIndices[0]->body->GetWorldPoint(b2Vec2(0.0, -bodySize.y * 0.5));
            b2Vec2 diff = botPos - contactBody->GetWorldPoint(v1);
            int index = (int)std::round(b2Dot(diff, dirVec) * 10.0) + extraOffset;
            if (index < 0 || index >= eGlobal->unitWalls.size())
			{
				return;
			}

            s_unitWallData uData = eGlobal->unitWalls.at(index);
			if (uData.gelType & ORANGE && uData.gelAmount > 0.5)
			{
                fastRun = true;

                int renderOffset = eGlobal->renderOffset + index;
				b2Vec2 vel = wData->pbIndices[0]->body->GetLinearVelocity();
                if (platformBody)
                {
                    vel -= platformBody->GetLinearVelocity();
                }

                if (renderOffset != lastOrangeGelSetOffset && abs(vel.x) > 3.0)
                {
                    lastOrangeGelSetOffset = renderOffset;
                    s_wallPerGelData* mapMem2;
                    vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
						renderOffset * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData), 0, (void**)&mapMem2);
                    mapMem2->data.z = ThinDrawer::object->gameTime - 0.03;
                    mapMem2->data.w = std::min(abs(vel.x) * 0.04, 0.3);
					vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);
                }
			}
        }
    }
}

void TestPlayer::jumpEffect()
{
    b2Body* body = wData->pbIndices[0]->body;

    if (!contactFixture || effectiveFunnel)
    {
        grounded = false;
        return;
    }

    bool jumped = false;

    static double jumpPressTime = 0.0;

    b2Vec2 lv = body->GetLinearVelocity();

    grounded = ((contactBody || abs(avgNormalX) < normXToWalk) && lv.y <= 5.0f) || (lv.y < 0.0f && onAirFor < 0.05f && onAirFor != 0.0f);

    float checkXSpeed = lv.x -= contactBody ? contactBody->GetLinearVelocity().x : 0.0f;
    
    bool noKeyJump = false;
    if (wh->keyData[Game::jumpKey] == 2)
    {
        jumpPressTime = gameTime;
    }
    else if (abs(checkXSpeed) > 6.0f && !wh->keyData[SDL_SCANCODE_LCTRL] && !wh->keyData[SDL_SCANCODE_RCTRL])
    {
        noKeyJump = Gel::bounceAt(wData->pbIndices[0]->body, contactFixture, contactPoint);
    }

    if (grounded && gameTime < jumpPressTime + 0.04 || noKeyJump)
    {
        jumped = true;
        bool extraJump = noKeyJump;

        if (!noKeyJump && !wh->keyData[SDL_SCANCODE_LCTRL] && !wh->keyData[SDL_SCANCODE_RCTRL])
        {
            extraJump = Gel::bounceAt(wData->pbIndices[0]->body, contactFixture, contactPoint);
        }

        lv.y = (6.0f + 6.0f * extraJump) * (float)wData->pbIndices.size();

        jumpPressTime = 0.0;

        ALuint source;
        if (!extraJump)
            source = Sounder::object->playSound(jumpBuffer, 1.0);
        else
            source = Sounder::object->playSound(blueJumpBuffer, 3.0);
        b2Vec2 pPos = body->GetPosition();
        Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));
    }

    lv.y = std::fmax(lv.y, -15.0);

    body->SetLinearVelocity(lv);
}

void TestPlayer::renderInit()
{
    ThinDrawer::object->memMapFuncs.push_back({ memMap, this });
    ThinDrawer::object->midDrawFuncs.push_back({ draw, this });

    ThinDrawer::object->midDrawFuncs.push_back({ drawGunEffect, this });

    renderData.edgeColour = glm::vec4(1.0, 1.0, 1.0, 1.0);
    renderData.bodyColour1 = glm::vec4(0.0, 0.0, 0.0, 1.0);
    renderData.bodyColour2 = glm::vec4(0.0, 0.0, 0.3, 1.0);

    renderData.topMargin = 0.0;
    renderData.headRatio = 0.25;
    renderData.bodyRatio = 0.4;
    renderData.legRatio = 0.25;
    renderData.margin1 = (1.0 - renderData.headRatio - renderData.bodyRatio - renderData.legRatio) * 0.5;
    renderData.margin2 = (1.0 - renderData.headRatio - renderData.bodyRatio - renderData.legRatio) * 0.5;
    renderData.lineSize = 0.035;
    renderData.headCircles = glm::vec2(0.07, 0.07);
    renderData.bodyCircles = 0.1;

    renderData.ld.legThick = 0.12;
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
    bodyDef.fixedRotation = false;
    bodyDef.linearDamping = 0.01f;
    bodyDef.bullet = true;
    bodyDef.allowSleep = false;

    b2Body* body = pWorld->CreateBody(&bodyDef);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(bodySize.x * 0.5 - 0.01, bodySize.y * 0.5 - bodySize.x * 0.5);

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
    pData.type = (g_DataType)(DEFAULT_OBJECT_O | PLAYER_O);
    pData.data = this;
    bData->extraData = &pData;

    wData = pb->worldIndex;

    gunSize = b2Vec2(0.3f, 0.8f);
    Gun* dGun = new Gun(pWorld, pos, gunSize);

    this->gun = &dGun->wData->pbIndices;

    b2RevoluteJointDef jDef;
    jDef.bodyA = wData->pbIndices[0]->body;
    jDef.bodyB = gun->at(0)->body;
    jDef.collideConnected = false;
    jDef.localAnchorA = b2Vec2(0.0f, +0.35f);
    jDef.localAnchorB = b2Vec2(0.0f, -0.35f);
    jDef.calculateAnchorHelpers();

    pWorld->CreateJoint(&jDef);
}

void TestPlayer::recreate(bool mirrored)
{

}

bool TestPlayer::enterForceField(ForceField* forceField)
{
    game->singleTaskRegistry.push_back(
        [this]()
        {
            if (portalA && portalB) getBodyOut();

            if (portalA || portalB)
            {
                ALuint source = Sounder::object->playSound(fizzleBuffer, 1.0);
                b2Vec2 bPos = wData->pbIndices[0]->body->GetPosition();
                Sounder::object->setSoundPosition(source, glm::vec3(bPos.x, bPos.y, 0.0));
            }

            if (portalA)
            {
                portalA->getPreDestructionFixture();
                pWorld->destroyPortal(portalA);
            }
            if (portalB)
            {
                portalB->getPreDestructionFixture();
                pWorld->destroyPortal(portalB);
            }
            portalA = nullptr; portalB = nullptr;
        }
    );

    return true;
}

void TestPlayer::beginContact(b2Contact* contact, b2Fixture* otherFix, b2Fixture* thisFix)
{
    fixtureData* fixData = (fixtureData*)otherFix->GetUserData().pointer;
    if (fixData)
    {
        switch (fixData->type)
        {
        case F_FORCE_FIELD:
        {
            ForceField* forceField = (ForceField*)fixData->data;
            if (forceField->enabled)
            {
                enterForceField((ForceField*)fixData->data);
            }
        }
        break;
        case F_HAZARD_FIELD:
            health -= 1000;
			healthCheck();
			break;
        case F_CATAPULT:
        {
            Catapult* catapult = (Catapult*)fixData->data;
            catapult->sendIt(this, wData->pbIndices[0]->body, (float)wData->pbIndices.size());

            gun->at(0)->body->SetLinearVelocity((float)gun->size() * wData->pbIndices[0]->body->GetLinearVelocity());
        }
        break;
        case F_LIGHT_BRIDGE:
        case F_EDGE:
        case F_PORTAL:
        {
            if (grounded || wh->keyData[SDL_SCANCODE_LCTRL] || wh->keyData[SDL_SCANCODE_RCTRL])
			{
                break;
			}

            b2Vec2 res = tryBounce(contact, thisFix, otherFix);
            if (res.x > -900.0f)
            {
                float spd = thisFix->GetBody()->GetLinearVelocity().Length() * 0.5f;
                ALuint source = Sounder::object->playSound(blueJumpBuffer, 3.0 + std::min(spd, 3.0f));
                Sounder::object->setSoundPosition(source, glm::vec3(contactPoint.x, contactPoint.y, 0.0));
            }
        }
        break;
        default:
            break;
        }
    }
}

void TestPlayer::endContact(b2Contact* contact, b2Fixture* otherFix)
{
    if (otherFix == contactFixture)
	{
		contactFixture = nullptr;
		contactBody = nullptr;
		platformBody = nullptr;
        l_platformBody = nullptr;
        l_contactBody = nullptr;
		leftCollisionType = b2_staticBody;
		rightCollisionType = b2_staticBody;
		numNormals = 0;
		avgNormalX = 0.0f;
	}
    else if (otherFix->GetBody() == l_platformBody)
    {
        l_platformBody = nullptr;
    }

    if (!contact->IsEnabled()) return;
}

void TestPlayer::winwin()
{
    if (won || game->isPaused) return;

    won = true;

    enterForceField(nullptr);

    game->singleTaskRegistry.push_back([this]
    {
        b2Vec2 topPos = EndCapsule::object->pos + b2Vec2(0.0f, 0.975f);
        b2BodyDef bDef;
        bDef.type = b2_kinematicBody;
        bDef.position = topPos;
        b2Body* body = pWorld->CreateBody(&bDef);

        Portal* p1 = pWorld->createPortal(topPos, b2Vec2(0.0f, -1.0f), 0.49f, body);
        Portal* p2 = pWorld->createPortal(b2Vec2(1453.0f, 1071.0f), b2Vec2(0.0f, -1.0f), 0.49f);

        p1->connect(p2);
    });

    gData.positions.x = -1881.0f;
    gData.positions.z = -1938.0f;
}

void TestPlayer::preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2)
{
    fixtureData* fData = (fixtureData*)fix2->GetUserData().pointer;

    if (fData && fData->type & F_END_CAPSULE)
    {
        EndCapsule* ec = (EndCapsule*)fData->data;
        float collFixSide = ((b2EdgeShape*)fix2->GetShape())->m_vertex1.x > 0.1f ? 1.0f : -1.0f;
        float posXDiff = thisBody->GetPosition().x - ec->pos.x;
        float res = posXDiff * collFixSide;
        float normAngle = PortalWorld::normalizeAngle(thisBody->GetAngle());

        if (res > 0.15f && !won)
        {
            contact->SetEnabled(false);
            return;
        }
        else if (abs(normAngle) < 0.2f)
        {
            winwin();
        }

        return;
    }

    if (holdBody)
    {
        bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
        if (bData && bData->type & B_PORTAL_BODY)
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

    PortalBody* contactingwData = (PortalBody*)((bodyData*)thisBody->GetUserData().pointer)->data;
    float tHold = 0.4f;
    b2Vec2 bodyPos = contactingwData->body->GetPosition();
    b2Vec2 botVector = b2Vec2(0.0f, -1.0f);
    botVector = pWorld->rotateVec(botVector, thisBody->GetAngle());

    float botY = bodyPos.y + (bodySize.y / 2.0f) * botVector.y;
    if (wManifold.points[0].y < botY + tHold && wManifold.points[0].y > botY - tHold)
    {
        b2Vec2 normal = pWorld->rotateVec(wManifold.normal, -contactingwData->body->GetAngle());
        if (abs(normal.x) <= normXToWalk)
        {
            contact->SetFriction(setFriction);
        }
    }

    contact->SetRestitution(0.0f);

    float sep = wManifold.separations[0];
    if (contact->GetManifold()->pointCount > 1) sep = (sep + wManifold.separations[1]) / 2.0f;

    if (health != 0 && fix2->GetBody()->GetType() != b2_dynamicBody && sep < -0.1f && contact->IsEnabled() && wData->pbIndices.size() == 1)
    {
        bool pass = false;
        if (fData && (fData->type & F_LIGHT_BRIDGE))
        {
            LightBridge* lb = (LightBridge*)fData->extraData;
            if (lb->newlyCreated)
            {
                pass = true;

                if (lb->seperations.find(contactingwData) == lb->seperations.end())
                {
                    lb->seperations.insert({ contactingwData, fix2->GetBody() });
                }
            }
        }

        if (fData && (fData->type & F_BRIDGE_CAP))
        {
            pass = true;
            contact->SetEnabled(false);
        }

        if (!pass)
        {
            health = 0;
            healthCheck();
        }
    }
}

void TestPlayer::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
    if (fix2->IsSensor())
    {
        return;
    }

    PortalBody* contactingwData = (PortalBody*)((bodyData*)contactingBody->GetUserData().pointer)->data;

    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);
    b2Vec2 normal = pWorld->rotateVec(wManifold.normal, -contactingwData->body->GetAngle());

    b2Vec2 cPos = wManifold.points[0];

    if (contact->GetManifold()->pointCount > 1)
    {
        cPos += wManifold.points[1];
        cPos *= 0.5f;
    }

    float tHold = 0.4f;
    b2Vec2 bodyPos = contactingwData->body->GetPosition();
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
            contactFixture = fix2;
            contactPoint = cPos;
            if (contactType == b2_dynamicBody || contactType == b2_kinematicBody)
            {
                if (fix2->GetDensity() == 0.0f &&
                    (contact->GetFixtureA()->GetBody() == wData->pbIndices[0]->body || contact->GetFixtureB()->GetBody() == wData->pbIndices[0]->body))
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

    glm::vec2 mp = wh->cam->getMouseCoords();
    glm::vec2 camDiff = glm::vec2(wh->mouseData[0] - wh->windowSizes.x * 0.5, wh->windowSizes.y * 0.5 - wh->mouseData[1]);

    b2Vec2 bp = wData->pbIndices[0]->body->GetPosition();
    glm::vec2 diff1 = (glm::vec2(bp.x, bp.y) - glm::vec2(wh->cam->pos.x, -wh->cam->pos.y)) * (float)game->deltaTime * 4.0f;
    glm::vec2 diff2 = (!holdBody ? (float)(game->deltaTime * 0.04 / wh->cam->zoom) * camDiff : glm::vec2(0.0f, 0.0f));
    diff1.x += diff2.x;
    diff1.y += diff2.y;
    wh->cam->pos += glm::vec2(diff1.x, -diff1.y);

    //wh->cam->pos = glm::vec2(bp.x, -bp.y);
    wh->cam->updateOrtho();
}

void TestPlayer::handleBodyRotation()
{
    if (effectiveFunnel) return;

    PortalBody* pb = wData->pbIndices[0];

    float angAddition = 0.0f;

    if (grounded)
    {
        angAddition = pb->body->GetLinearVelocity().x * 0.03f;
    }

    float ang = PortalWorld::normalizeAngle(pb->body->GetAngle() + angAddition);

    for (PortalBody* l_portalBody : wData->pbIndices)
    {
        l_portalBody->body->SetAngularVelocity(0.0f);
    }

    pb->body->SetAngularVelocity(-4.0f * ang);
}

void TestPlayer::handleGunRotation()
{
    float ang;
    float mult = 1.0f;
    PortalBody* pb = wData->pbIndices[0];
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

    if (!found && gun->size() > 0)
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
            for (bodyCollisionStatus stat : pb->bodyMaps)
            {
                if (stat.body == playerBody)
                {
                    pConnection = stat.connection;
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
        for (PortalBody* l_portalBody : wData->pbIndices)
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
    else if (fixData->data == (void*)oldPortal && oldPortal)
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

    int requiredRangeSize = std::ceil(size * 10.0f);

    b2EdgeShape* eShape = (b2EdgeShape*)checkFixture->GetShape();
    pos = checkFixture->GetBody()->GetLocalPoint(pos);
    float dist = (pos - eGlobalData->localPoints[0]).Length() * 10.0f;

    int startIndex = std::min(std::max((int)std::floor(dist), eLocalData->globalRange[0]), eLocalData->globalRange[1]);

    if (startIndex > -1 && startIndex < eGlobalData->unitWalls.size())
    {
        s_unitWallData ud = eGlobalData->unitWalls[startIndex];

        if (ud.wallType != PORTALLABLE && (ud.gelAmount <= 0.5 || ud.gelType != WHITE))
        {
            return nullptr;
        }
    }

    int checkRange[2] = { eLocalData->globalRange[0], eLocalData->globalRange[1] };

    if (eGlobalData->unitWalls.size() < requiredRangeSize) return nullptr;

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

    if (checkRange[1] - checkRange[0] + 1 < requiredRangeSize || startIndex < 0 || startIndex > eLocalData->globalRange[1]) return nullptr;

    int leftCount = 0;
    int rightCount = 0;
    int lastSum = 0;

    while (true)
    {
        if (startIndex + leftCount <= checkRange[1])
        {
            s_unitWallData& uData = eGlobalData->unitWalls[startIndex + leftCount];
            if (uData.wallType == PORTALLABLE || (uData.gelAmount > 0.5 && uData.gelType & WHITE))
            {
                leftCount++;

                if (leftCount + rightCount == requiredRangeSize)
                {
                    break;
                }
            }
        }

        if (startIndex - rightCount >= checkRange[0])
        {
            s_unitWallData& uData = eGlobalData->unitWalls[startIndex - rightCount];
            if (uData.wallType == PORTALLABLE || (uData.gelAmount > 0.5 && uData.gelType & WHITE))
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

void TestPlayer::gunToBody(b2Body* playerBody, b2Body* gunBody)
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

void TestPlayer::getBodyOut()
{
    PortalBody::getBodyOut(wData->pbIndices[0]->worldIndex);
    PortalBody::getBodyOut(gun->at(0)->worldIndex);

    b2Body* playerBody = wData->pbIndices[0]->body;
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
        gunToBody(playerBody, gunBody);
    }
}

PortalBody* TestPlayer::getOuterGun()
{
    PortalBody* portalGun = gun->at(0);
    for (PortalBody* gb : *gun)
    {
        bool ok = true;
        b2Vec2 p = gb->body->GetWorldPoint(b2Vec2(0.0, -0.35));
        for (bodyCollisionStatus bcs : gb->bodyMaps)
        {
            if (bcs.connection->portal1->getPointSide(p) != bcs.connection->side1)
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
    char control = canBlue | (canRed << 1);
    switch (control)
    {
    case 0:
        gData.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        break;
    case 1:
        gData.color = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
        break;
    case 2:
        gData.color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
        break;
    case 3:
        gData.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        break;
    default:
        break;
    }
}

void TestPlayer::portalClosingRitual()
{
    if (Game::object->pWorld->portals.size() < 2) return;

    getBodyOut();

    Portal* lastPortal = Game::object->pWorld->portals[Game::object->pWorld->portals.size() - 1];

    lastPortal->getPreDestructionFixture();
    Game::object->pWorld->destroyPortal(lastPortal);

    if (portalA == lastPortal) portalA = nullptr;
    else portalB = nullptr;
}

void TestPlayer::handlePortalCD(float dt)
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    const float portalSize = 2.6f;
    const float rayLength = 100.0f;

    PortalBody* portalGun = getOuterGun();
    b2Vec2 p0 = portalGun->body->GetWorldPoint(b2Vec2(0.0, -0.35));

    b2Vec2 vec;

    float vecAngle = b2_pi * 0.5f;
    vecAngle += portalGun->body->GetAngle();
    vec = b2Vec2(cos(vecAngle) * rayLength, sin(vecAngle) * rayLength);

    handleObjectHold(portalGun, p0, vec, dt);
    if (holdBody)
    {
        return;
    }

    PortalGunRayCast rayCaster = PortalGunRayCast(pWorld);

    b2Vec2 rayPoint1 = p0;
    b2Vec2 rayPoint2 = p0 + vec;

    //thinDrawer->registerDebugLine(glm::vec4(rayPoint1.x, rayPoint1.y, rayPoint2.x, rayPoint2.y), glm::vec4(1.0, 1.0, 1.0, 1.0), 0.03, 0.5);

    rayCaster.cast(rayPoint1, rayPoint2);

    if (rayCaster.found && !wh->keyData[SDL_SCANCODE_LCTRL])
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

        if (wh->mouseData[2] > 0 && gameTime - lastPortalCreationTime > 0.4f)
        {
            fixtureData* fData = (fixtureData*)rayCaster.edgeFix->GetUserData().pointer;
            if (fData && fData->type == F_FORCE_FIELD)
            {
                ForceField* ff = (ForceField*)fData->data;

                s_blueFieldUbo* blueFieldUbo;
                vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory,
                    ff->fieldRenderOffset * sizeof(s_blueFieldUbo), sizeof(s_blueFieldUbo), 0, (void**)&blueFieldUbo);

                blueFieldUbo->colour2 = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
                blueFieldUbo->shootData = glm::vec4(rayCaster.rayPoint.x, rayCaster.rayPoint.y, thinDrawer->gameTime, 0.0f);

                vkUnmapMemory(thinDrawer->blueField->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory);

                shootDatas.push_back({  { p0.x, p0.y, np1.x, np1.y },   { 0.0, 1.0f, 1.0f, 1.0f },
                                        { NAN, 0.0f, 0.0f, 0.0f },      { 0.5f, 0.0f, 0.0f, 0.0f } });

                ALuint source = Sounder::object->playSound(blueShootBufferJ, 1.0);
                b2Vec2 pPos = rayCaster.rayPoint;
                Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));

                canCreateBlue = nullptr;
            }

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

                portalEffect(portal);

                shootDatas.push_back({  { p0.x, p0.y, portal->pos.x, portal->pos.y },   { 0.0, 0.5f, 1.0f, 1.0f },
                                        { NAN, 0.0f, 0.0f, 0.0f },                      { 1.0f, 0.0f, 0.0f, 0.0f } });

                ALuint source = Sounder::object->playSound(blueShootBuffer, 1.0);
                b2Vec2 pPos = rayCaster.rayPoint;
                Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));

                portal->reviveAround();
                portalA = portal;

                auto iter = thinDrawer->renderPortalData.find(portal);
                iter->second.portalPsh.color = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);

                if (portalB)
                {
                    portalA->connect(portalB, true);
                    portalB->reviveAround();

                    auto iter2 = thinDrawer->renderPortalData.find(portalB);

                    iter->second.particlePsh.color = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
                    iter->second.particlePsh.data0.z = thinDrawer->gameTime;
                    iter->second.particlePsh.data1.z = thinDrawer->gameTime;

                    iter2->second.particlePsh.color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
                    iter2->second.particlePsh.data0.z = thinDrawer->gameTime;
                    iter2->second.particlePsh.data1.z = thinDrawer->gameTime;
                }
            }
            else if (fData && fData->type != F_FORCE_FIELD)
            {
                sData.color = glm::vec4(0.0f, 0.5f, 1.0f, 0.5f);
                sData.posSpeed.x = np1.x; sData.posSpeed.y = np1.y;
                sData.posSpeed.z = (float)rand() / RAND_MAX;
                sData.data.y = thinDrawer->gameTime;
                sData.data.z = 2.0f;
                sData.data.w = atan2(rayCaster.rayNormal.y, rayCaster.rayNormal.x) - 1.0f;

                endSplashes.push_back(sData);

                shootDatas.push_back({ { p0.x, p0.y, np1.x, np1.y },   { 0.0, 1.0f, 1.0f, 1.0f },
                                        { NAN, 0.0f, 0.0f, 0.0f },      { 0.5f, 0.0f, 0.0f, 0.0f } });

                ALuint source = Sounder::object->playSound(blueShootBufferJ, 1.0);
                b2Vec2 pPos = rayCaster.rayPoint;
                Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));
            }

            lastPortalCreationTime = gameTime;
        }
        else if (wh->mouseData[4] > 0 && gameTime - lastPortalCreationTime > 0.4f)
        {
            fixtureData* fData = (fixtureData*)rayCaster.edgeFix->GetUserData().pointer;
            if (fData && fData->type == F_FORCE_FIELD)
            {
                ForceField* ff = (ForceField*)fData->data;

                s_blueFieldUbo* blueFieldUbo;
                vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory,
                    ff->fieldRenderOffset * sizeof(s_blueFieldUbo), sizeof(s_blueFieldUbo), 0, (void**)&blueFieldUbo);

                blueFieldUbo->colour2 = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                blueFieldUbo->shootData = glm::vec4(rayCaster.rayPoint.x, rayCaster.rayPoint.y, thinDrawer->gameTime, 0.0f);

                vkUnmapMemory(thinDrawer->blueField->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory);

                shootDatas.push_back({  { p0.x, p0.y, np1.x, np1.y },   { 1.0, 0.5f, 0.0f, 1.0f },
                                        { NAN, 0.0f, 0.0f, 0.0f },      { 0.5f, 0.0f, 0.0f, 0.0f } });

                ALuint source = Sounder::object->playSound(orangeShootBufferJ, 1.0);
                b2Vec2 pPos = rayCaster.rayPoint;
                Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));

                canCreateRed = nullptr;
            }

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

                portalEffect(portal);

                shootDatas.push_back({  { p0.x, p0.y, portal->pos.x, portal->pos.y },   { 1.0, 0.5f, 0.0f, 1.0f },
                                        { NAN, 0.0f, 0.0f, 0.0f },                      { 1.0f, 0.0f, 0.0f, 0.0f } });

                ALuint source = Sounder::object->playSound(orangeShootBuffer, 1.0);
                b2Vec2 pPos = rayCaster.rayPoint;
                Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));

                portal->reviveAround();
                portalB = portal;

                auto iter = thinDrawer->renderPortalData.find(portal);
                iter->second.portalPsh.color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);

                if (portalA)
                {
                    portalB->connect(portalA);
                    portalA->reviveAround();

                    auto iter2 = thinDrawer->renderPortalData.find(portalA);

                    iter->second.particlePsh.color = glm::vec4(1.0f, 0.5f, 0.0f, -1.0f);
                    iter->second.particlePsh.data0.z = thinDrawer->gameTime;
                    iter->second.particlePsh.data1.z = thinDrawer->gameTime;

                    iter2->second.particlePsh.color = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
                    iter2->second.particlePsh.data0.z = thinDrawer->gameTime;
                    iter2->second.particlePsh.data1.z = thinDrawer->gameTime;
                }
            }
            else if (fData && fData->type != F_FORCE_FIELD)
            {
                sData.color = glm::vec4(1.0f, 0.5f, 0.0f, 0.5f);
                sData.posSpeed.x = np1.x; sData.posSpeed.y = np1.y;
                sData.posSpeed.z = (float)rand() / RAND_MAX;
                sData.data.y = thinDrawer->gameTime;
                sData.data.z = 2.0f;
                sData.data.w = atan2(rayCaster.rayNormal.y, rayCaster.rayNormal.x) - 1.0f;

                endSplashes.push_back(sData);

                shootDatas.push_back({ { p0.x, p0.y, np1.x, np1.y },   { 1.0, 0.5f, 0.0f, 1.0f },
                                        { NAN, 0.0f, 0.0f, 0.0f },      { 0.5f, 0.0f, 0.0f, 0.0f } });

                ALuint source = Sounder::object->playSound(orangeShootBufferJ, 1.0);
                b2Vec2 pPos = rayCaster.rayPoint;
                Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));
            }

            lastPortalCreationTime = gameTime;
        }
    }
    else
    {
        gData.positions.x = -1881.0f;
        gData.positions.z = -1938.0f;
    }

    if (!portalA || !portalB)
    {
        return;
    }

    float lenVH = abs(portalA->pos.x - portalB->pos.x) + abs(portalA->pos.y - portalB->pos.y);
    b2Vec2 posDiff = portalB->pos - portalA->pos;
    float dotVal = b2Dot(posDiff, portalA->dir);
    bool myParallel = dotVal <= 0.0f && dotVal > -0.2f;
    if (lenVH < 2.599f && myParallel)
    {
        portalClosingRitual();
    }
    else if ((portalA->body->GetType() != b2_staticBody || portalB->body->GetType() != b2_staticBody) &&
            abs(b2Dot(portalA->dir, portalB->dir)) < 0.1f)
    {
        Portal* hPortal, *vPortal;
        if (abs(portalA->points[0].x - portalA->points[1].x) > 0.1f)
        {
            hPortal = portalA;
            vPortal = portalB;
        }
        else
        {
            hPortal = portalB;
            vPortal = portalA;
        }
        
        float minX = std::min(hPortal->points[0].x, hPortal->points[1].x);
        float maxX = std::max(hPortal->points[0].x, hPortal->points[1].x);

        float minY = std::min(vPortal->points[0].y, vPortal->points[1].y);
        float maxY = std::max(vPortal->points[0].y, vPortal->points[1].y);

        float xDiff = vPortal->points[0].x - minX;
        float yDiff = hPortal->points[0].y - minY;

        if ((xDiff > 0.01f && xDiff < 2.599f) && (yDiff > -0.01f && yDiff < 2.601f))
		{
            portalClosingRitual();
		}
        else if ((yDiff > 0.01f && yDiff < 2.599f) && (xDiff > -0.01f && xDiff < 2.601f))
        {
            portalClosingRitual();
        }
    }
}

void TestPlayer::gunDestrCallback(PortalBody* gb, void* ptr)
{
    TestPlayer* tp = (TestPlayer*)ptr;

    if (gb != tp->holdGun || gb->bodyMaps.size() == 0)
    {
        return;
    }

    bodyCollisionStatus bcs = gb->bodyMaps[0];
    tp->holdGun = bcs.body;

    if (tp->holdConnection == nullptr)
    {
        tp->holdConnection = bcs.connection;
    }
    else if (tp->holdConnection->portal1 == bcs.connection->portal1 || tp->holdConnection->portal2 == bcs.connection->portal1)
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

    if (pb != tp->holdBody || pb->bodyMaps.size() == 0)
    {
        return;
    }

    bodyCollisionStatus bcs = pb->bodyMaps.at(0);
    tp->holdBody = bcs.body;
    if (bcs.connection->portal1 == tp->destrPortal || bcs.connection->portal2 == tp->destrPortal)
    {
        tp->shouldRelease = true;
        return;
    }

    if (tp->holdConnection == nullptr)
    {
        tp->holdConnection = bcs.connection;
    }
    else if (tp->holdConnection->portal1 == bcs.connection->portal1 || tp->holdConnection->portal2 == bcs.connection->portal1)
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
    pb->worldIndex->gravityScale = 0.0f;
    holdBody = pb;
    holdGun = gunBody;
    lastHoldPos = pb->body->GetPosition();

    holdBody->worldIndex->destroyCallbacks.push_back(std::make_pair(objectDestrCallback, (void*)this));
    holdGun->worldIndex->destroyCallbacks.push_back(std::make_pair(gunDestrCallback, (void*)this));

    shouldRelease = false;

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
    
    bodyData* bData = (bodyData*)holdBody->body->GetUserData().pointer;
    g_PhysicsData* pData = (g_PhysicsData*)bData->extraData;
    DefaultObject* obj = (DefaultObject*)pData->data;
    if (!obj->effectiveFunnel) indx->gravityScale = 1.0f;
    
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
            edge->contact->SetRestitution(holdBody->worldIndex->baseRestitutions[0]);
            if (obj->renderGroup->gelIndex == 1)
            {
                edge->contact->SetFriction(0.1f);
            }
            else
            {
				edge->contact->SetFriction(holdBody->worldIndex->baseFrictions[0]);
            }

            b2Contact* contact = edge->contact;
            if (contact->IsEnabled() && contact->IsTouching())
            {
                b2Fixture* fixA = contact->GetFixtureA();
                b2Fixture* fixB = contact->GetFixtureB();

                if (!fixA->IsSensor() && !fixB->IsSensor()) continue;

                fixtureData* fData = (fixtureData*)fixA->GetUserData().pointer;
                if (fData && fData->type == F_CATAPULT)
                {
                    Catapult* cat = (Catapult*)fData->data;
                    cat->sendIt(obj, pbA->body, holdBody->worldIndex->pbIndices.size());
                }

                fData = (fixtureData*)fixB->GetUserData().pointer;
                if (fData && fData->type == F_CATAPULT)
                {
                    Catapult* cat = (Catapult*)fData->data;
                    cat->sendIt(obj, pbA->body, holdBody->worldIndex->pbIndices.size());
                }
            }
        }

        int i = 0;
        for (b2Fixture* fix : pbA->fixtures)
        {
            fix->SetRestitution(holdBody->worldIndex->baseRestitutions[i]);

            if (obj->renderGroup->gelIndex == 1)
            {
                fix->SetFriction(0.1f);
            }
            else
            {
                fix->SetFriction(holdBody->worldIndex->baseFrictions[i++]);
            }
        }
    }

    holdBody = nullptr;
    holdGun = nullptr;
    holdConnection = nullptr;
    releaseRequestTime = -1.0f;
    holdERequestTime = -1.0f;
}

void TestPlayer::handleObjectHold(PortalBody* gunBody, b2Vec2 pos, b2Vec2 dir, float dt)
{
    object->destrPortal = nullptr;

    if (holdERequestTime < 0.0f && wh->keyData[Game::holdKey] == 2 && gameTime - lastBodyHoldTime > 0.3)
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
            pWorld->rayHandler->sendRay(pos, dir, 1.5f, wData);
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
                    if (pData && pData->type & PICKABLES)
                    {
                        PortalBody* pb = (PortalBody*)bData->data;
                        if (pb->worldIndex->destrTime < -10.0f)
                        {
                            holdB(pb, gunBody);
                            
                            if (pWorld->rayHandler->rayResult.size() == 2 && pWorld->rayHandler->rayResult[0].portal)
                            {
                                portalConnection* revConnection = pWorld->rayHandler->rayResult[0].portal->connections[0][0];
                                holdConnection = revConnection->portal2->connections[0][0];
                            }
                        }
                    }
                }

                fixtureData* fData = (fixtureData*)pWorld->rayHandler->closestFixture->GetUserData().pointer;
                if (fData && fData->type == F_USE_BUTTON)
                {
                    UseButton* ub = (UseButton*)fData->data;
                    ub->enable();
                    holdERequestTime = -1.0f;
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
                if (gunBody->bodyMaps.size() == 0)
                {
                    releaseB();
                    return;
                }

                portalConnection* newConnection = gunBody->bodyMaps[0].connection;
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
                pWorld->rayHandler->sendRay(pos, newDir, 1.5f, wData);
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

            float angAdd = 0.0f;
            if (wh->mouseData[2]) angAdd = dt * 15.0f;
            else if (wh->mouseData[4]) angAdd = -dt * 15.0f;

            float rotRatio = 1.0f - dt * 15.0f;
            holdBody->body->SetAngularVelocity(std::max(std::min(holdBody->body->GetAngularVelocity() * rotRatio + angAdd, 2.0f), -2.0f));
        }

        lastHoldPos = holdBody->body->GetPosition();
    }
}

void TestPlayer::healthCheck()
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    if (health < 1 && !won)
    {
        for (auto iter = thinDrawer->memMapFuncs.begin(); iter != thinDrawer->memMapFuncs.end(); iter++)
		{
			if ((*iter).first == memMap)
			{
                (*iter).first = memMapDeathByElectrocution;
                deathTimer = 0.0;
				break;
			}
		}
    }
}

void TestPlayer::updateReset()
{
    l_contactBody = contactBody;
    l_platformBody = platformBody;

    avgNormalX = 0.0;
    numNormals = 0;
    contactBody = nullptr;
    contactFixture = nullptr;
    platformBody = nullptr;
    right = false;
    left = false;
    contactType = (b2BodyType)INT_MAX; // NULL
    rightCollisionType = b2_dynamicBody;
    leftCollisionType = b2_dynamicBody;
    lastBody = wData->pbIndices[0];
    holdColliding = false;
}

void TestPlayer::update(float dt, int totalIter)
{
    static float lastMaxSpeed = 0.0f;

    gameTime = Game::object->gameTime;
    
    runEffect();

    const float maxSpeed = lastMaxSpeed + (4.0f + fastRun * 6.0f - lastMaxSpeed) * dt * (1.0f + lastMaxSpeed * 0.25);
    lastMaxSpeed = maxSpeed;

    float speedOffset = 0.0f;
    avgNormalX /= numNormals;
    setFriction = 0.0;
    PortalBody* pb = wData->pbIndices[0];

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

    if (wh->keyData[Game::rightKey] && wh->keyData[Game::leftKey])
    {
        if (lastDoubleKey == -1)
        {
            right = true;
        }
        else if (lastDoubleKey == Game::leftKey)
    {
            right = true;
        }
        else if (lastDoubleKey == Game::rightKey)
    {
            left = true;
        }
    }
    else if (wh->keyData[Game::rightKey])
    {
        right = true;
    }
    else if (wh->keyData[Game::leftKey])
    {
        left = true;
    }

    if (wData->pbIndices.size() > 1)
    {
        PortalBody* b1 = wData->pbIndices[0];
        PortalBody* b2 = wData->pbIndices[1];
        fixtureData* fixData1 = (fixtureData*)b1->body->GetFixtureList()->GetUserData().pointer;
        PBFixtureData* pbfData1 = (PBFixtureData*)fixData1->data;
        fixtureData* fixData2 = (fixtureData*)b2->body->GetFixtureList()->GetUserData().pointer;
        PBFixtureData* pbfData2 = (PBFixtureData*)fixData2->data;

        if (pbfData1->pCollisions.size() > 0 && pbfData2->pCollisions.size() > 0)
        {
            portalCollision pc1 = pbfData1->pCollisions.at(0);
            portalCollision pc2 = pbfData2->pCollisions.at(0);

            lastDirs[0] = pc1.side ? -pc1.portal->dir : pc1.portal->dir;
            lastDirs[1] = pc2.side ? -pc2.portal->dir : pc2.portal->dir;
        }
    }

    if (!(wh->keyData[Game::leftKey] && wh->keyData[Game::rightKey]))
    {
        if (wh->keyData[Game::leftKey])
        {
            lastDoubleKey = Game::leftKey;
        }
        else if (wh->keyData[Game::rightKey])
        {
            lastDoubleKey = Game::rightKey;
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
            if (keyBeforeSwitch == Game::rightKey)
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
            if (keyBeforeSwitch == Game::leftKey)
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
        float diff = platformBody->GetLinearVelocity().x - lv.x;
        lv.x += diff * dt * 15.0f * wData->pbIndices.size();
        setFriction = 2.0;
    }

    if (!effectiveFunnel && (left || right))
    {
        const float groundMult = 40.0f;
        const float airMult = 10.0f;
        const float fwdGrndSlw = 5.0f;
        if (right)
        {
            float addition;
            if (contactBody != nullptr)
            {
                addition = groundMult * dt * wData->pbIndices.size() * mult;
            }
            else if (rightCollisionType == b2_dynamicBody)
            {
                addition = airMult * dt * wData->pbIndices.size() * mult;
                /*if (lv.x < -maxSpeed * 2.0)
                {
                    addition = 0.0;
                }*/
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
                            lv.x *= fmax(0.0f, 1.0f - dt * (float)wData->pbIndices.size() * fwdGrndSlw);
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
                        lv.x *= fmax(0.0f, 1.0f - dt * (float)wData->pbIndices.size() * 5.0f);
                        lv.x = lv.x - addition;
                    }
                    else
                    {
                        if (contactBody != nullptr)
                        {
                            if (lv.x - addition < -maxSpeed + speedOffset)
                            {
                                lv.x *= fmax(0.0f, 1.0f - dt * (float)wData->pbIndices.size() * fwdGrndSlw);
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

            lastKey = Game::rightKey;
        }
        if (left)
        {
            float subtraction;
            if (contactBody != nullptr)
            {
                subtraction = groundMult * dt * wData->pbIndices.size() * mult;
            }
            else if (leftCollisionType == b2_dynamicBody)
            {
                subtraction = airMult * dt * wData->pbIndices.size() * mult;
                /*if (lv.x > maxSpeed * 2.0)
                {
                    subtraction = 0.0;
                }*/
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
                        lv.x *= fmax(0.0f, 1.0f - dt * (float)wData->pbIndices.size() * 5.0f);
                        lv.x = lv.x + subtraction;
                    }
                    else
                    {
                        if (contactBody != nullptr && mult < 0.0f)
                        {
                            if (lv.x + subtraction > maxSpeed + speedOffset)
                            {
                                lv.x *= fmax(0.0f, 1.0f - dt * (float)wData->pbIndices.size() * fwdGrndSlw);
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
                            lv.x *= fmax(0.0f, 1.0f - dt * (float)wData->pbIndices.size() * fwdGrndSlw);
                        }
                        else
                        {
                            lv.x = -maxSpeed + speedOffset;
                        }
                    }

                    lv.x = fmin(-maxSpeed + speedOffset, lv.x + subtraction);
                }
            }

            lastKey = Game::leftKey;
        }
    }
    else if (!contactBody)
    {
        setFriction = 0.0;
    }
    else if (contactBody && contactBody->GetMass() != 0.0f && onAirFor == 0.0f)
    {
        // kinematic body
        setFriction = 2.0;
    }
    else if (contactBody && contactType != b2_dynamicBody && onAirFor == 0.0f)
    {
        setFriction = 2.0;
    }
    else
    {
        setFriction = 2.0;
    }

    if (!contactBody) onAirFor += dt;
    else onAirFor = 0.0f;

    jumpEffect();

    if (effectiveFunnel)
    {
        b2Vec2 rightDir;
        if (inPortalFunel) rightDir = effectiveFunnel->funnelDir2;
        else rightDir = effectiveFunnel->funnelDir;
        rightDir = b2Vec2(abs(rightDir.y), abs(rightDir.x));

        b2Vec2 speedAdd = b2Vec2(0.0f, 0.0f);

        speedAdd.y += wh->keyData[Game::jumpKey] ? 0.01f : 0.0f;
        speedAdd.x += wh->keyData[Game::leftKey] ? -0.03f : 0.0f;
        speedAdd.y += wh->keyData[Game::downKey] ? -0.03f : 0.0f;
        speedAdd.x += wh->keyData[Game::rightKey] ? 0.03f : 0.0f;

        speedAdd = b2Vec2(rightDir.x * speedAdd.x, rightDir.y * speedAdd.y);

        lv += speedAdd;
        pb->body->SetLinearVelocity(lv);
    }
    else
    {
        pb->body->SetLinearVelocity(b2Vec2(lv.x, pb->body->GetLinearVelocity().y));
    }

    handleBodyRotation();
    
    handleGunRotation();

    if (!won)
    {
        handlePortalCD(dt);
    }

    if (health <= 0)
    {
        healthCheck();
    }

    updateReset();
}