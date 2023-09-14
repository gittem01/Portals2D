#include <BaseTestCase.h>
#include <GameBase.h>
#include <DefaultObjects/DefaultCube.h>

class LevelerTestCase : public BaseTestCase
{
public:
    std::vector<std::vector<bool>> reachableMap;

    LevelerTestCase(Game* game);

    void fillReachableMap();

    void perFrameUpdate();
    void onUpdate();
    void onCreate();
};
