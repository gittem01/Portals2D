#include "GameObjects/LaserEmitter.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Gel.h"
#include "Overlayer.h"

std::vector<LaserEmitter> LaserEmitter::laserEmitters(128);
int LaserEmitter::numLaserEmitters = 0;
s_perTextureData* LaserEmitter::ptData;
s_texture* LaserEmitter::tex;

static b2Vec2 fwdDirs[4] = { { 0.0, 1.0 }, { -1.0, 0.0 }, { 0.0, -1.0 }, { 1.0, 0.0 } };

static b2Vec2 bodyOffsets[4] =
{
	b2Vec2(0.5f, 0.1f),
	b2Vec2(0.9f, 0.5f),
	b2Vec2(0.5f, 0.9f),
	b2Vec2(0.1f, 0.5f),
};

static b2Vec2 vertices[2] = { b2Vec2(+0.299f, -0.1f), b2Vec2(0.0f, 0.35f)};

static splashData sData;

LaserEmitter::LaserEmitter(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	laserDir = fwdDirs[objData->rotation % 4];
	lastLaserCube = false;
	lastWallReceiver = nullptr;

	b2Vec2 offset = bodyOffsets[objData->rotation % 4];
	laserPos = b2Vec2(objData->pos.x + offset.x, objData->pos.y + offset.y);

	soundData.pos0 = glm::vec2(laserPos.x, laserPos.y);

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = laserPos;
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	b2Body* body = portalWorld->CreateBody(&bDef);

	b2EdgeShape shape;
	shape.SetOneSided(	b2Vec2(vertices[0].x, vertices[0].y - 0.1), vertices[0],
						vertices[1], b2Vec2(-vertices[0].x, vertices[0].y));
	body->CreateFixture(&shape, 0.0f);

	shape.SetOneSided(	b2Vec2(vertices[0].x, vertices[0].y), vertices[1],
						b2Vec2(-vertices[0].x, vertices[0].y), b2Vec2(-vertices[0].x, vertices[0].y - 0.1));
	body->CreateFixture(&shape, 0.0f);

	//fixtureData* fData = new fixtureData();
	//fData->type = F_LASER_EMITTER;
	//fData->data = (void*)this;
	//fData->extraData = nullptr;

	//fix->GetUserData().pointer = (uintptr_t)fData;

	Sounder::object->putStatOnGrid(&soundData, "laserloop.wav", 0.25f, objData->startActive, true);
}

LaserEmitter* LaserEmitter::createLaserEmitter(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	LaserEmitter* lsr = &laserEmitters[numLaserEmitters++];
	return new (lsr) LaserEmitter(portalWorld, objData);
}

void LaserEmitter::toDeath()
{
	for (int i = 0; i < numLaserEmitters; i++)
	{
		LaserEmitter& lsr = laserEmitters[i];

		lsr.laserRays.clear();
		lsr.lastLaserCube = true;
		lsr.gndLaserMasks.reset();
		lsr.enableSet.reset();
		lsr.lastWallReceiver = nullptr;
	}
}

void LaserEmitter::staticInit()
{
	constexpr int index = getLevelObjectIndex(LASERER) - 4;

	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->earlyDrawFuncs.push_back({ render, nullptr });

	sData.posSpeed.x = 0.0;
	sData.posSpeed.y = 0.0;
	sData.posSpeed.z = 0.0f;
	sData.posSpeed.w = 1.0f;
	sData.data.x = 0.15;
	sData.data.y = -1000.0;
	sData.data.z = b2_pi * 0.5;

	sData.color = glm::vec4(1.0, 0.0, 0.0, 1.0);

	sData.data2.z = 2.0;
	sData.data2.x = 2.0;
	sData.data2.y = 3.0;
	sData.data2.w = 0.5;

	sData.data3.x = 0.1;
	sData.data3.y = 1.0;
	sData.data3.z = 2.0;
	sData.data3.w = 0.01;
}

void LaserEmitter::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ManyTexture* mt = Leveler::object->ol->mt;
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	if (ptData->currentOffset - ptData->offsetStart == 0)
	{
		return;
	}

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->laserPshShader->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->laserPshShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->playerShader->buffers[0].buffer, &offsets);

	for (int i = 0; i < numLaserEmitters; i++)
	{
		LaserEmitter& lsr = laserEmitters[i];

		if (lsr.laserRays.size() == 0) continue;

		for (int i = 0; i < lsr.laserRays.size(); i++)
		{
			s_laserUbo laserData;
			laserData.points = glm::vec4(	lsr.laserRays[i].startPos.x, lsr.laserRays[i].startPos.y,
											lsr.laserRays[i].endPos.x, lsr.laserRays[i].endPos.y);
			laserData.colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			laserData.data = glm::vec4(0.4, 11.0, 36.0, 0.0);

			vkCmdPushConstants(cmdBuff, td->laserPshShader->pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_laserUbo), &laserData);

			vkCmdDraw(cmdBuff, 6, 1, 0, 0);
		}
	}

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->splash->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->splash->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	for (int i = 0; i < numLaserEmitters; i++)
	{
		LaserEmitter& lsr = laserEmitters[i];

		if (lsr.laserRays.size() == 0 || lsr.lastLaserCube) continue;

		sData.posSpeed.x = lsr.endPos.x; sData.posSpeed.y = lsr.endPos.y;
		sData.posSpeed.z = (lsr.laserPos.x + lsr.laserPos.y) / 800.0f + 1.0f;
		sData.data.w = lsr.endAngle - b2_pi * 0.25f;

		vkCmdPushConstants(cmdBuff, td->splash->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(splashData), &sData);

		vkCmdDraw(cmdBuff, 6, 100, 0, 0);
	}

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mt->pipeline);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mt->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mt->pipelineLayout, 1, 1, &mt->descriptorSets[1], 0, VK_NULL_HANDLE);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mt->pipelineLayout, 2, 1, &tex->set, 0, VK_NULL_HANDLE);

	vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, ptData->currentOffset - ptData->offsetStart, 0, ptData->offsetStart);
}

void LaserEmitter::handleGroundReceivers()
{
	for (auto pair : Game::object->pWorld->rayHandler->groundReceivers)
	{
		if (pair.second <= Game::object->pWorld->rayHandler->minFraction)
		{
			enableSet[pair.first] = true;
			GndLaserReceiver& glr = GndLaserReceiver::gndLaserReceivers[pair.first];

			b2Vec2 bodyPos = glr.body->GetPosition();
			b2Vec2 upVec = 0.2f * glr.body->GetTransform().q.GetYAxis();

			laserEmitters[0].laserRays.push_back({ bodyPos - upVec, bodyPos + upVec });

			if (!gndLaserMasks[pair.first])
			{
				glr.enable(this);
			}
		}
	}
}

void LaserEmitter::update()
{
	for (int i = 0; i < numLaserEmitters; i++)
	{
		LaserEmitter& lsr = laserEmitters[i];
		
		lsr.laserRays.clear();
		
		if (!lsr.enabled)
		{
			if (lsr.lastWallReceiver)
			{
				lsr.lastWallReceiver->disable(&lsr);
				lsr.lastWallReceiver = nullptr;
			}
			if (lsr.gndLaserMasks.any())
			{
				for (int i = 0; i < GndLaserReceiver::numGndLaserReceivers; i++)
				{
					if (lsr.gndLaserMasks[i])
					{
						GndLaserReceiver& glr = GndLaserReceiver::gndLaserReceivers[i];
						glr.disable(&lsr);
					}
				}

				lsr.gndLaserMasks.reset();
			}

			continue;
		}
		
		lsr.collidedLaserCubes.clear();
		lsr.enableSet.reset();

		PortalWorld* pWorld = Game::object->pWorld;

		pbWorldData* pbData = nullptr;
		b2Vec2 laserPos = lsr.laserPos;
		b2Vec2 laserDir = lsr.laserDir;

	ray_send:
		lsr.lastLaserCube = false;

		pWorld->rayHandler->sendRay(laserPos, laserDir, 400.0f, pbData, (uint32_t)LSR_FILTER_MASK);

		lsr.handleGroundReceivers();

		for (int j = 0; j < pWorld->rayHandler->rayResult.size(); j++)
		{
			lsr.laserRays.push_back({ pWorld->rayHandler->rayResult[j].startPos, pWorld->rayHandler->rayResult[j].endPos });
		}
		
		if (pWorld->rayHandler->closestFixture != nullptr)
		{
			b2Body* collBody = pWorld->rayHandler->closestFixture->GetBody();
			bodyData* bData = (bodyData*)collBody->GetUserData().pointer;
			if (bData && bData->extraData)
			{
				PortalBody* pb = (PortalBody*)bData->data;
				g_PhysicsData* pData = (g_PhysicsData*)bData->extraData;
				if (pData->type & LASER_CUBE_O)
				{
					b2Vec2 lastLaserPos = pWorld->rayHandler->rayResult[0].endPos;
					b2Vec2 bodyCenter = collBody->GetPosition();
					laserEmitters[0].laserRays.push_back({lastLaserPos, bodyCenter});

					LaserCube* lc = (LaserCube*)pData->data;
					if (lsr.collidedLaserCubes.find(lc->wData) == lsr.collidedLaserCubes.end())
					{
						lsr.collidedLaserCubes.insert(lc->wData);

						if (lc->wData->pbIndices.size() <= 1)
						{
							laserPos = collBody->GetPosition();
							laserDir = collBody->GetTransform().q.GetXAxis();
							if (pb->isMirrored) laserDir = -laserDir;
						}
						else
						{
							pbData = lc->wData;

							//PortalBody* pb = lc->wData->pbIndices[0];
							for (bodyCollisionStatus bcs : pb->bodyMaps)
							{
								b2Vec2 p2 = PortalWorld::getRelativePortalPosition_W(lastLaserPos, bcs.connection);
								bodyCenter = bcs.body->body->GetPosition();
								laserEmitters[0].laserRays.push_back({p2, bodyCenter});

								if (bcs.connection->portal1->getPointSide(pb->body->GetPosition()) != bcs.connection->side1)
								{
									b2Body* body2 = bcs.body->body;
									laserPos = body2->GetPosition() + 0.001f * bcs.connection->portal2->dir;
									laserDir = body2->GetTransform().q.GetXAxis();
									if (bcs.body->isMirrored) laserDir = -laserDir;
								}
								else
								{
									b2Body* body = pb->body;
									laserPos = body->GetPosition() + 0.001f * bcs.connection->portal1->dir;
									laserDir = body->GetTransform().q.GetXAxis();
									if (pb->isMirrored) laserDir = -laserDir;
								}
							}
						}

						goto ray_send;
					}
					else
					{
						lsr.lastLaserCube = true;
					}
				}
			}
		}

		if (lsr.gndLaserMasks != lsr.enableSet)
		{
			for (int i = 0; i < GndLaserReceiver::numGndLaserReceivers; i++)
			{
				if (lsr.gndLaserMasks[i] && !lsr.enableSet[i])
				{
					GndLaserReceiver& glr = GndLaserReceiver::gndLaserReceivers[i];
					glr.disable(&lsr);
				}
			}

			lsr.gndLaserMasks = lsr.enableSet;
		}

		WallLaserReceiver* wlr = nullptr;
		if (lsr.laserRays.size() > 0 && !lsr.lastLaserCube)
		{
			fixtureData* fData = nullptr;
			if (pWorld->rayHandler->closestFixture) fData = (fixtureData*)pWorld->rayHandler->closestFixture->GetUserData().pointer;

			if (fData && fData->type == F_WALL_RECEIVER)
			{
				wlr = (WallLaserReceiver*)fData->data;
				wlr->enable(&lsr);
				lsr.lastLaserCube = true;

				b2Vec2 lastPos = pWorld->rayHandler->rayResult[0].endPos;
				b2Vec2 recCenter = wlr->body->GetPosition();

				s_simpleRay toCenterRay = { lastPos, recCenter };

				laserEmitters[0].laserRays.push_back(toCenterRay);
			}
			else
			{
				b2Vec2 vec = pWorld->rayHandler->endNormal;
				lsr.endAngle = atan2(vec.y, vec.x);
				lsr.endPos = pWorld->rayHandler->rayResult[0].endPos;
			}
		}

		if (lsr.lastWallReceiver && wlr != lsr.lastWallReceiver)
		{
			lsr.lastWallReceiver->disable(&lsr);
		}

		lsr.lastWallReceiver = wlr;
	}
}