#include <Portal.h>

typedef struct bodyRenderData{
    b2Color color;
    std::vector<PortalBody*>* worldIndex;
} bodyRenderData;

typedef struct portalRenderData{
    b2Color color;
    Portal* portal;
} portalRenderData;

class Renderer
{

public:
    
    Renderer(PortalWorld* pWorld);

    void addPortalBody(PortalBody* pBody, b2Color color=b2Color(1, 1, 1, 0.5));
    void addPortal(Portal* portal, b2Color color=b2Color(1, 1, 1, 0.5));

    void render();

private:

    PortalWorld* pWorld;

    std::vector<bodyRenderData*> portalBodies;
    std::vector<portalRenderData*> portals;

    void portalRender(portalRenderData* portalData);
    void bodyRender(bodyRenderData* bodyData);

    void portalRender(PortalBody* pBody, b2Fixture* fix, std::vector<b2Vec2>& vertices, b2Color color);
    void drawPolygonFix(PortalBody* pBody, b2Fixture* fix, b2Color color);
    void drawCircleFix(PortalBody* pBody, b2Fixture* fix, b2Color color);
    void drawVertices(PortalBody* pBody, std::vector<b2Vec2>& vertices, b2Color color);
};