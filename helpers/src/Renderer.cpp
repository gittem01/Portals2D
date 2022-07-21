#include <Renderer.h>

#define RENDER_COLOURFUL 0

#if RENDER_COLOURFUL
    b2Color colors[] = {
        b2Color(0, 1, 0, 0.5f),
        b2Color(0, 0, 1, 0.5f),
        b2Color(0, 1, 1, 0.5f),
        b2Color(1, 1, 0, 0.5f),
        b2Color(1, 1, 1, 0.2f)
    };
#endif

Renderer::Renderer(PortalWorld* pWorld)
{
    this->pWorld = pWorld;
}

void Renderer::addPortal(Portal* portal, b2Color color){
    portalRenderData* portalData = new portalRenderData;
    portalData->portal = portal;
    portalData->color = color;

    portals.push_back(portalData);
}

void Renderer::addPortalBody(PortalBody* pBody, b2Color color){
    bodyRenderData* bodyData = new bodyRenderData;
    bodyData->worldIndex = pBody->worldIndex;
    bodyData->color = color;
    
    portalBodies.push_back(bodyData);
}

void Renderer::render(){
    for (portalRenderData* prd : portals){
        portalRender(prd);
    }

    for (bodyRenderData* brd : portalBodies){
        bodyRender(brd);
    }
}

void Renderer::portalRender(portalRenderData* prd){
    glLineWidth(2.0f);
	glColor4f(prd->color.r, prd->color.g, prd->color.b, prd->color.a);
	glBegin(GL_LINES);
	glVertex2d(prd->portal->points[0].x, prd->portal->points[0].y);
	glVertex2d(prd->portal->points[1].x, prd->portal->points[1].y);
	glEnd();

    // draw dir
    // pWorld->drawer->DrawArrow(pos, pos + 1.0f * dir, b2Color(1, 1, 1, 1));
}

void Renderer::bodyRender(bodyRenderData* brd){
    for (PortalBody* pBody : *brd->worldIndex){
        for (b2Fixture* fix = pBody->body->GetFixtureList(); fix != nullptr; fix = fix->GetNext()){
            if (fix->GetType() == b2Shape::Type::e_polygon){
                drawPolygonFix(pBody, fix, brd->color);
            }
            else if (fix->GetType() == b2Shape::Type::e_circle){
                drawCircleFix(pBody, fix, brd->color);
            }
        }
#if 0
        b2Transform t = body->GetTransform();
        t.q.Set(offsetAngle + body->GetAngle());
        pWorld->drawer->DrawTransform(t);
#endif
    }
}


void Renderer::portalRender(PortalBody* pBody, b2Fixture* fix, std::vector<b2Vec2>& vertices, b2Color color){
    b2Body* bBody = fix->GetBody();

    int side;
    int renderStatus = 2;

    std::map<b2Fixture*, std::set<portalCollision*>*>::iterator fixIter = pBody->fixtureCollisions.find(fix);
    std::set<portalCollision*>::iterator iter = (*fixIter).second->begin();
    
    void* portal = NULL;
    int s = -1;
    if (iter != (*fixIter).second->end()){
        portal = (*iter)->portal;
        s = (*iter)->side;
        renderStatus = (*iter)->status;
    }
    for (portalCollision* coll : *(*fixIter).second){
        if (coll->status == 0){
            renderStatus = 0;
            break;
        }
        else{
            renderStatus = 1;
        }
    }

    if (renderStatus == 1){
        if (pBody->allParts[fix]->at(0)->size() > 0){
#if RENDER_COLOURFUL
            color = b2Color(1, 0, 1, 0.5f);
#endif
            drawVertices(pBody, *pBody->allParts[fix]->at(0), color);
        }
        if (pWorld->drawReleases){
            for (int i = 1; i < pBody->allParts[fix]->size(); i++){
                auto& vecs = pBody->allParts[fix]->at(i);                
#if RENDER_COLOURFUL
                b2Color c = b2Color(1, 0, 0, 0.5f);
#else
                b2Color c = pWorld->releaseColor;
#endif
                drawVertices(pBody, *vecs, c);
            }
        }
    }
    else if (pWorld->drawReleases && renderStatus == 0){
            b2Color c;

#if RENDER_COLOURFUL
            auto iter = pBody->fixtureCollisions[fix]->begin();
            int val = (*iter)->portal->releaseFixtures[(*iter)->side][fix];
            if (val <= sizeof(colors) / sizeof(colors[0])){
                c = colors[val - 1];
            }

#else
            c = pWorld->releaseColor;
#endif
            drawVertices(pBody, vertices, c);
    }

    else if (renderStatus == 2) drawVertices(pBody, *pBody->allParts[fix]->at(0), color);
}

void Renderer::drawPolygonFix(PortalBody* pBody, b2Fixture* fix, b2Color color){
    b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();

    int vertexCount = shape->m_count;
    std::vector<b2Vec2> vertices;

	for (int i = 0; i < vertexCount; i++) {
        b2Vec2 p = b2Vec2((shape->m_vertices + i)->x, (shape->m_vertices + i)->y);
        p = pBody->body->GetWorldPoint(p);
        vertices.push_back(p);
	}

    portalRender(pBody, fix, vertices, color);
}

void Renderer::drawCircleFix(PortalBody* pBody, b2Fixture* fix, b2Color color){
    b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
    float r = shape->m_radius;

    std::vector<b2Vec2> vertices;

    float angle = 0;
	for (int i = 0; i < CIRCLE_POINTS; i++) {
        float angle0 = ((float)i / CIRCLE_POINTS) * b2_pi * 2;
        b2Vec2 p = b2Vec2(sin(angle0) * r, cos(angle0) * r) + shape->m_p;
        p = pBody->body->GetWorldPoint(p);
        vertices.push_back(p);
	}

    portalRender(pBody, fix, vertices, color);
}

void Renderer::drawVertices(PortalBody* pBody, std::vector<b2Vec2>& vertices, b2Color color){
    float transparency = 1.0f;
    if (!pBody->body->IsAwake()){
        transparency /= 1.5f;
    }
    
    glColor4f(color.r, color.g, color.b, color.a * transparency);
	glBegin(GL_POLYGON);
	for (int i = 0; i < vertices.size(); i++) {
		glVertex2d(vertices.at(i).x, vertices.at(i).y);
	}
	glEnd();

	glLineWidth(1.0f);
	glColor4f(color.r, color.g, color.b, color.a * 2.0f);
	glBegin(GL_LINES);
	for (int i = 0; i < vertices.size(); i++) {
		glVertex2d(vertices.at(i).x, vertices.at(i).y);
		glVertex2d(vertices.at((i + 1) % vertices.size()).x, vertices.at((i + 1) % vertices.size()).y);
	}
	glEnd();
}