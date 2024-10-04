#pragma once

#include <WindowHandler.h>
#include <PortalGunRayCast.h>
#include <GameBase.h>
#include <Camera.h>
#include "PortalBody.h"
#include "Portal.h"
#include <SwapChain.h>
#include "ThinDrawer.h"
#include <Sounder.h>
#include <DefaultObjects/Gun.h>
#include <Game.h>

class ForceField;

class TestPlayer : public DefaultObject
{
public:
    static TestPlayer* object;

    PlayerShader* playerShader;
    std::vector<s_gunData> shootDatas;
    s_gunData gData;

    Portal* portalA;
    Portal* portalB;
    float lastPortalCreationTime;

    PortalBody* holdBody;
    PortalBody* holdGun;
    Portal* destrPortal;
    b2Vec2 lastHoldPos;
    portalConnection* holdConnection;
    float lastBodyHoldTime;
    float releaseRequestTime;
    float holdERequestTime;
    float holdAngle;
    float angVal;
    bool shouldRelease;
    bool holdColliding;
    bool fastRun;
    int health;
    double deathTimer;

    std::vector<PortalBody*>* gun;
    Game* game;
    double gameTime;
    WindowHandler* wh;
    b2RevoluteJoint* oldJoint;
    PortalBody* oldPortalBody;
    
    b2Vec2 bodySize;
    b2Vec2 gunSize;

    bool grounded;
    b2Fixture* tireFixture;
    b2Fixture* contactFixture;
    b2Vec2 contactPoint;
    b2Body* contactBody;
    b2Body* platformBody;
    b2Body* l_contactBody;
    b2Body* l_platformBody;

    b2BodyType contactType;
    b2BodyType rightCollisionType = b2_dynamicBody;
    b2BodyType leftCollisionType = b2_dynamicBody;

    PortalBody* lastBody;
    bool bodySwitch;
    float totalTime = 0.1f;
    float switchTimer;

    int lastOrangeGelSetOffset = -1;
    int lastKey;
    int keyBeforeSwitch;

    bool right;
    bool left;
    int lastDoubleKey;

    float onAirFor;
    float avgNormalX;
    int numNormals;
    float normXToWalk;
    float setFriction;

    s_renderPlayerUbo renderData;
    float legAng;
    float rotSpeed;

    b2Vec2 lastDirs[2];

    ALuint jumpBuffer;
    ALuint blueJumpBuffer;
    ALuint blueShootBuffer;
    ALuint orangeShootBuffer;
    ALuint blueShootBufferJ;
    ALuint orangeShootBufferJ;
    ALuint fizzleBuffer;

    TestPlayer(Game* game, b2Vec2 pos = b2Vec2(0, 0), b2Vec2 size = b2Vec2(0.75f, 1.5f));

    void init();

    static void portalDestrCallBack(Portal* portal);
    static float springMovement(float value, float vel, float springFactor, float dampingFactor, float deltaTime);

    static void memMap(void* ptr);
    static void memMapDeathByElectrocution(void* ptr);
    static void memMapDeathNatural(void* ptr);
    static void zoomOut(void* ptr);

    static void draw(VkCommandBuffer cmdBuff, void* ptr);

    static void drawGunEffect(VkCommandBuffer cmdBuff, void* ptr);

    void renderInit();

    void createBody(b2Vec2 pos);

    void enterForceField(ForceField* forceField);
    void beginContact(b2Contact* contact, b2Fixture* otherFix, b2Fixture* thisFix);
    void endContact(b2Contact* contact, b2Fixture* otherFix);
    void preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2);
    void reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2);
    
    void camHandle();
    void handleBodyRotation();

    void handleGunRotation();

    b2Fixture* canCreatePortal(b2Fixture* edgeFix, b2Vec2 pos, float size, Portal* oldPortal);

    void portalClosingRitual();
    void getBodyOut();
    PortalBody* getOuterGun();

    void setXColor(bool canBlue, bool canRed);

    void handlePortalCD(float dt);

    static void gunDestrCallback(PortalBody* gb, void* ptr);

    static void objectDestrCallback(PortalBody* pb, void* ptr);

    void holdB(PortalBody* pb, PortalBody* gunBody);

    void releaseB();

    void handleObjectHold(PortalBody* gunBody, b2Vec2 pos, b2Vec2 dir, float dt);

    void portalEffect(Portal* portal);
    void runEffect();
    void jumpEffect();
    void healthCheck();
    void updateReset();
    void update(float dt, int totalIter);
};