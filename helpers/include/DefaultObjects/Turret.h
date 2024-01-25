#pragma once

#include <DefaultObject.h>
#include <vulkan/vulkan.h>
#include <ShaderBase.h>
#include <LaserShader.h>

const float shootTimer = 0.05f;
const float rotationLimit = 0.5f;
const float fullBackInTime = shootTimer;
const uint32_t numLasers = 16;

typedef struct centerLaser
{
	b2Vec2 from;
	b2Vec2 to;
} centerLaser;

class Turret : public DefaultObject
{
public:
	static std::set<Turret*> turrets;
	static s_texture* coverTex;
	static s_texture* gunTex;
	static s_texture* glowTex;
	static s_buffers* coverVertexBuffer;
	static std::vector<uint32_t> soundBuffers;

	PortalBody* eyeBody;
	std::vector<centerLaser> laserPoints;
	float midX;
	uint32_t shootGundex = 0;
	double lastShootTime = -10.0;
	double shootTimes[2] = { -10.0, -10.0 };
	float gunBacks[2] = { 0.0f, 0.0f };
	bool status = false; // true for deploying, false for retracting
	float openAmount = 0.0f;
	float rotation = 0.0f;

	uint32_t bodyFixDataIndex;

	std::vector<std::pair<s_laserUbo, uint32_t>> renderLaserDatas;
	uint32_t firstLaserIndex = 0;
	uint32_t lastLaserIndex = 0;

	Turret(PortalWorld* portalWorld);

	void createBody();

	void shoot();

	void lookAt(glm::vec2 pos);
	void aimAt(glm::vec2 pos);
	void deploy();
	void retract();
	void sayGoodbye();

	static void staticInit();
	static void draw(VkCommandBuffer cmdBuff, void* ptr);
	static void update();
};