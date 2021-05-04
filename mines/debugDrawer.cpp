#include "debugDrawer.h"
#include <GLFW/glfw3.h>

void debugDrawer::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color){
	glLineWidth(1.0f);
	glColor4f(color.r, color.g, color.b, color.a);
	glBegin(GL_LINES);
	for (int i = 0; i < vertexCount; i++) {
		glVertex2d((vertices + i)->x, (vertices + i)->y);
		glVertex2d((vertices + (i + 1) % vertexCount)->x, (vertices + (i + 1) % vertexCount)->y);
	}
	glEnd();
}

void debugDrawer::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
	glColor4f(color.r, color.g, color.b, 0.5f);
	glBegin(GL_POLYGON);
	for (int i = 0; i < vertexCount; i+=1) {
		glVertex2d((vertices + i)->x, (vertices + i)->y);
	}
	glEnd();

	glLineWidth(1.0f);
	glColor4f(color.r, color.g, color.b, 1.0f);
	glBegin(GL_LINES);
	for (int i = 0; i < vertexCount; i++) {
		glVertex2d((vertices + i)->x, (vertices + i)->y);
		glVertex2d((vertices + (i + 1) % vertexCount)->x, (vertices + (i + 1) % vertexCount)->y);
	}
	glEnd();
}

void debugDrawer::DrawSolidCircle(const b2Vec2& center, float radius, 
									const b2Vec2& axis, const b2Color& color){

	int n = 100;
	b2Vec2* positions = (b2Vec2*)malloc((n + 1) * sizeof(b2Vec2));
	glColor4f(color.r, color.g, color.b, 0.5f);
	glBegin(GL_POLYGON);
	float angle = 0;
	for (int i=0; i<=n; i++){
		b2Vec2 pos = b2Vec2(sin(angle) * radius + center.x, cos(angle) * radius + center.y);
		positions[i] = pos;
		glVertex2d(pos.x, pos.y);
		angle = (b2_pi * i * 2) / n;
	}
	glEnd();

	glColor4f(color.r, color.g, color.b, 1.0f);
	glBegin(GL_LINES);
	for (int i=0; i<=n; i++){
		glVertex2d(positions[i].x, positions[i].y);
		glVertex2d(positions[(i+1)%n].x, positions[(i+1)%n].y);
	}
	glVertex2d(center.x, center.y);
	glVertex2d(center.x + axis.x * radius, center.y + axis.y * radius);
	glEnd();

	free(positions);
}

void debugDrawer::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {
	glLineWidth(2.0f);
	glColor4f(color.r, color.g, color.b, color.a);
	glBegin(GL_LINES);
	glVertex2d(p1.x, p1.y);
	glVertex2d(p2.x, p2.y);
	glEnd();
}

void debugDrawer::DrawPoint(const b2Vec2& p, float size, const b2Color& color) {
	glPointSize(size);
	glColor4f(color.r, color.g, color.b, 1.0f);
	glBegin(GL_POINTS);
	glVertex2d(p.x, p.y);
	glEnd();
}