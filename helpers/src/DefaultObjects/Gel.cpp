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
    bDef.position.Set(pos.x, pos.y);
    bDef.linearDamping = 0.6;

    b2CircleShape cShape;
    cShape.m_radius = radius;

    b2Body* body = pWorld->CreateBody(&bDef);
    body->SetLinearVelocity(b2Vec2(velocity.x, velocity.y));

    b2Fixture* fix = body->CreateFixture(&cShape, 0.1f);
    fix->SetSensor(true);

    PortalBody* portalBody = portalWorld->createPortalBody(body);
    *portalBody->gravityScale = 0.3;

    bodyData* bData = (bodyData*)body->GetUserData().pointer;
    g_PhysicsData* physicsData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
    physicsData->type = (g_DataType)(DEFAULT_OBJECT_O | GEL_O);
    physicsData->data = this;
    bData->extraData = physicsData;

    this->pBody = &portalBody->worldIndex->pbIndices;

    //ThinDrawer::object->debug_addPortalBody(portalBody);
}

void Gel::staticInit()
{
    ThinDrawer::object->midDrawFuncs.push_back({ &Gel::render, nullptr });
}

void Gel::splash()
{
    if (gelIndex == -1) return;

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
            PortalBody* pb = (*g->pBody).at(0);
            PortalBody* tpb = (*gel->pBody).at(0);
			b2Vec2 pos = tpb->body->GetPosition();
            b2Vec2 posDiff = pos - pb->body->GetPosition();
            b2Vec2 velNow = tpb->body->GetLinearVelocity();
            //tpb->body->SetLinearVelocity(velNow - 0.02f * posDiff);
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
    fixtureData* fData = (fixtureData*)otherFix->GetUserData().pointer;
    if (fData && fData->type == F_PORTAL)
    {
        Portal* p = (Portal*)fData->data;
        if (p->midFixture == otherFix && p->connections[0].size() > 0) return;
    }

    if (!otherFix->IsSensor())
    {
        ALuint source = Sounder::object->playSound(onHitSoundBuffers[0], 0.5);
        Sounder::object->setSoundPosition(source, glm::vec3(pBody->at(0)->body->GetPosition().x, pBody->at(0)->body->GetPosition().y, 0.0));
        splash();
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
                nearGels[numNearGels++] = gel;
            }
        }
    }
    else
    {
        ALuint source = Sounder::object->playSound(onHitSoundBuffers[0], 0.5);
        Sounder::object->setSoundPosition(source, glm::vec3(pBody->at(0)->body->GetPosition().x, pBody->at(0)->body->GetPosition().y, 0.0));
        splash();
    }
}