#include "DebugDrawer.h"
#include <Camera.h>

DebugDrawer::DebugDrawer(Renderer* renderer) : b2Draw(){
	this->renderer = renderer;
}

void DebugDrawer::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color){
	
}

void DebugDrawer::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
	
}

void DebugDrawer::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color){
	renderer->debug_Circle(center, radius, 0.1f, color);
}

void DebugDrawer::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {
	renderer->debug_Line(p1, p2, 0.01f, color);
}

void DebugDrawer::DrawPoint(const b2Vec2& p, float size, const b2Color& color) {
	renderer->debug_Circle(p, size * 0.01f, 0.0f, color, true);
}

void DebugDrawer::DrawTransform(const b2Transform& xf){
	const float k_axisScale = 0.3f / (renderer->camera->zoom * renderer->camera->zoom);
	
	b2Vec2 p1 = xf.p, p2;

	p2 = p1 + k_axisScale * xf.q.GetXAxis();
	renderer->debug_Line(p1, p2, 0.01f, b2Color(1, 0, 0, 1));

	p2 = p1 + k_axisScale * xf.q.GetYAxis();
	renderer->debug_Line(p1, p2, 0.01f, b2Color(0, 0, 1, 1));
}

void DebugDrawer::drawWorld(b2World* world){
	for (b2Body* b = world->GetBodyList(); b; b = b->GetNext()){

		bodyData* bData = (bodyData*)b->GetUserData().pointer;
		if (bData && (bData->type == PORTAL_BODY || bData->type == PORTAL)) continue;

		const b2Transform& xf = b->GetTransform();
		for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
		{
			if (b->GetType() == b2_dynamicBody && b->GetMass() == 0.0f) {
				// Bad body (0 density)
				drawShape(f, xf, b2Color(1.0f, 0.0f, 0.0f));
			}
			else if (b->IsEnabled() == false){
				drawShape(f, xf, b2Color(0.5f, 0.5f, 0.3f));
			}
			else if (b->GetType() == b2_staticBody){
				drawShape(f, xf, b2Color(0.5f, 0.9f, 0.5f));
			}
			else if (b->GetType() == b2_kinematicBody){
				drawShape(f, xf, b2Color(0.5f, 0.5f, 0.9f));
			}
			else if (b->IsAwake() == false){
				drawShape(f, xf, b2Color(0.6f, 0.6f, 0.6f));
			}
			else{
				drawShape(f, xf, b2Color(0.9f, 0.7f, 0.7f));
			}
		}
	}
}

void DebugDrawer::drawShape(b2Fixture* fixture, const b2Transform& xf, const b2Color& color){
	switch (fixture->GetType())
	{
	case b2Shape::e_circle:
		{
			if (fixture->IsSensor()) {
				break;
			}
			b2CircleShape* circle = (b2CircleShape*)fixture->GetShape();

			b2Vec2 center = b2Mul(xf, circle->m_p);
			float radius = circle->m_radius;
			b2Vec2 axis = b2Mul(xf.q, b2Vec2(1.0f, 0.0f));

			DrawSolidCircle(center, radius, axis, color);
		}
		break;

	case b2Shape::e_edge:
		{
			b2EdgeShape* edge = (b2EdgeShape*)fixture->GetShape();
			b2Vec2 v1 = b2Mul(xf, edge->m_vertex1);
			b2Vec2 v2 = b2Mul(xf, edge->m_vertex2);
			DrawSegment(v1, v2, color);

			if (edge->m_oneSided == false)
			{
				DrawPoint(v1, 4.0f, color);
				DrawPoint(v2, 4.0f, color);
			}
		}
		break;

	case b2Shape::e_chain:
		{
			b2ChainShape* chain = (b2ChainShape*)fixture->GetShape();
			int32 count = chain->m_count;
			const b2Vec2* vertices = chain->m_vertices;

			b2Vec2 v1 = b2Mul(xf, vertices[0]);
			for (int32 i = 1; i < count; ++i)
			{
				b2Vec2 v2 = b2Mul(xf, vertices[i]);
				DrawSegment(v1, v2, color);
				v1 = v2;
			}
		}
		break;

	case b2Shape::e_polygon:
		{
			if (fixture->IsSensor()) {
				break;
			}

			b2PolygonShape* poly = (b2PolygonShape*)fixture->GetShape();
			int32 vertexCount = poly->m_count;
			b2Assert(vertexCount <= b2_maxPolygonVertices);
			b2Vec2 vertices[b2_maxPolygonVertices];

			for (int32 i = 0; i < vertexCount; ++i)
			{
				vertices[i] = b2Mul(xf, poly->m_vertices[i]);
			}

			DrawSolidPolygon(vertices, vertexCount, color);
		}
		break;

	default:
		break;
	}
}
