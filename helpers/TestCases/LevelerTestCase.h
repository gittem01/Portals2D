#pragma once

#include <BaseTestCase.h>
#include <GameBase.h>
#include <DefaultObjects/DefaultCube.h>
#include <DefaultObjects/LaserCube.h>
#include <DefaultObjects/Sphere.h>
#include <DefaultObjects/Cyber.h>
#include <DefaultObjects/Turret.h>

#include <DefaultObjects/Gel.h>
#include "GameObjects/ForceField.h"
#include "GameObjects/HazardField.h"
#include "GameObjects/Platform.h"
#include "GameObjects/Geller.h"

#include <Leveler.h>

typedef struct
{
    std::vector<int> offsets;
    Receiver* receiver;
    bool secondaryFunc;
} connectionSignaller;

typedef enum
{
    NOT_REACHABLE = 0x0,
    BW_REACHABLE = 0x1,
    GBW_REACHABLE = 0x2,
} ReachableType;

typedef enum
{
    DOWN_W      = 0x1 << 0,
    RIGHT_W     = 0x1 << 1,
    UP_W        = 0x1 << 2,
    LEFT_W      = 0x1 << 3,

    DOWN_G      = 0x1 << 4,
    RIGHT_G     = 0x1 << 5,
    UP_G        = 0x1 << 6,
    LEFT_G      = 0x1 << 7,

    DOWN_F      = 0x1 << 8,
    RIGHT_F     = 0x1 << 9,
    UP_F        = 0x1 << 10,
    LEFT_F      = 0x1 << 11,
} EdgeUse;

inline ReachableType& operator |=(ReachableType& a, ReachableType b)
{
    return a = (ReachableType)(a | b);
}

inline EdgeUse& operator |=(EdgeUse& a, EdgeUse b)
{
    return a = (EdgeUse)(a | b);
}

class Transmitter;
class Receiver;

class LevelerTestCase : public BaseTestCase
{
public:
    std::vector<s_gridSquare> nextVisitSquares[2];
    std::vector<std::vector<ReachableType>> reachableMap;
    std::vector<std::vector<EdgeUse>> edgeUsedMap;
    std::map<s_objectDataGrid*, void*> connMap;
    bool menuUpdate = false;
    fixtureData outerFixDatas[2];

    LevelerTestCase(Game* game, int levelIndex=8);

    static void wallMemFill(b2Vec2 p0, b2Vec2 p1, int index, int offset, int size, int prevIndex, bool initial=false);

    void reachableFillFrom(s_gridSquare from, LevelObject mask, ReachableType reachFiller);
    void fillReachableMap();
    void createDefaultEdges();
    void walkArndAndCreateEdge(short x, short y);
    void walkFwrdAndCreateEdge(glm::i16vec2 pos, char dir, glm::i16vec2* nextPos, glm::i16vec2* endPos, char* nextDir, std::vector<s_unitWallData>& wallData);

    short getDirs(glm::i16vec2 p0, glm::i16vec2 p1, short dir);
    void walkArndAndCreateEdgeGF(short x, short y, LevelObject wallType);
    void walkFwrdAndCreateEdgeGF(   glm::i16vec2 pos, char dir, LevelObject wallMask, ReachableType empMask, int extraShift,
                                    glm::i16vec2* nextPos, glm::i16vec2* endPos, char* nextDir, bool* emptyFound);

    void receiverToTransmitter(s_objectDataGrid* objectData, Receiver* receiver);
    void transmitterToReceiver(s_objectDataGrid* objectData, Transmitter* transmitter);
    void createDynamics();

    void perFrameUpdate();
    void onUpdate();
    void onCreate();
};
