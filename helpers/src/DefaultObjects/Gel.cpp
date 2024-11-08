#include <DefaultObjects/DefaultCube.h>
#include <DefaultObjects/Gel.h>
#include <ThinDrawer.h>
#include <Game.h>
#include <Portal.h>

#define GELLABLE (DEFAULT_CUBE_O | LASER_CUBE_O | SPHERE_CUBE_O | TURRET_O | GUN_O)

std::vector<Gel*> Gel::gels(MAX_GEL_COUNT);
std::vector<splashData> Gel::splashEffects(4096);
int Gel::gelCount = 0;
int Gel::splashCount = 0;

Gel::Gel(PortalWorld* portalWorld, glm::vec2 pos, glm::vec2 velocity, float radius, int gelType) : DefaultObject(portalWorld)
{
    this->pWorld = portalWorld;
    ALuint onHitSoundBuffer = Sounder::object->loadSound("pop.wav");
    onHitSoundBuffers.push_back(onHitSoundBuffer);
    
    b2BodyDef bDef;
    bDef.type = b2_dynamicBody;
    //bDef.type = b2_kinematicBody;
    bDef.position.Set(pos.x, pos.y);
    bDef.linearDamping = 0.5;

    b2CircleShape cShape;
    cShape.m_radius = radius;

    b2Body* body = pWorld->CreateBody(&bDef);
    body->SetLinearVelocity(b2Vec2(velocity.x, velocity.y));

    b2Fixture* fix = body->CreateFixture(&cShape, 0.1f);
    fix->SetSensor(true);

    PortalBody* portalBody = portalWorld->createPortalBody(body);
    wData = portalBody->worldIndex;
    
    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    pData.type = (g_DataType)(DEFAULT_OBJECT_O | GEL_O);
    pData.data = this;
    bData->extraData = &pData;

    // too slow
    // must be done once in the game initialisation but it is not possible for now
    // ThinDrawer::object->debug_addPortalBody(portalBody);
}

void Gel::recreate()
{
    for (int j = wData->pbIndices.size() - 1; j >= 0; j--)
    {
        PortalBody* portalBody = wData->pbIndices[j];

        delete portalBody;
    }
}

void Gel::staticInit()
{
    gelCount = 0;
    splashCount = 0;

    ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
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
    if (fData && fData->type & (F_EDGE | F_LIGHT_BRIDGE))
    {
        s_edgeLocalData* eLocal = (s_edgeLocalData*)fData->data;
        *data = eLocal->globalData;
        range[0] = std::min(eLocal->globalRange[0], (int)eLocal->globalData->unitWalls.size() - 1);
        range[1] = std::min(eLocal->globalRange[1], (int)eLocal->globalData->unitWalls.size() - 1);
    }
    else
    {
        *data = nullptr;
    }
}

void setFrictions(PbWorldData* wData, float friction)
{
	for (PortalBody* pb : wData->pbIndices)
	{
		for (int i = 0; i < pb->fixtures.size(); i++)
		{
			b2Fixture* fix = pb->fixtures[i];
            if (friction >= 0.0)
            {
			    fix->SetFriction(friction);
            }
            else
            {
                fix->SetFriction(wData->baseFrictions[i]);
            }
		}

		for (b2ContactEdge* ce = pb->body->GetContactList(); ce; ce = ce->next)
		{
			b2Contact* contact = ce->contact;
			if (contact->IsTouching())
			{
                if (friction >= 0.0)
                {
                    contact->SetFriction(friction);
                }
                else
                {
                    contact->SetFriction(wData->baseFrictions[0]);
                }
			}
		}
	}
}

void Gel::splash(b2Contact* contact, b2Fixture* otherFix)
{
    if (gelIndex == -1) return;

    ALuint source = Sounder::object->playSound(onHitSoundBuffers[0], gelRadius);
    Sounder::object->setSoundPosition(source, glm::vec3(wData->pbIndices[0]->body->GetPosition().x, wData->pbIndices[0]->body->GetPosition().y, 0.0));

    int range[2];
    s_edgeGlobalData* eGlobalData;
    getRangeData(contact->GetFixtureB()->GetBody()->GetPosition(), otherFix, range, &eGlobalData);

    if (eGlobalData)
    {
        b2Vec2 v1;
        int extraOffset;
        b2Vec2 dirVec;

        fixtureData* fData = (fixtureData*)otherFix->GetUserData().pointer;
        if (fData && fData->type & F_LIGHT_BRIDGE)
        {
            b2EdgeShape* edge = (b2EdgeShape*)otherFix->GetShape();
            dirVec = edge->m_vertex2 - edge->m_vertex1;
            dirVec.Normalize();
            v1 = edge->m_vertex1;
            s_edgeLocalData* eLocal = (s_edgeLocalData*)fData->data;
            extraOffset = eLocal->globalRange[0];
        }
        else
        {
            v1 = eGlobalData->localPoints[0];
            extraOffset = 0;
            dirVec = eGlobalData->dir;
        }

        b2Vec2 distVec = contact->GetFixtureB()->GetBody()->GetPosition() - otherFix->GetBody()->GetWorldPoint(v1);
        float dist = std::max(b2Dot(distVec, dirVec), 0.0f);
        int startPos = std::min((int)std::round(dist * 10.0f) + extraOffset, (int)eGlobalData->unitWalls.size() - 1);
        
        float amnt = gelRadius * 10.0;

        if (eGlobalData->unitWalls[startPos].wallType & (PORTALLABLE | NOT_PORTALLABLE))
            amnt -= handleGelPut(&eGlobalData->unitWalls[startPos], gelType, amnt);
        else
            goto out;

        int rightPos = startPos;
        int leftPos = startPos;
        for (int i = 1; i < 30; i++)
        {
            rightPos = startPos - i;
            leftPos = startPos + i;

            float decrease = 0.0f;
            if (rightPos >= range[0] && (eGlobalData->unitWalls[rightPos].wallType & (PORTALLABLE | NOT_PORTALLABLE)))
            {
                decrease += handleGelPut(&eGlobalData->unitWalls[rightPos], gelType, amnt);
            }
            if (leftPos <= range[1] && (eGlobalData->unitWalls[leftPos].wallType & (PORTALLABLE | NOT_PORTALLABLE)))
            {
                decrease += handleGelPut(&eGlobalData->unitWalls[leftPos], gelType, amnt);
			}

            amnt -= decrease;
			if (amnt <= 0.0f || decrease == 0.0f) break;
        }

        rightPos = std::max(rightPos, range[0]);
        leftPos = std::min(leftPos, range[1]);

        int mapSize = leftPos - rightPos + 1;
        s_wallPerGelData* mapMem2;
        vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
            (eGlobalData->renderOffset + rightPos) * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData) * mapSize, 0, (void**)&mapMem2);

        for (int i = 0; i < mapSize; i++)
        {
            s_unitWallData wData = eGlobalData->unitWalls[i + rightPos];
            mapMem2[i].data.x = gelTypeToIndex(wData.gelType);
            mapMem2[i].data.y = wData.gelAmount;
        }

        vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);
    }
    else
    {
		bodyData* bData = (bodyData*)otherFix->GetBody()->GetUserData().pointer;
        if (bData && bData->extraData && bData->type == B_PORTAL_BODY)
        {
			g_PhysicsData* pData = (g_PhysicsData*)bData->extraData;
            if (pData->type & GELLABLE)
            {
                DefaultObject* defObj = (DefaultObject*)pData->data;
                int prevGel = defObj->renderGroup->gelIndex;
                defObj->renderGroup->gelIndex = gelType;

                if (prevGel == 1 && gelType != 1)
                {
					setFrictions(defObj->wData, -1.0f);
                }
                else if (gelType == 1)
                {
                    setFrictions(defObj->wData, 0.1f);
                }
            }
        }
    }

out:
    b2Vec2 pos = wData->pbIndices[0]->body->GetPosition();

    splashData sData;
    sData.posSpeed.x = pos.x;
    sData.posSpeed.y = pos.y;
    sData.posSpeed.z = (float)rand() / RAND_MAX;
    sData.posSpeed.w = 1.0;
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

    for (PortalBody* pb : wData->pbIndices)
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
            PortalBody* pb = g->wData->pbIndices[g->wData->pbIndices.size() - 1];
            PortalBody* tpb = gel->wData->pbIndices[gel->wData->pbIndices.size() - 1];
			b2Vec2 pos = tpb->body->GetPosition();
            b2Vec2 posDiff = pos - pb->body->GetPosition();
            float lenPosDiff = posDiff.Normalize();
            float maxPosDiff = gel->gelRadius + g->gelRadius;

            float ratio = lenPosDiff - std::max(g->gelRadius, gel->gelRadius);

            if (ratio < maxPosDiff)
            {
                b2Vec2 velNow = tpb->body->GetLinearVelocity();
                tpb->body->SetLinearVelocity(velNow - 0.01f * ratio * posDiff);
            }
		}
    }
}

void Gel::toDeath()
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    Gel::gelCount = 0;
    Gel::splashCount = 0;

    int clearSize = thinDrawer->wallGel->numWallGels + 48000 * 6;
    s_wallPerGelData* perGelData;
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->wallGel->uniformBuffers[1].memory,
        0, sizeof(s_wallPerGelData) * clearSize, 0, (void**)&perGelData);

    for (int i = 0; i < clearSize; i++)
    {
        perGelData[i].data = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->wallGel->uniformBuffers[1].memory);

    for (b2Body* body = Game::object->pWorld->GetBodyList(); body; body = body->GetNext())
    {
        b2Fixture* fix = body->GetFixtureList();
        if (!fix)
        {
            continue;
        }

        for (; fix; fix = fix->GetNext())
        {
            fixtureData* fData = (fixtureData*)fix->GetUserData().pointer;
            if (fData && fData->type == F_EDGE && fix->GetType() == b2Shape::e_edge)
            {
                s_edgeLocalData* eLocal = (s_edgeLocalData*)fData->data;
                s_edgeGlobalData* eGlobal = eLocal->globalData;
                for (auto& unitWall : eGlobal->unitWalls)
                {
                    unitWall.gelAmount = 0.0f;
                    unitWall.gelType = NONE;
                }
            }
        }
    }
}

bool Gel::bounceAt(b2Body* body, b2Fixture* edgeFix, b2Vec2 pos)
{
    int index = 0;
    int extraJumpRange[2] = { 0, 0 };

    s_edgeGlobalData* eGlobal = nullptr;

    fixtureData* fData = (fixtureData*)edgeFix->GetUserData().pointer;
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
            b2EdgeShape* edge = (b2EdgeShape*)edgeFix->GetShape();
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

        b2Vec2 diff = pos - edgeFix->GetBody()->GetWorldPoint(v1);
        index = (int)std::round(b2Dot(diff, dirVec) * 10.0) + extraOffset;
        index = std::max(std::min(index, (int)eGlobal->unitWalls.size() - 1), 0);

        extraJumpRange[0] = index;
        extraJumpRange[1] = index;

        for (int i = 1; i < 11; i++)
        {
            bool found = false;

            if (index - i >= 0)
            {
                int arrIndex = index - i;
                s_unitWallData uData = eGlobal->unitWalls.at(arrIndex);
                if (uData.gelType & BLUE && uData.gelAmount > 0.25)
                {
                    extraJumpRange[0] = index - i;
                    found = true;
                }
            }
            if (index + i < eGlobal->unitWalls.size())
            {
                int arrIndex = index + i;
                s_unitWallData uData = eGlobal->unitWalls.at(arrIndex);
                if (uData.gelType & BLUE && uData.gelAmount > 0.25)
                {
                    extraJumpRange[1] = index + i;
                    found = true;
                }
            }

            if (!found)
            {
                break;
            }
        }

        int memMapSize = extraJumpRange[1] - extraJumpRange[0] + 1;
        if (eGlobal && memMapSize > 5)
        {
            int memMapSize = extraJumpRange[1] - extraJumpRange[0] + 1;
            s_wallPerGelData* mapMem2;
            vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
                (eGlobal->renderOffset + extraJumpRange[0]) * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData) * memMapSize, 0, (void**)&mapMem2);

            for (int i = 0; i < memMapSize; i++)
            {
                float dist = (float)abs(index - extraJumpRange[0] - i) * 0.1;
                mapMem2[i].data.z = Game::object->gameTime + dist * 0.2 - 0.05;
                mapMem2[i].data.w = (1.0 - dist) * 0.4;
            }

            vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);

            return true;
        }
    }

    return false;
}

void Gel::toLeveler()
{
    ThinDrawer* thinDrawer = ThinDrawer::object;

    Gel::gelCount = 0;
    Gel::splashCount = 0;

    int clearSize = thinDrawer->wallGel->numWallGels + 48000 * 6;
    s_wallPerGelData* perGelData;
    vkMapMemory(thinDrawer->logicalDevice, thinDrawer->wallGel->uniformBuffers[1].memory,
        0, sizeof(s_wallPerGelData) * clearSize, 0, (void**)&perGelData);

    for (int i = 0; i < clearSize; i++)
    {
        perGelData[i].data = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->wallGel->uniformBuffers[1].memory);
}

void Gel::perFrameUpdate()
{
    for (int i = 0; i < gelCount; i++)
    {
        Gel* gel = gels[i];

        for (PortalBody* tpb : gel->wData->pbIndices)
        {
            b2Vec2 vel = tpb->body->GetLinearVelocity();

            vel.y = std::clamp(vel.y, -5.0f, 5.0f);

            float randValx = (float)rand() / RAND_MAX;
            float randValy = (float)rand() / RAND_MAX;

            vel += 10.0f * Game::object->deltaTime * b2Vec2(randValx - 0.5, randValy - 0.5);

            tpb->body->SetLinearVelocity(vel);
        }
    }
}

void Gel::render(VkCommandBuffer cmdBuff, void* ptr)
{
    ThinDrawer::object->gelShader->preCalculate();

    for (int i = 0; i < gelCount; i++)
    {
        Gel* gel = gels[i];
        for (PortalBody* pb : gel->wData->pbIndices)
        {
            b2Vec2 pos = pb->body->GetPosition();
            float opRadius = gel->gelRadius * 2.0;
            for (auto bm : pb->bodyMaps)
            {
                float dist = PortalWorld::getRealDist(bm->connection->portal1->points[0], bm->connection->portal1->points[1], pos);
                float remVal = std::max((gel->gelRadius + dist) - 0.5f, 0.0f);
                opRadius = std::min(gel->gelRadius * 2.0f - remVal * 2.0f, opRadius);
            }

            ThinDrawer::object->gelShader->putGel(gel->gelType, glm::vec2(pos.x, pos.y), opRadius);
        }
    }

    if (gelCount > 0)
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

void Gel::beginContact(b2Contact* contact, b2Fixture* otherFix, b2Fixture* thisFix)
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
        else if (fData->type & GEL_COLLISION_IGNORE ||
            otherFix->GetBody()->GetType() == b2_kinematicBody && otherFix->GetShape()->GetType() == b2Shape::e_polygon)
        {
            return;
        }
    }
    
    if (!otherFix->IsSensor())
    {
        splash(contact, otherFix);
    }
   /* else if (numNearGels < std::size(nearGels))
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
    }*/
}