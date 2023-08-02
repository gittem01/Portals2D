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

typedef enum LevelObject
{
	NONE_LO = 0x0,
	WHITE_WALL = 0x1 << 0,
	BLACK_WALL = 0x1 << 1,
	DEFAULT_CUBE = 0x1 << 2,
} levelObject;

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

class Leveler
{
public:
	class Game* game;
	class ThinDrawer* thinDrawer;
	class WindowHandler* wh;
	class Overlayer* ol;

	//////////////////
	std::vector<s_gridSquare> lastOffsetPos_W;
	std::vector<s_gridSquare> lastOffsetPos_B;

	//////////////////
	std::vector<std::vector<s_levelGridData>> gridMap;
	int clickStart[2];

	short xSize = 1000;
	short ySize = 1000;
	short xSize_H = xSize / 2;
	short ySize_H = ySize / 2;

	int numWhiteWall = xSize * ySize;
	int numBlackWall = xSize * ySize;

	Leveler(Game* game);
	~Leveler();

	void initOffsets();

	void setSquare(short x, short y, s_levelGridData data);
	s_levelGridData getSquare(short x, short y);

	void fillGrid(short minX, short minY, short maxX, short maxY, levelObject fillType);
	void addGridSquare_W(short x, short y);
	void addGridSquare_B(short x, short y);

	void removeGridSquare(short x, short y);

	void update();
	glm::ivec2 getMouseGrid();

	static void renderParticles(VkCommandBuffer cmdBuff, void* ptr);
	static void renderGrid(VkCommandBuffer cmdBuff, void* ptr);
};