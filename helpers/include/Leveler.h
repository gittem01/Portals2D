#pragma once

#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

typedef union GridData
{
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

typedef enum LevelObject
{
	NONE_LO			= 0x0,
	WHITE_WALL		= 0x1 << 0,
	BLACK_WALL		= 0x1 << 1,
	GLASS_WALL		= 0x1 << 2,
	FENCE_WALL		= 0x1 << 3,
	DEFAULT_CUBE	= 0x1 << 4,
	DOWN_BUTTON		= 0x1 << 5,
	ERASER			= 0x1 << 20,
} levelObject;

#define GRID_DRAWS (WHITE_WALL | BLACK_WALL | GLASS_WALL | FENCE_WALL | ERASER)
#define SINGLE_DRAWS (DEFAULT_CUBE | DOWN_BUTTON)
#define OCCUPIERS (WHITE_WALL | BLACK_WALL | GLASS_WALL | FENCE_WALL | DEFAULT_CUBE | DOWN_BUTTON)

typedef enum OperationType
{
	ADD		= 0,
	REMOVE	= 1,
} operationType;

typedef struct WallData
{
	int offset;
}s_levelWallData;

typedef struct GridSquare_S
{
	short x;
	short y;
} s_gridSquare;

typedef struct GridData_S
{
	levelObject objectType;
	s_gridData data;
} s_levelGridData;

inline bool operator<(const s_gridSquare lhs, const s_gridSquare rhs)
{
    if (lhs.y < rhs.y)
        return true;
    else if (lhs.y == rhs.y && lhs.x < rhs.x)
        return true;
    else
        return false;
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

typedef struct GridOperationList
{
	std::vector<s_gridOperation> operations;
	GridOperationList* next = nullptr;
	GridOperationList* prev = nullptr;
	glm::i16vec4 region;
} s_gridOperationList;

typedef struct PerTextureData
{
	int offsetStart;
	int currentOffset;
} s_perTextureData;

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
	s_gridOperationList* operationList;
	s_gridOperationList* currentOperation;
	int numOperations = 0;

	//////////////////
	std::vector<std::vector<std::vector<s_gridSquare>>> lastOffsetPos_W;
	std::vector<std::vector<std::vector<s_gridSquare>>> lastOffsetPos_B;
	std::vector<std::vector<std::vector<s_gridSquare>>> lastOffsetPos_G;

	//////////////////
	std::vector<std::vector<s_levelGridData>> gridMap;
	glm::ivec2 clickStart;
	glm::ivec2 lastGrid;
	int selectedObjectRotation = 0;

	/////////////////
	const short xSize = 400;
	const short ySize = 400;
	const short xSize_H = xSize / 2;
	const short ySize_H = ySize / 2;

	const int numWhiteWall = xSize * ySize;
	const int numBlackWall = xSize * ySize;

	const int perTextureSpace = 400;
	int textureRenderOffset;
	std::vector<s_perTextureData> perTextureData;

	int numSwapChainImages;

	Leveler(Game* game);
	~Leveler();

	void initOffsets();

	void setSquare(short x, short y, s_levelGridData data);
	s_levelGridData getSquare(short x, short y);

	void fillGrid(short minX, short minY, short maxX, short maxY, levelObject fillType);
	void putObject();
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
	glm::i16vec2 keepInBounds(glm::i16vec2 pos);

	void sideUpdate(short minX, short maxX, short minY, short maxY);
	bool checkGridOccupancy(short x, short y);

	void removeGridSquare(short x, short y);

	void update();
	glm::ivec2 getMouseGrid();

	static void renderParticles(VkCommandBuffer cmdBuff, void* ptr);
	static void renderGrid(VkCommandBuffer cmdBuff, void* ptr);
};