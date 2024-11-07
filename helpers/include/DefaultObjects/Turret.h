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
	static std::vector<Turret*> turrets;
	static s_texture* coverTex;
	static s_texture* gunTex;
	static s_texture* glowTex;
	static s_buffers* coverVertexBuffer;
	static std::vector<uint32_t> soundBuffers;

	bool mirrored;

	PortalBody* eyeBody;
	std::vector<centerLaser> laserPoints;
	float midX;
	b2Vec2 fwd;
	b2Vec2 eyePos;
	b2Vec2 laserDir;
	b2Vec2 trackPoint;
	double lastTrackTime = -10.0;
	void* lastTrackPtr = nullptr;
	bool tracking = false;

	uint32_t shootGundex = 0;
	double lastShootTime = -10.0;
	double shootTimes[2] = { -10.0, -10.0 };
	double fallTime = -10.0;
	float gunBacks[2] = { 0.0f, 0.0f };
	bool status = false; // true for deploying, false for retracting
	bool deactivated = false;
	float openAmount = 0.0f;
	float rotation = 0.0f;

	uint32_t bodyFixDataIndex;

	std::vector<std::pair<s_laserUbo, uint32_t>> renderLaserDatas;
	uint32_t firstLaserIndex = 0;
	uint32_t lastLaserIndex = 0;

	Turret(PortalWorld* portalWorld, b2Vec2 pos, bool mirrored);

	void createBody(b2Vec2 pos);

	void shoot();

	void lookAt(glm::vec2 pos);
	void aimAt(glm::vec2 pos);
	void deploy();
	void retract();
	void sayGoodbye();

	void recreate(bool unused_mirrored=false) override;

	static void staticInit();
	static void draw(VkCommandBuffer cmdBuff, void* ptr);
	static void perTickUpdate(float dt);
};