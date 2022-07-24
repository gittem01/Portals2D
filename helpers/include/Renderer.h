#pragma once

#include <glad/glad.h>
#include <Portal.h>
#include <Shader.h>

class Camera;
class DebugDrawer;

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

friend class DebugDrawer;

public:
    bool drawReleases;
    b2Color releaseColor;
    
    Renderer(PortalWorld* pWorld, Camera* camera);

    void addPortalBody(PortalBody* pBody, b2Color color=b2Color(1, 1, 1, 0.5));
    void addPortal(Portal* portal, b2Color color=b2Color(1, 1, 1, 0.5));

    void drawArrow(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);

    void debug_Line(b2Vec2 p1, b2Vec2 p2, float thickness=0.01f, b2Color color=b2Color(1, 1, 1, 1));
    void debug_Circle(  b2Vec2 p, float r, float outThickness = 0.01f, b2Color color=b2Color(1, 1, 1, 1),
                         bool isPoint=false, b2Color outerColor=b2Color(1, 1, 1, 1));
    void debug_Polygon( const b2Vec2* vertices, int vertexCount, float outThickness = 0.01f, b2Color color = b2Color(1, 1, 1, 1),
                        b2Color outerColor = b2Color(1, 1, 1, 1));

    void render();

private:

    // -0.5f 0.5f box VAO
    unsigned int VAO;
    Shader* polyShader;
    Shader* circShader;
    Shader* lineShader;
    Camera* camera;

    PortalWorld* pWorld;

    std::vector<bodyRenderData*> portalBodies;
    std::vector<portalRenderData*> portals;

    void portalRender(portalRenderData* portalData);
    void bodyRender(bodyRenderData* bodyData);

    void drawPolygonFix(PortalBody* pBody, b2Fixture* fix, b2Color color);
    void drawCircleFix(PortalBody* pBody, b2Fixture* fix, b2Color color);
};