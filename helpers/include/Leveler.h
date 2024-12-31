#pragma once

#include <vector>
#include <set>
#include <string>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <PlatformRectangle.h>
#include <TestPlayer.h>
#include "CharDrawer.h"

typedef union GridData
{
	void* ptr = nullptr;
	int offset;
	char ext;
} s_gridData;

typedef enum SideData
{
	SIDE_DISABLE = 0x0,
	SIDE_ENABLE = 0x1 << 0,
	DOWN_LINE = 0x1 << 1,
	RIGHT_LINE = 0x1 << 2,
	UP_LINE = 0x1 << 3,
	LEFT_LINE = 0x1 << 4,
	LD_POINT = 0x1 << 5,
	RD_POINT = 0x1 << 6,
	RU_POINT = 0x1 << 7,
	LU_POINT = 0x1 << 8,
	ALL_SIDES = 0x1FF,
} sideData;

typedef enum LevelObject : long long
{
	NONE_LO			= 0x0,
	WHITE_WALL		= 0x1ll << 0,
	BLACK_WALL		= 0x1ll << 1,
	GLASS_WALL		= 0x1ll << 2,
	FENCE_WALL		= 0x1ll << 3,

	CLICK_BUTTON	= 0x1ll << 4,
	DOWN_BUTTON		= 0x1ll << 5,
	CUBE_BUTTON		= 0x1ll << 6,
	SPHERE_BUTTON	= 0x1ll << 7,

	DEFAULT_CUBE	= 0x1ll << 8,
	LASER_CUBE		= 0x1ll << 9,
	SPHERE_CUBE		= 0x1ll << 10,
	TURRET			= 0x1ll << 11,

	LASERER			= 0x1ll << 12,
	LASER_REC_GND	= 0x1ll << 13,
	LASER_REC_WALL	= 0x1ll << 14,
	CATAPULT		= 0x1ll << 15,

	FUNNEL			= 0x1ll << 16,
	LIGHT_BRIDGE	= 0x1ll << 17,
	HAZARDER		= 0x1ll << 18,
	FORCE_FIELD		= 0x1ll << 19,

	BLUE_GELLER		= 0x1ll << 20,
	ORANGE_GELLER	= 0x1ll << 21,
	WHITE_GELLER	= 0x1ll << 22,
	WATER_GELLER	= 0x1ll << 23,

	PLATFORM		= 0x1ll << 24,

	HAZARDER_MID_DU = 0x1ll << 55,
	FIELD_MID_DU	= 0x1ll << 56,
	HAZARDER_MID_LR	= 0x1ll << 57,
	FIELD_MID_LR	= 0x1ll << 58,

	DOWN_DEPENDENT	= 0x1ll << 59,
	RIGHT_DEPENDENT	= 0x1ll << 60,
	UP_DEPENDENT	= 0x1ll << 61,
	LEFT_DEPENDENT	= 0x1ll << 62,

	ERASER			= 0x1ll << 63,
} levelObject;

constexpr int getLevelObjectIndex(levelObject val)
{
	int index = 0;
	val = (LevelObject)(val & 0x1FFFFFF);

	long long value = (long long)val;
	if (value & 0xFFFFFFFF00000000) { value >>= 32; index += 32; }
	if (value & 0xFFFF0000) { value >>= 16; index += 16; }
	if (value & 0xFF00) { value >>= 8;  index += 8; }
	if (value & 0xF0) { value >>= 4;  index += 4; }
	if (value & 0xC) { value >>= 2;  index += 2; }
	if (value & 0x2) { index += 1; }

	return index;
}

constexpr int getRotVal(int rot)
{
	switch (rot & 0xF)
	{
	case 1:
		return 0;
	case 2:
		return 1;
	case 4:
		return 2;
	case 8:
		return 3;
	default:
		return -31;
	}
}

#define GRID_DRAWS (WHITE_WALL | BLACK_WALL | GLASS_WALL | FENCE_WALL | ERASER)
#define GRID_DEPS (DOWN_DEPENDENT | RIGHT_DEPENDENT | UP_DEPENDENT | LEFT_DEPENDENT)
#define SINGLE_DRAWS 0xFFFFF0	// all except walls and platform
#define OCCUPIERS 0x7FFFFFF		// all
#define START_END_CHECKER (OCCUPIERS | HAZARDER_MID_DU | HAZARDER_MID_LR)

#define MID_FIELDS (HAZARDER_MID_DU | FIELD_MID_DU | HAZARDER_MID_LR | FIELD_MID_LR)
#define TRANSMITTERS (CLICK_BUTTON | DOWN_BUTTON | CUBE_BUTTON | SPHERE_BUTTON | LASER_REC_GND | LASER_REC_WALL)
#define RECEIVERS (LASERER | FUNNEL | HAZARDER | FORCE_FIELD | LIGHT_BRIDGE | PLATFORM | BLUE_GELLER | ORANGE_GELLER | WHITE_GELLER | WATER_GELLER)
#define GELS (BLUE_GELLER | ORANGE_GELLER | WHITE_GELLER | WATER_GELLER)
#define WALL_EDGE_OCCUPIERS ((TRANSMITTERS | RECEIVERS | CATAPULT) & ~PLATFORM)


typedef enum OperationType																 
{
	NO_OP	= 0,
	ADD		= 1,
	REMOVE	= 2,
} operationType;

typedef enum CableDir : unsigned short
{
	LR_0 = 0x1 << 0,
	LR_1 = 0x1 << 1,
	LR_2 = 0x1 << 2,
	LR_3 = 0x1 << 3,
	LR_4 = 0x1 << 4,
	LR_5 = 0x1 << 5,
	LR_6 = 0x1 << 6,
	LR_7 = 0x1 << 7,

	DU_0 = 0x1 << 8,
	DU_1 = 0x1 << 9,
	DU_2 = 0x1 << 10,
	DU_3 = 0x1 << 11,
	DU_4 = 0x1 << 12,
	DU_5 = 0x1 << 13,
	DU_6 = 0x1 << 14,
	DU_7 = 0x1 << 15,
} cableDir;

typedef struct GridSquare_S
{
	short x;
	short y;

	GridSquare_S() { }

	GridSquare_S(short x, short y)
	{
		this->x = x;
		this->y = y;
	}

	glm::i16vec2 asGlm() const
	{
		return glm::i16vec2(x, y);
	}
} s_gridSquare;

typedef struct GridData_S
{
	levelObject objectType;
	s_gridData data = { 0 };
} s_levelGridData;

typedef struct ObjectDataGrid
{
	std::vector<int> extraOffsets;
	std::vector<struct Connection*> connections;
	glm::vec4 fullData;
	levelObject objectType;
	levelObject extraType;
	int offset;
	glm::i16vec2 pos;
	glm::i16vec2 extraPos;
	glm::i16vec2 scale;
	glm::i16vec2 extraScale;
	int rotation;
	char platformType;
	char speed;
	bool startActive = true;
	bool startPolarized;
	bool loop;
	bool mirrored;
} s_objectDataGrid;

typedef struct Connection
{
	std::vector<glm::i16vec2> pathPoints;
	std::vector<glm::vec2> renderPathPoints;
	std::vector<int> offsets;
	s_objectDataGrid* transmitter;
	s_objectDataGrid* receiver;
	int index;
	bool changePolarization;
} s_connection;

inline bool operator<(const s_gridSquare lhs, const s_gridSquare rhs)
{
	return (lhs.y < rhs.y) || (lhs.y == rhs.y && lhs.x < rhs.x);
}

inline s_gridSquare operator + (const s_gridSquare& a, const s_gridSquare& b)
{
	return s_gridSquare(a.x + b.x, a.y + b.y);
}

inline bool operator==(const s_gridSquare lhs, const s_gridSquare rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

typedef struct GridOperation
{
	s_gridSquare pos;
    OperationType operationType;
	levelObject objectType;
} s_gridOperation;

typedef struct GridOperation_O
{
	s_objectDataGrid* objData;
    OperationType operationType;
} s_gridOperation_o;

typedef struct GridOperationList
{
	std::vector<s_gridOperation> operations;
	s_gridOperation_o objectOperation = { nullptr, NO_OP };
	s_connection* connection = nullptr;
	GridOperationList* next = nullptr;
	GridOperationList* prev = nullptr;
	glm::i16vec4 region;
} s_gridOperationList;

typedef struct PerTextureData
{
	int offsetStart;
	int currentOffset;
} s_perTextureData;

typedef struct GridItemData
{
	std::string name;
	std::string popName;
	levelObject objectType;
	struct s_texture* tex;
	glm::vec2 gridRenderTranslation;
	glm::ivec2 gridScale;
	glm::vec2 scaleRatio;
	float extraScale;
	int numTextureObjects;
	float gridRenderScale;
	int requiredSideFill;
	bool canRotate;
	bool canMirror;
} s_gridItemData;

class Leveler
{
public:
	static Leveler* object;

	class Game* game;
	class ThinDrawer* thinDrawer;
	class WindowHandler* wh;
	class Overlayer* ol;
	class ManyTexture* mt;
	bool shouldQuit = false;

	//////////////////
	s_gridOperationList* currentOperation;

	//////////////////
	std::vector<std::vector<std::vector<s_gridSquare>>> lastOffsetPos_W;
	std::vector<std::vector<std::vector<s_gridSquare>>> lastOffsetPos_B;
	std::vector<std::vector<std::vector<s_gridSquare>>> lastOffsetPos_G;

	std::vector<std::vector<s_gridSquare>> lastOffsetPos_M;
	std::vector<s_gridSquare> lastOffsetPos_Laser;
	std::vector<s_gridSquare> lastOffsetPos_Bridge;
	std::vector<s_gridSquare> lastOffsetPos_Funnel;
	std::vector<s_gridSquare> lastOffsetPos_Hazard;
	std::vector<s_gridSquare> lastOffsetPos_Field;
	std::vector<s_gridSquare> lastOffsetPos_Platform;
	std::vector<s_connection*> lastOffsetPos_Connection;

	//////////////////
	glm::ivec2 mouseGrid;
	std::vector<std::vector<s_levelGridData>> gridMap;
	std::vector<std::vector<cableDir>> cableMap;
	glm::ivec2 clickStart;
	glm::ivec2 lastGrid;
	int selectedObjectRotation = 0;
	int selectedObjectMirror = 0;
	std::vector<glm::ivec2> consqClickVec;
	bool connecting = false;

	/////////////////
	const short xSize = 400;
	const short ySize = 400;
	const short xSize_H = xSize / 2;
	const short ySize_H = ySize / 2;
	const short extraCable = 5;

	const int numWhiteWall = xSize * ySize;
	const int numBlackWall = xSize * ySize;

	const int perTextureSpace = 1000;
	int textureRenderOffset;
	std::vector<s_perTextureData> perTextureData;

	int numSwapChainImages;

	s_renderPlayerUbo startRenderData {};
	std::set<s_objectDataGrid*> levelObjects;
	s_gridSquare startSquare = { -10, 1 };
	s_gridSquare endSquare = { 10, 1 };

	std::string openLevelName = "";
	std::vector<char> textSizes;	

	Leveler(Game* game);
	
	void renderInit();
	void initOffsets();

	void setSquare(short x, short y, s_levelGridData data);
	s_levelGridData getSquare(short x, short y);
	void setCable(short x, short y, cableDir cblDir);
	cableDir getCable(short x, short y);

	void fillGrid(short minX, short minY, short maxX, short maxY, levelObject fillType);
	void putBarrierOnMemories(std::vector<class ShaderBase*> shaders);
	void afterFillRayCheck(short minX, short minY, short maxX, short maxY);
	void putPlatformOnClick();
	void putPlatform(s_objectDataGrid* inData);
	void putCatapult(std::vector<glm::i16vec2> positions, glm::vec4 oData);
	bool putObject(const s_gridItemData& item, s_objectDataGrid* inData);
	bool extraPutCheck(const s_gridItemData& item, s_objectDataGrid* objectData);
	void afterPutObject(const s_gridItemData& item, s_objectDataGrid* objectData, short scaleX, short scaleY, int rotation);
	void afterRemoveObject(const s_gridItemData& item, s_objectDataGrid* objectData, short scaleX, short scaleY);
	void removeObject(short x, short y);
	void removePlatform(short x, short y);
	void takeBack(bool shiftPressed);
	void addGridSquare_W(short x, short y);
	void addGridSquare_B(short x, short y);
	void addGridSquare_G(short x, short y);
	void addGridSquare_F(short x, short y);
	void addGridSquare_E(short x, short y);
	void addGridSquare(short x, short y, levelObject objectType);

	glm::ivec2 getGridPos(glm::vec2 p);
	bool canConnect(glm::ivec2 p0, glm::ivec2 p1);
	short keepXInBounds(short x);
	short keepYInBounds(short y);
	bool isInBounds(short x, short y);
	bool isInBoundsCable(short x, short y);
	glm::i16vec2 keepInBounds(glm::i16vec2 pos);

	void updateConnectionRenderData(s_connection* connection);
	s_connection* connectObjects(s_objectDataGrid* obj1, s_objectDataGrid* obj2, s_connection* prevConnection, s_objectDataGrid* noAddObject=nullptr);
	void removeConnection(s_connection* connection, s_objectDataGrid* noRemove=nullptr, bool track=true);
	short selectDirs(const glm::i16vec2 p0, const glm::i16vec2 p1, const char inDir, const char cableIndex);
	char getAvailableCableIndices(const glm::i16vec2 p0, const glm::i16vec2 p1);
	void getSimplePath(std::vector<glm::i16vec2>& fillVec, glm::i16vec2 from, glm::i16vec2 to, char cableIndex, char rot);
	void getPath(std::vector<glm::i16vec2>& fillVec, glm::i16vec2 from, glm::i16vec2 to, char cableIndex);
	void sendObjectRay(const s_gridItemData& item, short x, short y, short rotation, short scaleX, short scaleY,
		float startOffset, float endOffset, levelObject objectMask,
		glm::vec2& startPoint, glm::vec2& endPoint);
	void sideUpdate(short minX, short maxX, short minY, short maxY);
	bool checkGridOccupancy(short x, short y);

	void moveVkMem(class ShaderBase* shader, int srcOffset, int destOffset, int size);
	void removeGridSquare(short x, short y);
	void setObjectOperationTrack(operationType opType, s_objectDataGrid* objectData, s_connection* connection=nullptr);
	void update();
	void calcMouseGrid();
	glm::ivec2 getMouseGrid();

	void setRealTimeActivation(s_objectDataGrid* objectData);
	void fillPlatformData(int platformType, s_platformRectUbo* platformUbo);
	glm::i16vec2 getBestVert(glm::i16vec2 checkStart, int desiredSize, levelObject mask); // returns y0, y1
	void handleRightClickClick();

	int saveLevel(std::string fileName, bool overwrite=false); // 0 = success, 1 = can't open file, 2 = file exists
	int openLevel(std::string fileName); // 0 = success, 1 = can't open file, 2 = file exists
	void renderClear();
	void quitLeveler();

	void pushRenderFunctions();
	void createLevel();

	static void renderParticles(VkCommandBuffer cmdBuff, void* ptr);
	static void renderLasers(VkCommandBuffer cmdBuff, void* ptr);
	static void renderBridges(VkCommandBuffer cmdBuff, void* ptr);
	static void renderFunnels(VkCommandBuffer cmdBuff, void* ptr);
	static void renderGrid(VkCommandBuffer cmdBuff, void* ptr);
};