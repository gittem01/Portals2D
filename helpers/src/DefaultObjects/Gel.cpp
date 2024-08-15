#include <DefaultObjects/DefaultCube.h>
#include <DefaultObjects/Gel.h>
#include <ThinDrawer.h>
#include <Game.h>
#include <Portal.h>

std::vector<Gel*> Gel::gels(MAX_GEL_COUNT);
std::vector<splashData> Gel::splashEffects(4096);
int Gel::gelCount = 0;
int Gel::splashCount = 0;

Gel::Gel(PortalWorld* portalWorld, glm::vec2 pos, glm::vec2 velocity, float radius, int gelType) : DefaultObject(portalWorld)
{
    this->pWorld = portalWorld;
    ALuint onHitSoundBuffer = Sounder::object->loadSound("redo.wav");
    onHitSoundBuffers.push_back(onHitSoundBuffer);
    
    b2BodyDef bDef;
    bDef.type = b2_dynamicBody;
    //bDef.type = b2_kinematicBody;
    bDef.position.Set(pos.x, pos.y);
    bDef.linearDamping = 0.0;

    b2CircleShape cShape;
    cShape.m_radius = radius;

    b2Body* body = pWorld->CreateBody(&bDef);
    body->SetLinearVelocity(b2Vec2(velocity.x, velocity.y));

    b2Fixture* fix = body->CreateFixture(&cShape, 0.1f);
    fix->SetSensor(true);

    PortalBody* portalBody = portalWorld->createPortalBody(body);
    *portalBody->gravityScale = 0.5;

    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
    physicsData->type = (g_DataType)(DEFAULT_OBJECT_O | GEL_O);
    physicsData->data = this;
    bData->extraData = physicsData;

    this->pBody = &portalBody->worldIndex->pbIndices;

    // too slow
    // must be done once in the game initialisation but it is not possible for now
    // ThinDrawer::object->debug_addPortalBody(portalBody);
}

void Gel::staticInit()
{
    gelCount = 0;
    splashCount = 0;

    ThinDrawer::object->midDrawFuncs.push_back({ &Gel::render, nullptr });
}

float handleGelPut(s_unitWallData* wallData, int gelType, float amnt)
{
    float val = std::max(pow(amnt, 0.5f) * 0.2f, 0.05f);
    if ((wallData->gelType & ((2 << gelType) | NONE | CLEAR) || wallData->gelAmount < 0.25) && gelType != 3)
    {
        wallData->gelAmount = std::min(wallData->gelAmount + val, 0.8f);
        wallData->gelType = (GEL_TYPE)(2 << gelType);
    }
	else
	{
        wallData->gelAmount = std::max(wallData->gelAmount - val, 0.0f);
	}

    return val;
}

int gelTypeToIndex(GEL_TYPE gelType)
{
    switch (gelType)
    {
    case BLUE:
		return 0;
    case ORANGE:
        return 1;
    case WHITE:
        return 2;
    case CLEAR:
        return 3;
    default:
		return -1;
    }
}

void Gel::getRangeData(b2Vec2 thisPos, b2Fixture* fix, int range[2], s_edgeGlobalData** data)
{
    fixtureData* fData = (fixtureData*)fix->GetUserData().pointer;
    if (fData && fData->type & F_PORTAL)
    {
        Portal* p = (Portal*)fData->data;
        if (p->getPointSide(thisPos) != 0)
        {
            *data = nullptr;
            return;
        }

        if (p->connections->size() == 0)
        {
            range[0] = 0;
            if (p->sideFixtures[0] && (fData = (fixtureData*)p->sideFixtures[0]->GetUserData().pointer) && fData->type & F_EDGE)
            {
                s_edgeLocalData* eLocal = (s_edgeLocalData*)fData->data;
                *data = eLocal->globalData;
                range[1] = eLocal->globalData->unitWalls.size() - 1;
            }
            else if (p->sideFixtures[1] && (fData = (fixtureData*)p->sideFixtures[1]->GetUserData().pointer) && fData->type & F_EDGE)
            {
                s_edgeLocalData* eLocal = (s_edgeLocalData*)fData->data;
                *data = eLocal->globalData;
                range[1] = eLocal->globalData->unitWalls.size() - 1;
            }
            else
			{
				*data = nullptr;
			}

            return;
        }
        else if (fix == p->yFix[0])
        {
            fix = p->sideFixtures[0];
        }
        else if (fix == p->yFix[1])
        {
            fix = p->sideFixtures[1];
        }
        else
        {
            *data = nullptr;
            return;
        }
    }

    fData = (fixtureData*)fix->GetUserData().pointer;
    if (fData && fData->type & F_EDGE)
    {
        s_edgeLocalData* eLocal = (s_edgeLocalData*)fData->data;
        *data = eLocal->globalData;
        range[0] = eLocal->globalRange[0];
        range[1] = eLocal->globalRange[1];
    }
    else
    {
        *data = nullptr;
    }
}

void Gel::splash(b2Contact* contact, b2Fixture* otherFix)
{
    if (gelIndex == -1) return;

    ALuint source = Sounder::object->playSound(onHitSoundBuffers[0], 0.5);
    Sounder::object->setSoundPosition(source, glm::vec3(pBody->at(0)->body->GetPosition().x, pBody->at(0)->body->GetPosition().y, 0.0));

    int range[2];
    s_edgeGlobalData* eGlobalData;
    getRangeData(contact->GetFixtureB()->GetBody()->GetPosition(), otherFix, range, &eGlobalData);

    if (eGlobalData)
    {        
        b2Vec2 distVec = contact->GetFixtureB()->GetBody()->GetPosition() - otherFix->GetBody()->GetWorldPoint(eGlobalData->localPoints[1]);
        float dist = std::max(b2Dot(distVec, -eGlobalData->dir), 0.0f);
        int off = std::min((int)std::round(dist * 10.0f), (int)eGlobalData->unitWalls.size() - 1);
        
        float amnt = gelRadius * 5.0;

        int startPos = eGlobalData->unitWalls.size() - off - 1;
        if (eGlobalData->unitWalls[startPos].wallType & (PORTALLABLE | NOT_PORTALLABLE))
            amnt -= handleGelPut(&eGlobalData->unitWalls[startPos], gelType, amnt);
        else
            goto out;

        int rightPos = startPos;
        int leftPos = startPos;
        for (int i = 1; ; i++)
        {
            float decrease = 0.0f;
            if (rightPos >= range[0] && (eGlobalData->unitWalls[rightPos].wallType & (PORTALLABLE | NOT_PORTALLABLE)))
            {
                decrease += handleGelPut(&eGlobalData->unitWalls[rightPos], gelType, amnt);
                rightPos = startPos - i;
            }
            if (leftPos <= range[1] && (eGlobalData->unitWalls[leftPos].wallType & (PORTALLABLE | NOT_PORTALLABLE)))
			{
				decrease += handleGelPut(&eGlobalData->unitWalls[leftPos], gelType, amnt);
                leftPos = startPos + i;
			}

            amnt -= decrease;
			if (amnt <= 0.0f || decrease == 0.0f) break;
        }

        rightPos = std::max(rightPos, range[0]);
        leftPos = std::min(leftPos, range[1]);

        int mapSize = leftPos - rightPos + 1;
        s_wallPerGelData* mapMem2;
        vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
            (eGlobalData->renderOfsset + eGlobalData->unitWalls.size() - leftPos - 1) * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData) * mapSize, 0, (void**)&mapMem2);

        for (int i = 0; i < mapSize; i++)
        {
            s_unitWallData wData = eGlobalData->unitWalls[leftPos - i];
            mapMem2[i].data.x = gelTypeToIndex(wData.gelType);
            mapMem2[i].data.y = wData.gelAmount;
        }

        vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);
    }

out:
    b2Vec2 pos = pBody->at(0)->body->GetPosition();

    splashData sData;
    sData.posSpeed.x = pos.x;
    sData.posSpeed.y = pos.y;
    sData.data.x = 0.075;
    sData.data.y = Game::object->gameTime;
    sData.data.z = b2_pi * 2.0;
    sData.data.w = 0.0;

    sData.color = gelColor;
    sData.color.w = 0.4;

    sData.data2.z = 3.0;
    sData.data2.x = 1.0;
    sData.data2.y = 5.0;
    sData.data2.w = 0.5;

    sData.data3.x = 0.0;
    sData.data3.y = 0.8;
    sData.data3.z = 100000.0;
    sData.data3.w = 0.5;

    splashEffects[splashCount++] = sData;

    Gel* lastGel = gels[--gelCount];
    lastGel->gelIndex = gelIndex;
    gels[gelIndex] = lastGel;
    gelIndex = -1;

    for (PortalBody* pb : *pBody)
    {
        pWorld->destroyBodies.insert(pb);
    }
}

void Gel::generateGel(glm::vec2 pos, glm::vec2 velocity, float radius, int colorIndex)
{
    Gel* gel = new Gel(Game::object->pWorld, pos, velocity, radius, colorIndex);
    gel->gelColor = ThinDrawer::object->gelShader->gelColors[colorIndex];
    gel->gelRadius = radius;
    gel->gelIndex = gelCount;
    gel->gelType = colorIndex;

    gels[gelCount++] = gel;
}

void Gel::update()
{
    for (int i = 0; i < gelCount; i++)
    {
        Gel* gel = gels[i];

        for (int i = 0; i < gel->numNearGels; i++)
		{
            Gel* g = gel->nearGels[i];
            if (g->gelIndex == -1) continue;
            PortalBody* pb = (*g->pBody).at(g->pBody->size() - 1);
            PortalBody* tpb = (*gel->pBody).at(gel->pBody->size() - 1);
			b2Vec2 pos = tpb->body->GetPosition();
            b2Vec2 posDiff = pos - pb->body->GetPosition();
            float lenPosDiff = posDiff.Normalize();
            float maxPosDiff = gel->gelRadius + g->gelRadius;

            float ratio = lenPosDiff - std::max(g->gelRadius, gel->gelRadius);

            if (ratio < maxPosDiff)
            {
                b2Vec2 velNow = tpb->body->GetLinearVelocity();
                tpb->body->SetLinearVelocity(velNow - 0.05f * ratio * posDiff);
            }
		}

        for (PortalBody* tpb : *gel->pBody)
        {
            b2Vec2 vel = tpb->body->GetLinearVelocity();

            if (vel.Length() > 4.0)
            {
                tpb->body->SetLinearVelocity(0.99f * vel);
            }
        }
    }
}

void Gel::render(VkCommandBuffer cmdBuff, void* ptr)
{
    ThinDrawer::object->gelShader->preCalculate();

    for (int i = 0; i < gelCount; i++)
    {
        Gel* gel = gels[i];
        for (PortalBody* pb : *gel->pBody)
        {
            b2Vec2 pos = pb->body->GetPosition();
            ThinDrawer::object->gelShader->putGel(gel->gelType, glm::vec2(pos.x, pos.y), gel->gelRadius * 2.0);
        }
    }

    ThinDrawer::object->gelShader->render(cmdBuff, ThinDrawer::object->lastSwapChainImageIndex);

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ThinDrawer::object->splash->pipeline);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ThinDrawer::object->splash->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &ThinDrawer::object->camOffset);

    for (int i = 0; i < splashCount; i++)
	{
		splashData sData = splashEffects[i];

        vkCmdPushConstants(cmdBuff, ThinDrawer::object->splash->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(splashData), &sData);

        vkCmdDraw(cmdBuff, 6, 25, 0, 0);

		float dt = Game::object->gameTime - sData.data.y;

		if (dt > sData.data3.y)
		{
            splashEffects[i--] = splashEffects[--splashCount];
			continue;
		}
	}
}

void Gel::endContact(b2Contact* contact, b2Fixture* otherFix)
{
	if (otherFix->IsSensor())
	{
		bodyData* bData = (bodyData*)otherFix->GetBody()->GetUserData().pointer;
		if (bData && bData->extraData)
		{
			g_PhysicsData* physicsData = (g_PhysicsData*)bData->extraData;
			if (physicsData->type & GEL_O)
			{
				Gel* gel = (Gel*)physicsData->data;
				for (int i = 0; i < numNearGels; i++)
				{
					if (nearGels[i] == gel)
					{
						nearGels[i] = nearGels[--numNearGels];
						break;
					}
				}
			}
		}
	}
}

void Gel::beginContact(b2Contact* contact, b2Fixture* otherFix)
{
    if (!contact->IsEnabled()) return;

    fixtureData* fData = (fixtureData*)otherFix->GetUserData().pointer;
    if (fData)
    {
        if (fData->type & F_PORTAL)
        {
            Portal* p = (Portal*)fData->data;
            if (p->midFixture == otherFix && p->connections[0].size() > 0) return;
        }
        else if (fData->type & F_FEDGE)
        {
            return;
        }
    }
    else if (otherFix->GetBody()->GetType() == b2_kinematicBody && otherFix->GetShape()->GetType() == b2Shape::e_polygon)
    {
        return;
    }
    
    if (!otherFix->IsSensor())
    {
        splash(contact, otherFix);
    }
    else if (numNearGels < std::size(nearGels))
    {
        bodyData* bData = (bodyData*)otherFix->GetBody()->GetUserData().pointer;
        if (bData && bData->extraData)
        {
            g_PhysicsData* physicsData = (g_PhysicsData*)bData->extraData;
            if (physicsData->type & GEL_O)
            {
                Gel* gel = (Gel*)physicsData->data;
                if (gel->gelType == gelType)
                    nearGels[numNearGels++] = gel;
            }
        }
    }
    else
    {
        splash(contact, otherFix);
    }
}