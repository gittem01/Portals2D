#pragma once

#include <vector>
#include <set>
#include <string>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

typedef union GridData
{
	void* ptr;
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
	DEFAULT_CUBE	= 0x1ll << 4,
	DOWN_BUTTON		= 0x1ll << 5,
	LASERER			= 0x1ll << 6,
	FUNNEL			= 0x1ll << 7,
	HAZARDER		= 0x1ll << 8,
	FORCE_FIELD		= 0x1ll << 9,
	CATAPULT		= 0x1ll << 10,
	LIGHT_BRIDGE	= 0x1ll << 11,
	PLATFORM		= 0x1ll << 12,

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

constexpr int getLevelObjectIndex(levelObject value)
{
	switch (value)
	{
		case WHITE_WALL:
			return 0;
		case BLACK_WALL:
			return 1;
		case GLASS_WALL:
			return 2;
		case FENCE_WALL:
			return 3;
		case DEFAULT_CUBE:
			return 4;
		case DOWN_BUTTON:
			return 5;
		case LASERER:
			return 6;
		case FUNNEL:
			return 7;
		case HAZARDER:
			return 8;
		case FORCE_FIELD:
			return 9;
		case CATAPULT:
			return 10;
		case LIGHT_BRIDGE:
			return 11;
		case PLATFORM:
			return 12;
	default:
		return -1;
	}
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
#define SINGLE_DRAWS (DEFAULT_CUBE | DOWN_BUTTON | LASERER | FUNNEL | HAZARDER | FORCE_FIELD | CATAPULT | LIGHT_BRIDGE)
#define OCCUPIERS (WHITE_WALL | BLACK_WALL | GLASS_WALL | FENCE_WALL | DEFAULT_CUBE | DOWN_BUTTON | LASERER | PLATFORM | FUNNEL | HAZARDER | FORCE_FIELD | CATAPULT | LIGHT_BRIDGE)
#define MID_FIELDS (HAZARDER_MID_DU | FIELD_MID_DU | HAZARDER_MID_LR | FIELD_MID_LR)

typedef enum OperationType
{
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

	glm::i16vec2 asGlm() const
	{
		return glm::i16vec2(x, y);
	}

} s_gridSquare;

typedef struct GridData_S
{
	levelObject objectType;
	s_gridData data;
} s_levelGridData;

typedef struct ObjectDataGrid
{
	glm::vec4 fullData;
	std::vector<int> extraOffsets;
	levelObject objectType;
	levelObject extraType;
	int offset;
	glm::i16vec2 pos;
	glm::i16vec2 extraPos;
	glm::i16vec2 scale;
	glm::i16vec2 extraScale;
	int rotation;
} s_objectDataGrid;

inline bool operator<(const s_gridSquare lhs, const s_gridSquare rhs)
{
	return (lhs.y < rhs.y) || (lhs.y == rhs.y && lhs.x < rhs.x);
}

inline bool operator==(const s_gridSquare lhs, const s_gridSquare rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

typedef struct GridOperation
{
	s_gridSquare pos;
	operationType operationType;
	levelObject objectType;
} s_gridOperation;

typedef struct GridOperation_O
{
	s_objectDataGrid* objData;
	operationType operationType;
} s_gridOperation_o;

typedef struct GridOperationList
{
	std::vector<s_gridOperation> operations;
	s_gridOperation_o* objectOperation = nullptr;
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
	levelObject objectType;
	VkDescriptorSet texSet;
	glm::vec2 gridRenderTranslation;
	glm::ivec2 gridScale;
	float extraScale;
	int numTextureObjects;
	float gridRenderScale;
	int requiredSideFill;
	bool canRotate;
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

	//////////////////
	glm::ivec2 mouseGrid;
	std::vector<std::vector<s_levelGridData>> gridMap;
	std::vector<std::vector<cableDir>> cableMap;
	glm::ivec2 clickStart;
	glm::ivec2 lastGrid;
	int selectedObjectRotation = 0;
	std::vector<glm::ivec2> consqClickVec;

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

	std::set<s_objectDataGrid*> levelObjects;

	Leveler(Game* game);
	~Leveler();

	void initOffsets();

	void setSquare(short x, short y, s_levelGridData data);
	s_levelGridData getSquare(short x, short y);
	void setCable(short x, short y, cableDir cblDir);
	cableDir getCable(short x, short y);

	void fillGrid(short minX, short minY, short maxX, short maxY, levelObject fillType);
	void putBarrierOnMemories(std::vector<class ShaderBase*> shaders);
	void afterFillRayCheck(short minX, short minY, short maxX, short maxY);
	void putPlatformOnClick();
	s_objectDataGrid* putPlatform(s_gridSquare pos, s_gridSquare size, int pSize, int rotation);
	void putCatapult(std::vector<glm::i16vec2> positions, glm::vec4 oData);
	s_objectDataGrid* putObject(const s_gridItemData& item, s_objectDataGrid* inData);
	s_objectDataGrid* extraPutCheck(const s_gridItemData& item, short startX, short startY, short scaleX, short scaleY, int rotation);
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

	short keepXInBounds(short x);
	short keepYInBounds(short y);
	bool isInBounds(short x, short y);
	bool isInBoundsCable(short x, short y);
	glm::i16vec2 keepInBounds(glm::i16vec2 pos);

	short selectDirs(const glm::i16vec2 p0, const glm::i16vec2 p1, const char inDir, const char cableIndex);
	void getPath(std::vector<glm::i16vec2>& fillVec, glm::i16vec2 from, glm::i16vec2 to, char cableIndex);
	void sendObjectRay(const s_gridItemData& item, short x, short y, short rotation, short scaleX, short scaleY,
		float startOffset, float endOffset, levelObject objectMask,
		glm::vec2& startPoint, glm::vec2& endPoint);
	void sideUpdate(short minX, short maxX, short minY, short maxY);
	bool checkGridOccupancy(short x, short y);

	void moveVkMem(class ShaderBase* shader, int srcOffset, int destOffset, int size);
	void removeGridSquare(short x, short y);
	void setObjectOperationTrack(operationType opType, s_objectDataGrid* objectData);
	void update();
	void calcMouseGrid();
	glm::ivec2 getMouseGrid();

	static void renderParticles(VkCommandBuffer cmdBuff, void* ptr);
	static void renderLasers(VkCommandBuffer cmdBuff, void* ptr);
	static void renderBridges(VkCommandBuffer cmdBuff, void* ptr);
	static void renderFunnels(VkCommandBuffer cmdBuff, void* ptr);
	static void renderGrid(VkCommandBuffer cmdBuff, void* ptr);
};