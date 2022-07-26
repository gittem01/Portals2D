#pragma once

#include <glad/glad.h>
#include <Portal.h>
#include <Shader.h>
#include <random>

class Camera;
class DebugDrawer;

typedef enum shapeType{
    POLYGON,
    CIRCLE,
} shapeType;

typedef struct fixData{
    shapeType type;
    b2Vec2 vertices[8];
    b2Vec2 cPos;

    b2Vec2 mirroredVertices[8];
    b2Vec2 mirroredCPos;

    int vertexCount;
    float radius;
} fixData;

typedef struct bodyRenderData{
    b2Color color;
    std::vector<PortalBody*>* worldIndex;
    std::vector<fixData*> fixtureDatas;
    unsigned int texture;
    glm::vec2 xSides;
    glm::vec2 ySides;

    glm::vec2 mirroredXSides;
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

    void addPortalBody(PortalBody* pBody, b2Color color=b2Color(1, 1, 1, 0.5), const char* texture=NULL);
    void addPortal(Portal* portal, b2Color color=b2Color(1, 1, 1, 0.5));

    void drawArrow(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);

    void debug_Line(b2Vec2 p1, b2Vec2 p2, float thickness=0.01f, b2Color color=b2Color(1, 1, 1, 1));
    void debug_Circle(  b2Vec2 p, float r, float outThickness = 0.01f, b2Color color=b2Color(1, 1, 1, 1),
                         bool isPoint=false, b2Color outerColor=b2Color(1, 1, 1, 1));
    void debug_Polygon( const b2Vec2* vertices, int vertexCount, float outThickness = 0.01f, b2Color color = b2Color(1, 1, 1, 1),
                        b2Color outerColor = b2Color(1, 1, 1, 1));

    void render();

    void dotRender();

private:

    // -0.5f +0.5f box VAO
    unsigned int VAO;
    
    unsigned int pos_SBO;
    unsigned int size_SBO;
    unsigned int color_SBO;
    unsigned int mult_SBO;

    Shader* polyShader;
    Shader* circShader;
    Shader* lineShader;
    Shader* dotShader;

    Camera* camera;

    PortalWorld* pWorld;

    int numDots = 40000;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec2> positions;
    std::vector<glm::vec2> sizes;
    std::vector<glm::vec3> mults;

    int win_width, win_height;
    float minLineThck = 0.0f;

    std::vector<bodyRenderData*> portalBodies;
    std::vector<portalRenderData*> portals;

    void prepareDots();
    void createVAO();

    void portalRender(portalRenderData* portalData);
    void bodyRender(bodyRenderData* bodyData);

    void drawPolygonFix(PortalBody* pBody, b2Fixture* fix, bodyRenderData* brd, int renderIndex);
    void drawCircleFix(PortalBody* pBody, b2Fixture* fix, bodyRenderData* brd, int renderIndex);
};