#pragma once

#include <box2d/box2d.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

class DebugDrawer : public b2Draw
{
public:
    void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
    void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
    void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) { printf("circle\n"); }
    void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color);
    void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
    void DrawTransform(const b2Transform& xf){ printf("transform\n"); }
    void DrawPoint(const b2Vec2& p, float size, const b2Color& color);

    void drawWorld(b2World* world);
    void drawShape(b2Fixture* fixture, const b2Transform& xf, const b2Color& color);
};