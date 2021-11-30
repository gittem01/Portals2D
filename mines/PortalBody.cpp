#include "PortalBody.h"
#include "glm/common.hpp"

std::vector<PortalBody*> PortalBody::portalBodies;

PortalBody::PortalBody(b2Body* body, b2World* world, b2Vec3 bodyColor){
    this->world = world;
    this->bodyMaps[body] = new std::vector<void*>();
    this->bodyColor = bodyColor;
    PortalBody::portalBodies.push_back(this);
}

void PortalBody::drawBodies(){
    for (std::map<b2Body*, std::vector<void*>*>::iterator iter = bodyMaps.begin(); iter != bodyMaps.end(); iter++){
        for (b2Fixture* fix = iter->first->GetFixtureList(); fix != nullptr; fix = fix->GetNext()){
            if (fix->GetType() == b2Shape::Type::e_polygon){
                drawPolygonFix(fix);
            }
            else if (fix->GetType() == b2Shape::Type::e_circle){
                drawCircleFix(fix);
            }
        }
    }
}

void PortalBody::drawPolygonFix(b2Fixture* fix){
    b2Body* body = fix->GetBody();
    b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();

    int vertexCount = shape->m_count;
    b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2*) * vertexCount);

	for (int i = 0; i < vertexCount; i++) {
        b2Vec2 p1 = b2Vec2((shape->m_vertices + i)->x, (shape->m_vertices + i)->y);
        p1 = body->GetWorldPoint(p1);
        vertices[i] = p1;
	}

    drawVertices(body, vertices, vertexCount);
}

void PortalBody::drawCircleFix(b2Fixture* fix){
    b2Body* body = fix->GetBody();
    b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
    float r = shape->m_radius;

    int vertexCount = 20;
    b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2*) * vertexCount);

    float angle = 0;
	for (int i = 0; i < vertexCount; i++) {
        float angle0 = ((float)i / vertexCount) * b2_pi * 2;
        b2Vec2 p = b2Vec2(sin(angle0) * r, cos(angle0) * r) + shape->m_p;
        p = body->GetWorldPoint(p);
        vertices[i] = p;
	}

    drawVertices(body, vertices, vertexCount);
}

void PortalBody::drawVertices(b2Body* body, b2Vec2* vertices, int vertexCount){
    b2Vec3 color;
    if (body->IsAwake()){
        color = bodyColor;
    }
    else{
        color = b2Vec3(1.0f, 1.0f, 1.0f);
    }
    
    glColor4f(color.x, color.y, color.z, 0.2f);
	glBegin(GL_POLYGON);
	for (int i = 0; i < vertexCount; i+=1) {
		glVertex2d((vertices + i)->x, (vertices + i)->y);
	}
	glEnd();

	glLineWidth(1.0f);
	glColor4f(color.x, color.y, color.z, 1.0f);
	glBegin(GL_LINES);
	for (int i = 0; i < vertexCount; i++) {
		glVertex2d((vertices + i)->x, (vertices + i)->y);
		glVertex2d((vertices + (i + 1) % vertexCount)->x, (vertices + (i + 1) % vertexCount)->y);
	}
	glEnd();
}

b2Vec2 PortalBody::getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4){
    float d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    float up1 = (p1.x * p2.y - p1.y * p2.x) * (p3.x - p4.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x);
    float up2 = (p1.x * p2.y - p1.y * p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x);

    return b2Vec2(up1 / d, up2 / d);
}