#pragma once

#include <map>

typedef enum LevelObject
{
	WHITE_WALL = 0x1 << 0,
	BLACK_WALL = 0x1 << 1,
} levelObject;

typedef struct GridSquare_S
{
	char x;
	char y;
} s_gridSquare;

inline bool operator<(const s_gridSquare lhs, const s_gridSquare rhs)
{
    if (lhs.x < rhs.x)
        return true;
    else if (lhs.x == rhs.x && lhs.y < rhs.y)
        return true;
    else
        return false;
}

class Leveler
{
public:
	class Game* game;
	class ThinDrawer* thinDrawer;
	std::map<s_gridSquare, levelObject> gridMap;
	Leveler(Game* game);
	~Leveler();
};