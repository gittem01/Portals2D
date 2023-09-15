#include <BaseTestCase.h>
#include <GameBase.h>
#include <DefaultObjects/DefaultCube.h>

class LevelerTestCase : public BaseTestCase
{
public:
    std::vector<std::vector<bool>> reachableMap;
    std::vector<std::vector<bool>> edgeUsedMap;

    LevelerTestCase(Game* game);

    void fillReachableMap();
    void createDefaultEdges();
    void walkArndAndCreateEdge(short x, short y);
    void walkFwrdAndCreateEdge(glm::i16vec2 pos, char dir, glm::i16vec2* nextPos, glm::i16vec2* endPos, char* nextDir, std::vector<s_unitWallData>& wallData);

    void perFrameUpdate();
    void onUpdate();
    void onCreate();
};
