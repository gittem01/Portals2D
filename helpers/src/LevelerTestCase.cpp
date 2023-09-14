#include <BaseTestCase.h>
#include <GameBase.h>
#include <LevelerTestCase.h>
#include <Game.h>
#include <TestPlayer.h>
#include <Leveler.h>

s_gridSquare dirs[4] = { { 0, -1 }, { +1, 0 }, { 0, +1 }, { -1, 0 } };

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
    std::vector<std::vector<bool>> visitedMap;

    reachableMap.resize(g->leveler->gridMap.size());
    visitedMap.resize(reachableMap.size());
    for (int i = 0; i < reachableMap.size(); i++)
    {
        reachableMap[i].resize(g->leveler->gridMap[0].size());
        visitedMap[i].resize(g->leveler->gridMap[0].size());
    }

    s_gridSquare startSquare = g->leveler->startSquare;
    s_gridSquare endSquare = g->leveler->endSquare;

    std::vector<s_gridSquare> nextVisitSquares[2];
    int nextVisitIndex = 0;
    nextVisitSquares[0].reserve(1024);
    nextVisitSquares[1].reserve(1024);

    nextVisitSquares[0].push_back(startSquare);

    while (nextVisitSquares[nextVisitIndex].size() > 0)
    {
        for (s_gridSquare sq : nextVisitSquares[nextVisitIndex])
        {
            for (int i = 0; i < 4; i++)
            {
                s_gridSquare nextSquare = sq + dirs[i];
                if (g->leveler->isInBounds(nextSquare.x, nextSquare.y) && !visitedMap[nextSquare.y + g->leveler->ySize_H][nextSquare.x + g->leveler->xSize_H] &&
                    !(g->leveler->getSquare(nextSquare.x, nextSquare.y).objectType & (WHITE_WALL | BLACK_WALL)))
                {
                    reachableMap[nextSquare.y + g->leveler->ySize_H][nextSquare.x + g->leveler->xSize_H] = true;
                    nextVisitSquares[nextVisitIndex ^ 1].push_back(nextSquare);

                    visitedMap[nextSquare.y + g->leveler->ySize_H][nextSquare.x + g->leveler->xSize_H] = true;
                }
            }
        }

        nextVisitSquares[nextVisitIndex].clear();
        nextVisitIndex = nextVisitIndex ^ 1;
    }

    int countReachable = 0;
    for (int i = 0; i < reachableMap.size(); i++)
    {
        for (int j = 0; j < reachableMap[i].size(); j++)
        {
            if (reachableMap[i][j])
            {
				countReachable++;
			}
		}
    }

    printf("Reachable squares: %d\n", countReachable);
}

void LevelerTestCase::onCreate()
{
    fillReachableMap();

    g->pWorld->SetGravity(b2Vec2(0.0f, -12.5f));

    createEdge(b2Vec2(-10.0f, -10.0f), b2Vec2(-10.0f, 10.0f), g->pWorld);
    createEdge(b2Vec2(+40.0f, +10.0f), b2Vec2(+40.0f, -10.0f), g->pWorld);
    createEdge(b2Vec2(+40.0f, -10.0f), b2Vec2(-10.0f, -10.0f), g->pWorld);
    createEdge(b2Vec2(-10.0f, +10.0f), b2Vec2(+40.0f, 10.0f), g->pWorld);
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