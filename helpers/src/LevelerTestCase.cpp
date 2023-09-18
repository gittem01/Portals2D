#include <BaseTestCase.h>
#include <GameBase.h>
#include <LevelerTestCase.h>
#include <Game.h>
#include <TestPlayer.h>

s_gridSquare dirs[4] = { { 0, -1 }, { +1, 0 }, { 0, +1 }, { -1, 0 } };
b2Vec2 dirAdditionsStart[4] =   { { 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } };
b2Vec2 dirAdditionsEnd[4] =     { { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f } };

LevelerTestCase::LevelerTestCase(Game* game) : BaseTestCase(game)
{
    testCaseItem = 8;

    beforeCreate(false);

    tp = new TestPlayer(game, b2Vec2(game->leveler->startSquare.x + 0.5, game->leveler->startSquare.y + 0.75));

    onCreate();
    afterCreate();
}

void LevelerTestCase::fillReachableMap()
{
    reachableMap.resize(g->leveler->gridMap.size());
    for (int i = 0; i < reachableMap.size(); i++)
    {
        reachableMap[i].resize(g->leveler->gridMap[0].size());
    }

    s_gridSquare startSquare = g->leveler->startSquare;
    s_gridSquare endSquare = g->leveler->endSquare;

    int nextVisitIndex = 0;
    nextVisitSquares[0].reserve(1024);
    nextVisitSquares[1].reserve(1024);

    for (s_objectDataGrid* object : g->leveler->levelObjects)
    {
        glm::i16vec2 sq = object->pos;
        if (reachableMap[sq.y + g->leveler->ySize_H][sq.x + g->leveler->xSize_H] & BW_REACHABLE)
        {
            continue;
        }

        reachableFillFrom({ sq.x, sq.y }, (LevelObject)(WHITE_WALL | BLACK_WALL), BW_REACHABLE);
    }

    reachableFillFrom(startSquare, (LevelObject)(WHITE_WALL | BLACK_WALL), BW_REACHABLE);

    s_gridSquare es = g->leveler->endSquare;
    if (!reachableMap[es.y + g->leveler->ySize_H][es.x + g->leveler->xSize_H] || !reachableMap[es.y + g->leveler->ySize_H + 1][es.x + g->leveler->xSize_H])
    {
		shouldQuit = true;
        return;
	}

    for (s_objectDataGrid* object : g->leveler->levelObjects)
    {
        glm::i16vec2 sq = object->pos;
        if (reachableMap[sq.y + g->leveler->ySize_H][sq.x + g->leveler->xSize_H] & GBW_REACHABLE)
        {
            continue;
        }

        reachableFillFrom(startSquare, (LevelObject)(WHITE_WALL | BLACK_WALL | GLASS_WALL), GBW_REACHABLE);
    }

    reachableFillFrom(startSquare, (LevelObject)(WHITE_WALL | BLACK_WALL | GLASS_WALL), GBW_REACHABLE);
}

void LevelerTestCase::reachableFillFrom(s_gridSquare from, LevelObject mask, ReachableType reachFiller)
{
    nextVisitSquares[0].push_back(from);

    int nextVisitIndex = 0;
    while (nextVisitSquares[nextVisitIndex].size() > 0)
    {
        for (s_gridSquare sq : nextVisitSquares[nextVisitIndex])
        {
            for (int i = 0; i < 4; i++)
            {
                s_gridSquare nextSquare = sq + dirs[i];
                if (g->leveler->isInBounds(nextSquare.x, nextSquare.y) &&
                    !(reachableMap[nextSquare.y + g->leveler->ySize_H][nextSquare.x + g->leveler->xSize_H] & reachFiller) &&
                    !(g->leveler->getSquare(nextSquare.x, nextSquare.y).objectType & mask))
                {
                    reachableMap[nextSquare.y + g->leveler->ySize_H][nextSquare.x + g->leveler->xSize_H] |= reachFiller;
                    nextVisitSquares[nextVisitIndex ^ 1].push_back(nextSquare);
                }
            }
        }

        nextVisitSquares[nextVisitIndex].clear();
        nextVisitIndex = nextVisitIndex ^ 1;
    }
}

void LevelerTestCase::createDefaultEdges()
{
    edgeUsedMap.resize(g->leveler->gridMap.size());
    for (int i = 0; i < reachableMap.size(); i++)
    {
        edgeUsedMap[i].resize(g->leveler->gridMap[0].size());
    }

    for (int y = g->leveler->ySize_H - 1; y >= -g->leveler->ySize_H; y--)
    {
        for (int x = -g->leveler->xSize_H; x < g->leveler->xSize_H; x++)
        {
            int gridX = x + g->leveler->xSize_H;
            int gridY = y + g->leveler->ySize_H;

            if (!(edgeUsedMap[gridY][gridX] & RIGHT_W) && g->leveler->getSquare(x, y).objectType & (BLACK_WALL | WHITE_WALL))
            {
                bool topOK = !g->leveler->isInBounds(x, y + 1) || !(g->leveler->getSquare(x, y + 1).objectType & (BLACK_WALL | WHITE_WALL));
                topOK &= gridY + 1 < g->leveler->ySize && (reachableMap[gridY + 1][gridX] & BW_REACHABLE);

                if (topOK)
                {
                    walkArndAndCreateEdge(x, y);
                }
			}
            else if (!(edgeUsedMap[gridY][gridX] & RIGHT_G) && g->leveler->getSquare(x, y).objectType & GLASS_WALL)
            {
                bool topOK = !g->leveler->isInBounds(x, y + 1) || !(g->leveler->getSquare(x, y + 1).objectType & GLASS_WALL);

                if (topOK)
                {
                    walkArndAndCreateEdgeGF(x, y, GLASS_WALL);
                }
            }
            else if (!(edgeUsedMap[gridY][gridX] & RIGHT_F) && g->leveler->getSquare(x, y).objectType & FENCE_WALL)
            {
                bool topOK = !g->leveler->isInBounds(x, y + 1) || !(g->leveler->getSquare(x, y + 1).objectType & FENCE_WALL);

                if (topOK)
                {
                    walkArndAndCreateEdgeGF(x, y, FENCE_WALL);
                }
            }
        }
    }
}

void LevelerTestCase::walkArndAndCreateEdge(short x, short y)
{
    char startDir = 1;
    char endDir = 2;

    glm::i16vec2 pos = { x, y };
    while (1)
    {
        glm::i16vec2 nextPos;
        glm::i16vec2 endPos;
        char nextDir;
        std::vector<s_unitWallData> wallData;
        walkFwrdAndCreateEdge(pos, startDir, &nextPos, &endPos, &nextDir, wallData);

        b2Vec2 p0 = b2Vec2(pos.x, pos.y) + dirAdditionsStart[startDir];
        b2Vec2 p1 = b2Vec2(endPos.x, endPos.y) + dirAdditionsEnd[startDir];
        b2Body* body = createEdge(p1, p0, g->pWorld);

        fixtureData* fixData = (fixtureData*)body->GetFixtureList()->GetUserData().pointer;
        s_edgeLocalData* eLocalData = (s_edgeLocalData*)fixData->data;
        s_edgeGlobalData* eGlobalData = (s_edgeGlobalData*)eLocalData->globalData;

        assert(eGlobalData->unitWalls.size() == wallData.size());

        std::reverse_copy(wallData.begin(), wallData.end(), eGlobalData->unitWalls.begin());

        if (nextPos.x == x && nextPos.y == y && nextDir == 1)
        {
            break;
        }

        pos = nextPos;
        startDir = nextDir;
    }
}

void LevelerTestCase::walkFwrdAndCreateEdge(glm::i16vec2 pos, char dir, glm::i16vec2* nextPos, glm::i16vec2* endPos, char* nextDir, std::vector<s_unitWallData>& wallData)
{
    glm::i16vec2 upDir = dirs[(dir + 1) % 4].asGlm();

    while (1)
    {
        edgeUsedMap[pos.y + g->leveler->ySize_H][pos.x + g->leveler->xSize_H] |= (EdgeUse)((int)DOWN_W << dir);

        for (int i = 0; i < unitWallPerMeter; i++)
        {
            const s_levelGridData& lgd = g->leveler->getSquare(pos.x, pos.y);
            wallData.push_back({NONE, (WALL_TYPE)lgd.objectType, 0 });
        }

        glm::i16vec2 fwdPos = pos + dirs[dir].asGlm();
        glm::i16vec2 fwdUpPos = fwdPos + upDir;

        bool fwdEmp = !g->leveler->isInBounds(fwdPos.x, fwdPos.y) || !(g->leveler->getSquare(fwdPos.x, fwdPos.y).objectType & (BLACK_WALL | WHITE_WALL));
        bool upEmp = !g->leveler->isInBounds(fwdUpPos.x, fwdUpPos.y) || !(g->leveler->getSquare(fwdUpPos.x, fwdUpPos.y).objectType & (BLACK_WALL | WHITE_WALL));

        if (fwdEmp || (!upEmp && !fwdEmp))
        {
            *endPos = pos;

            if (fwdEmp)
            {
                *nextPos = pos;
                *nextDir = (dir + 3) % 4;
            }
            else
            {
                *nextPos = fwdUpPos;
				*nextDir = (dir + 1) % 4;
			}

            break;
        }

        pos = fwdPos;
    }
}

void LevelerTestCase::walkArndAndCreateEdgeGF(short x, short y, LevelObject wallType)
{
    char startDir = 1;
    char endDir = 2;

    glm::i16vec2 pos = { x, y };
    while (1)
    {
        glm::i16vec2 nextPos;
        glm::i16vec2 endPos;
        char nextDir;
        bool emptyFound = false;
        walkFwrdAndCreateEdgeGF(pos, startDir, wallType, GBW_REACHABLE,
            wallType == GLASS_WALL ? 0 : 4, &nextPos, &endPos, &nextDir, &emptyFound);

        if (emptyFound)
        {
            b2Vec2 p0 = b2Vec2(pos.x, pos.y) + dirAdditionsStart[startDir];
            b2Vec2 p1 = b2Vec2(endPos.x, endPos.y) + dirAdditionsEnd[startDir];
            createBasicEdge(p1, p0, wallType == GLASS_WALL ? F_GEDGE : F_FEDGE, g->pWorld);
        }

        if (nextPos.x == x && nextPos.y == y && nextDir == 1)
        {
            break;
        }

        pos = nextPos;
        startDir = nextDir;
    }
}

void LevelerTestCase::walkFwrdAndCreateEdgeGF(  glm::i16vec2 pos, char dir, LevelObject wallMask, ReachableType empMask, int extraShift,
                                                glm::i16vec2* nextPos, glm::i16vec2* endPos, char* nextDir, bool* emptyFound)
{
    glm::i16vec2 upDir = dirs[(dir + 1) % 4].asGlm();

    while (1)
    {
        edgeUsedMap[pos.y + g->leveler->ySize_H][pos.x + g->leveler->xSize_H] |= (EdgeUse)((int)DOWN_G << (dir + extraShift));

        glm::i16vec2 fwdPos = pos + dirs[dir].asGlm();
        glm::i16vec2 fwdUpPos = fwdPos + upDir;
        glm::i16vec2 upPos = pos + upDir;

        bool fwdEmp = !g->leveler->isInBounds(fwdPos.x, fwdPos.y) || !(g->leveler->getSquare(fwdPos.x, fwdPos.y).objectType & wallMask);
        bool upEmp = !g->leveler->isInBounds(fwdUpPos.x, fwdUpPos.y) || !(g->leveler->getSquare(fwdUpPos.x, fwdUpPos.y).objectType & wallMask);

        *emptyFound |= !g->leveler->isInBounds(upPos.x, upPos.y) || (reachableMap[upPos.y + g->leveler->ySize_H][upPos.x + g->leveler->xSize_H] & GBW_REACHABLE);

        if (fwdEmp || (!upEmp && !fwdEmp))
        {
            *endPos = pos;

            if (fwdEmp)
            {
                *nextPos = pos;
                *nextDir = (dir + 3) % 4;
            }
            else
            {
                *nextPos = fwdUpPos;
                *nextDir = (dir + 1) % 4;
            }

            break;
        }

        pos = fwdPos;
    }
}

void LevelerTestCase::onCreate()
{
    g->pWorld->SetGravity(b2Vec2(0.0f, -10.0f));

    fillReachableMap();

    if (!shouldQuit)
    {
        createDefaultEdges();
    }
}

void LevelerTestCase::perFrameUpdate()
{
    if (g->thinDrawer->wh->keyData[GLFW_KEY_ESCAPE] == 2)
    {
        shouldQuit = true;
    }
}

void LevelerTestCase::onUpdate()
{
    BaseTestCase::onUpdate();

    
}