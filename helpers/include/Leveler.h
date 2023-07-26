#pragma once

#include <map>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

typedef enum LevelObject
{
	WHITE_WALL = 0x1 << 0,
	BLACK_WALL = 0x1 << 1,
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
	void* data;
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

	std::map<s_gridSquare, s_levelGridData> gridMap;
	int clickStart[2];

	Leveler(Game* game);
	~Leveler();

	static void renderParticles(VkCommandBuffer cmdBuff, void* ptr);
	static void renderGrid(VkCommandBuffer cmdBuff, void* ptr);

	void update();
};