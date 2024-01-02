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

            if (!(edgeUsedMap[gridY][gridX] & 0xF) && g->leveler->getSquare(x, y).objectType & (BLACK_WALL | WHITE_WALL))
            {
                bool topOK = false;
                for (int i = 0; i < 4; i++)
                {
                    const s_gridSquare dir = dirs[i];
                    bool ok =   !g->leveler->isInBounds(x + dir.x, y + dir.y) ||
                                (reachableMap[gridY + dir.y][gridX + dir.x] & GBW_REACHABLE);

                    if (ok)
                    {
                        topOK = true;
                        break;
                    }
                }

                topOK &= !g->leveler->isInBounds(x, y + 1) || !(g->leveler->getSquare(x, y + 1).objectType & (BLACK_WALL | WHITE_WALL));

                if (topOK)
                {
                    walkArndAndCreateEdge(x, y);
                }
			}
            else if (!(edgeUsedMap[gridY][gridX] & (0xF << 4)) && g->leveler->getSquare(x, y).objectType & GLASS_WALL)
            {
                bool topOK = false;
                for (int i = 0; i < 4; i++)
                {
                    const s_gridSquare dir = dirs[i];
                    bool ok =   !g->leveler->isInBounds(x + dir.x, y + dir.y) ||
                                !(g->leveler->getSquare(x + dir.x, y + dir.y).objectType & GLASS_WALL);

                    if (ok)
                    {
                        topOK = true;
                        break;
                    }
                }
                
                topOK &= !g->leveler->isInBounds(x, y + 1) || !(g->leveler->getSquare(x, y + 1).objectType & GLASS_WALL);

                if (topOK)
                {
                    walkArndAndCreateEdgeGF(x, y, GLASS_WALL);
                }
            }
            else if (!(edgeUsedMap[gridY][gridX] & (0xF << 8)) && g->leveler->getSquare(x, y).objectType & FENCE_WALL)
            {
                bool topOK = false;
                for (int i = 0; i < 4; i++)
                {
                    const s_gridSquare dir = dirs[i];
                    bool ok =   !g->leveler->isInBounds(x + dir.x, y + dir.y) ||
                                !(g->leveler->getSquare(x + dir.x, y + dir.y).objectType & FENCE_WALL);
                    
                    ok = ok && (reachableMap[gridY][gridX] & GBW_REACHABLE);
                    
                    if (ok)
                    {
                        topOK = true;
                        break;
                    }
                }

                topOK &= !g->leveler->isInBounds(x, y + 1) || !(g->leveler->getSquare(x, y + 1).objectType & FENCE_WALL);

                if (topOK)
                {
                    walkArndAndCreateEdgeGF(x, y, FENCE_WALL);
                }
            }
        }
    }
}

short LevelerTestCase::getDirs(glm::i16vec2 p0, glm::i16vec2 p1, short dir)
{
    short ret = 0;

    s_gridSquare dirVec = dirs[dir];
    s_gridSquare upDirVec = dirs[(dir + 1) % 4];
    glm::i16vec2 p00 = { p0.x - dirVec.x, p0.y - dirVec.y };
    glm::i16vec2 p11 = { p1.x + dirVec.x, p1.y + dirVec.y };

    glm::i16vec2 p00Up = { p00.x + upDirVec.x, p00.y + upDirVec.y };
    glm::i16vec2 p11Up = { p11.x + upDirVec.x, p11.y + upDirVec.y };

    glm::i16vec2 p0Up = { p0.x + upDirVec.x, p0.y + upDirVec.y };
    glm::i16vec2 p1Up = { p1.x + upDirVec.x, p1.y + upDirVec.y };

    bool b0 =   (g->leveler->isInBounds(p00.x, p00.y) && (g->leveler->getSquare(p00.x, p00.y).objectType & 0xF) || 
                g->leveler->isInBounds(p0Up.x, p0Up.y) && (g->leveler->getSquare(p0Up.x, p0Up.y).objectType & 0xF)) &&
                !(g->leveler->isInBounds(p00Up.x, p00Up.y) && (g->leveler->getSquare(p00Up.x, p00Up.y).objectType & 0xF));
    
    bool b1 =   (g->leveler->isInBounds(p11.x, p11.y) && (g->leveler->getSquare(p11.x, p11.y).objectType & 0xF) ||
                g->leveler->isInBounds(p1Up.x, p1Up.y) && (g->leveler->getSquare(p1Up.x, p1Up.y).objectType & 0xF)) &&
                !(g->leveler->isInBounds(p11Up.x, p11Up.y) && (g->leveler->getSquare(p11Up.x, p11Up.y).objectType & 0xF));

    if (b0) ret |= (dir + 2) % 4;
    else ret |= (dir + 3) % 4;

    if (b1) ret |= dir << 8;
    else ret |= ((dir + 3) % 4) << 8;
    
    return ret;
}

void LevelerTestCase::walkArndAndCreateEdge(short x, short y)
{
    char startDir = 1;

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

        short cDirs = getDirs(pos, endPos, startDir);

        b2Vec2 p00 = { p0.x + dirs[cDirs & 0xFF].x, p0.y + dirs[cDirs & 0xFF].y };
        b2Vec2 p11 = { p1.x + dirs[cDirs >> 8].x, p1.y + dirs[cDirs >> 8].y };
        b2Body* body = createEdgeOneSided(p11, p1, p0, p00, g->pWorld);

        //b2Body* body = createEdge(p1, p0, g->pWorld);

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

        glm::i16vec2 fwdPos = pos + dirs[dir].asGlm();
        glm::i16vec2 fwdUpPos = fwdPos + upDir;

        for (int i = 0; i < unitWallPerMeter; i++)
        {
            const s_levelGridData& lgd = g->leveler->getSquare(pos.x, pos.y);
            wallData.push_back({ NONE, (WALL_TYPE)lgd.objectType, 0 });
        }

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
    const float exUp = 0.02f;
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
            glm::i16vec2 fwdDir = dirs[startDir].asGlm();
            glm::i16vec2 upDir = dirs[(startDir + 1) % 4].asGlm();

            bool isUpFree = !g->leveler->isInBounds(pos.x + upDir.x, pos.y + upDir.y) ||
                !(g->leveler->getSquare(pos.x + upDir.x, pos.y + upDir.y).objectType & (WHITE_WALL | BLACK_WALL | GLASS_WALL));
            glm::i16vec2 lastChangePos = pos;
            for (glm::i16vec2 p = pos; ; p += fwdDir)
            {
                bool thisUpFree = !g->leveler->isInBounds(p.x + upDir.x, p.y + upDir.y) ||
                    !(g->leveler->getSquare(p.x + upDir.x, p.y + upDir.y).objectType & (WHITE_WALL | BLACK_WALL | GLASS_WALL));

                if (isUpFree != thisUpFree)
                {
                    b2Vec2 p0 = b2Vec2(lastChangePos.x, lastChangePos.y) + dirAdditionsStart[startDir];
                    b2Vec2 p1 = b2Vec2(p.x, p.y) + dirAdditionsStart[startDir];
                    if (!isUpFree)
                    {
                        p0 -= exUp * b2Vec2(upDir.x, upDir.y); p1 -= exUp * b2Vec2(upDir.x, upDir.y);
                    }

                    short cDirs = getDirs(p, lastChangePos, startDir);
                    b2Vec2 p00 = { p0.x + dirs[cDirs & 0xFF].x, p0.y + dirs[cDirs & 0xFF].y };
                    b2Vec2 p11 = { p1.x + dirs[cDirs >> 8].x, p1.y + dirs[cDirs >> 8].y };
                    b2Body* body = createBasicEdgeOneSided(p11, p1, p0, p00, wallType == GLASS_WALL ? F_GEDGE : F_FEDGE, g->pWorld);

                    //createBasicEdge(p0, p1, wallType == GLASS_WALL ? F_GEDGE : F_FEDGE, g->pWorld);

                    isUpFree = thisUpFree;
                    lastChangePos = p;
                }

                if (p == endPos) break;
            }

            b2Vec2 p0 = b2Vec2(lastChangePos.x, lastChangePos.y) + dirAdditionsStart[startDir];
            b2Vec2 p1 = b2Vec2(endPos.x, endPos.y) + dirAdditionsEnd[startDir];
            if (!isUpFree)
            {
                p0 -= exUp * b2Vec2(upDir.x, upDir.y); p1 -= exUp * b2Vec2(upDir.x, upDir.y);
            }

            short cDirs = getDirs(lastChangePos, endPos, startDir);
            b2Vec2 p00 = { p0.x + dirs[cDirs & 0xFF].x, p0.y + dirs[cDirs & 0xFF].y };
            b2Vec2 p11 = { p1.x + dirs[cDirs >> 8].x, p1.y + dirs[cDirs >> 8].y };
            b2Body* body = createBasicEdgeOneSided(p11, p1, p0, p00, wallType == GLASS_WALL ? F_GEDGE : F_FEDGE, g->pWorld);

            //createBasicEdge(p1, p0, wallType == GLASS_WALL ? F_GEDGE : F_FEDGE, g->pWorld);
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
    glm::i16vec2 fwdDir = dirs[dir].asGlm();

    glm::i16vec2 backPos = pos - fwdDir;
    glm::i16vec2 backPosD = backPos + upDir;
    if ((wallMask & GLASS_WALL) &&
        (!g->leveler->isInBounds(backPos.x, backPos.y) || (reachableMap[backPos.y + g->leveler->ySize_H][backPos.x + g->leveler->xSize_H] & GBW_REACHABLE)) &&
        (g->leveler->isInBounds(backPosD.x, backPosD.y) && (g->leveler->getSquare(backPosD.x, backPosD.y).objectType & (WHITE_WALL | BLACK_WALL))))
    {
        *emptyFound = true;
    }

    while (1)
    {
        edgeUsedMap[pos.y + g->leveler->ySize_H][pos.x + g->leveler->xSize_H] |= (EdgeUse)((int)DOWN_G << (dir + extraShift));

        glm::i16vec2 fwdPos = pos + fwdDir;
        glm::i16vec2 fwdUpPos = fwdPos + upDir;
        glm::i16vec2 upPos = pos + upDir;

        bool fwdEmp = !g->leveler->isInBounds(fwdPos.x, fwdPos.y) || !(g->leveler->getSquare(fwdPos.x, fwdPos.y).objectType & wallMask);
        bool upEmp = !g->leveler->isInBounds(fwdUpPos.x, fwdUpPos.y) || !(g->leveler->getSquare(fwdUpPos.x, fwdUpPos.y).objectType & wallMask);

        if (wallMask == GLASS_WALL)
        {
            *emptyFound |= !g->leveler->isInBounds(upPos.x, upPos.y) || (reachableMap[upPos.y + g->leveler->ySize_H][upPos.x + g->leveler->xSize_H] & GBW_REACHABLE);
        }
        else
        {
            *emptyFound |= !g->leveler->isInBounds(upPos.x, upPos.y) || !(g->leveler->getSquare(upPos.x, upPos.y).objectType & GLASS_WALL);
        }

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

    glm::i16vec2 endNextPos = *endPos + fwdDir;
    glm::i16vec2 endNextPosD = endNextPos + upDir;
    if ((wallMask & GLASS_WALL) &&
        (!g->leveler->isInBounds(endNextPos.x, endNextPos.y) || (reachableMap[endNextPos.y + g->leveler->ySize_H][endNextPos.x + g->leveler->xSize_H] & GBW_REACHABLE)) &&
        (g->leveler->isInBounds(endNextPosD.x, endNextPosD.y) && (g->leveler->getSquare(endNextPosD.x, endNextPosD.y).objectType & (WHITE_WALL | BLACK_WALL))))
    {
        *emptyFound = true;
    }
}

void createRagDoll(PortalWorld* pw, ThinDrawer* td)
{
    // create a 2d ragdoll
    b2Body* body = createBox(b2Vec2(0.0f, 0.0f), b2Vec2(0.15f, 0.35f), pw, b2_dynamicBody, 1.0);
    PortalBody* pbx = pw->createPortalBody(body);
    td->addPortalBody(pbx, {});

    // right arm1
    b2Body* body1 = createBox(b2Vec2(0.3f, 0.0f), b2Vec2(0.3f, 0.075f), pw, b2_dynamicBody, 1.0);
    pbx = pw->createPortalBody(body1);
    td->addPortalBody(pbx, {});
    b2RevoluteJointDef jd;
    jd.Initialize(body, body1, b2Vec2(0.0f, 0.0f));
    jd.localAnchorA.Set(0.1f, 0.1f);
    jd.localAnchorB.Set(-0.25f, 0.0f);
    jd.calculateAnchorHelpers();
    pw->CreateJoint(&jd);

    // right arm2
    b2Body* body2 = createBox(b2Vec2(0.6f, 0.0f), b2Vec2(0.3f, 0.075f), pw, b2_dynamicBody, 1.0);
    pbx = pw->createPortalBody(body2);
    td->addPortalBody(pbx, {});
    jd.Initialize(body1, body2, b2Vec2(0.0f, 0.0f));
    jd.localAnchorA.Set(0.25f, 0.0f);
    jd.localAnchorB.Set(-0.25f, 0.0f);
    jd.calculateAnchorHelpers();
    pw->CreateJoint(&jd);

    // left arm1
    b2Body* body3 = createBox(b2Vec2(-0.3f, 0.0f), b2Vec2(0.3f, 0.075f), pw, b2_dynamicBody, 1.0);
    pbx = pw->createPortalBody(body3);
    td->addPortalBody(pbx, {});
    jd.Initialize(body, body3, b2Vec2(0.0f, 0.0f));
    jd.localAnchorA.Set(-0.1f, 0.1f);
    jd.localAnchorB.Set(0.25f, 0.0f);
    jd.calculateAnchorHelpers();
    pw->CreateJoint(&jd);

    // left arm2
    b2Body* body4 = createBox(b2Vec2(-0.6f, 0.0f), b2Vec2(0.3f, 0.075f), pw, b2_dynamicBody, 1.0);
    pbx = pw->createPortalBody(body4);
    td->addPortalBody(pbx, {});
    jd.Initialize(body3, body4, b2Vec2(0.0f, 0.0f));
    jd.localAnchorA.Set(-0.25f, 0.0f);
    jd.localAnchorB.Set(0.25f, 0.0f);
    jd.calculateAnchorHelpers();
    pw->CreateJoint(&jd);

    // head
    b2Body* body5 = createCircle(b2Vec2(0.0f, 0.5f), 0.3f, pw, b2_dynamicBody, 1.0);
    pbx = pw->createPortalBody(body5);
    td->addPortalBody(pbx, {"sphere.png"});
    jd.Initialize(body, body5, b2Vec2(0.0f, 0.0f));
    jd.localAnchorA.Set(0.0f, 0.5f);
    jd.localAnchorB.Set(0.0f, 0.0f);
    jd.calculateAnchorHelpers();
    pw->CreateJoint(&jd);

    // right leg1
    b2Body* body6 = createBox(b2Vec2(0.1f, -0.5f), b2Vec2(0.075f, 0.3f), pw, b2_dynamicBody, 1.0);
    pbx = pw->createPortalBody(body6);
    td->addPortalBody(pbx, {});
    jd.Initialize(body, body6, b2Vec2(0.0f, 0.0f));
    jd.localAnchorA.Set(0.1f, -0.35f);
    jd.localAnchorB.Set(0.0f, 0.25f);
    jd.calculateAnchorHelpers();
    pw->CreateJoint(&jd);

    // right leg2
    b2Body* body7 = createBox(b2Vec2(0.1f, -0.8f), b2Vec2(0.075f, 0.3f), pw, b2_dynamicBody, 1.0);
    pbx = pw->createPortalBody(body7);
    td->addPortalBody(pbx, {});
    jd.Initialize(body6, body7, b2Vec2(0.0f, 0.0f));
    jd.localAnchorA.Set(0.0f, -0.25f);
    jd.localAnchorB.Set(0.0f, 0.25f);
    jd.calculateAnchorHelpers();
    pw->CreateJoint(&jd);

    // left leg1
    b2Body* body8 = createBox(b2Vec2(-0.1f, -0.5f), b2Vec2(0.075f, 0.3f), pw, b2_dynamicBody, 1.0);
    pbx = pw->createPortalBody(body8);
    td->addPortalBody(pbx, {});
    jd.Initialize(body, body8, b2Vec2(0.0f, 0.0f));
    jd.localAnchorA.Set(-0.1f, -0.35f);
    jd.localAnchorB.Set(0.0f, 0.25f);
    jd.calculateAnchorHelpers();
    pw->CreateJoint(&jd);

    // left leg2
    b2Body* body9 = createBox(b2Vec2(-0.1f, -0.8f), b2Vec2(0.075f, 0.3f), pw, b2_dynamicBody, 1.0);
    pbx = pw->createPortalBody(body9);
    td->addPortalBody(pbx, {});
    jd.Initialize(body8, body9, b2Vec2(0.0f, 0.0f));
    jd.localAnchorA.Set(0.0f, -0.25f);
    jd.localAnchorB.Set(0.0f, 0.25f);
    jd.calculateAnchorHelpers();
    pw->CreateJoint(&jd);
}

void LevelerTestCase::createDynamics()
{
    for (s_objectDataGrid* objectData : g->leveler->levelObjects)
    {
        glm::i16vec2 pos = objectData->pos;

        switch (objectData->objectType)
        {
        case DEFAULT_CUBE:
        {
            DefaultCube* dc = new DefaultCube(g->pWorld);
            dc->pBody->at(0)->body->SetTransform(b2Vec2(pos.x + 0.5f, pos.y + 0.5f), 0.0);
        }
        break;
        case LASER_CUBE:
        {
			LaserCube* lc = new LaserCube(g->pWorld);
			lc->pBody->at(0)->body->SetTransform(b2Vec2(pos.x + 0.5f, pos.y + 0.5f), 0.0);
		}
        break;
        case SPHERE_CUBE:
        {
            Sphere* sp = new Sphere(g->pWorld);
            sp->pBody->at(0)->body->SetTransform(b2Vec2(pos.x + 0.5f, pos.y + 0.5f), 0.0);
        }
        break;
        case TURRET:
        {
            Turret* tr = new Turret(g->pWorld);
            tr->pBody->at(0)->body->SetTransform(b2Vec2(pos.x + 0.5f, pos.y + 0.85f), 0.0);
            tr->pBody->at(0)->isMirrored = objectData->mirrored;
        }
        break;

        default:
            break;
        }
    }

    Cyber* cyb = new Cyber(g->pWorld, b2Vec2(0.0f, 4.0f));

    createRagDoll(g->pWorld, td);
}

void LevelerTestCase::onCreate()
{
    g->pWorld->SetGravity(b2Vec2(0.0f, -10.0f));

    fillReachableMap();

    if (!shouldQuit)
    {
        createDefaultEdges();
    }

    createDynamics();

    /*b2Body* lastBody = createBox(b2Vec2(10.0f, 0.0f), b2Vec2(2.49, 2.49), g->pWorld, b2_dynamicBody, 1.0);
    lastBody->SetBullet(true);

    Portal* portalDown = g->pWorld->createPortal(b2Vec2(10.0f, -2.5f), b2Vec2(0.0f, -1.0f), 2.51f, lastBody);
    Portal* portalUp = g->pWorld->createPortal(b2Vec2(10.0f, +2.5f), b2Vec2(0.0f, +1.0f), 2.51f, lastBody);

    Portal* portalRight = g->pWorld->createPortal(b2Vec2(12.5f, 0.0f), b2Vec2(1.0f, 0.0f), 2.51f, lastBody);
    Portal* portalLeft = g->pWorld->createPortal(b2Vec2(7.5f, 0.0f), b2Vec2(-1.0f, 0.0f), 2.51f, lastBody);

    portalDown->connect(portalUp);
    portalRight->connect(portalLeft, true);*/
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