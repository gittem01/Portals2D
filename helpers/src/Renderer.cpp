#include <Camera.h>

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

b2Vec2 rotateVector(b2Vec2 vec, float angle){
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return {x, y};
}

Renderer::Renderer(PortalWorld* pWorld, Camera* camera)
{
    this->pWorld = pWorld;
    this->camera = camera;
    
    this->polyShader = new Shader("../assets/shaders/PolygonShaders/");
    this->circShader = new Shader("../assets/shaders/CircleShaders/");
    this->lineShader = new Shader("../assets/shaders/LineShaders/");

    this->drawReleases = false;
    this->releaseColor = b2Color(1.0f, 1.0f, 1.0f, 0.2f);

    float vertices[] = {
        -0.5f, +0.5f,
        -0.5f, -0.5f,
        +0.5f, -0.5f,
        +0.5f, -0.5f,
        +0.5f, +0.5f,
        -0.5f, +0.5f,
    };

    // just to make driver happy for now
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 48, vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
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

void Renderer::drawArrow(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color){
	b2Vec2 dir = 0.25f * (p2 - p1);
	b2Vec2 vec1 = rotateVector(dir, b2_pi * (5 / 6.0f));
	b2Vec2 vec2 = rotateVector(dir, b2_pi * (7 / 6.0f));

	debug_Line(p1, p2, 0.02f, color);
	debug_Line(p2, p2 + vec1, 0.02f, color);
	debug_Line(p2, p2 + vec2, 0.02f, color);
}

void Renderer::portalRender(portalRenderData* prd){
    debug_Line(prd->portal->points[0], prd->portal->points[1], 0.02f, prd->color);

    // draw dir
    // drawArrow(prd->portal->pos, prd->portal->pos + 1.0f * prd->portal->dir, prd->color);
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

void Renderer::debug_Polygon(const b2Vec2* vertices, int vertexCount, float outThickness, b2Color color, b2Color outerColor){
    polyShader->use();

    polyShader->setMat4("ortho", camera->ortho);

    glm::mat4 model = glm::mat4(1.0f);

    polyShader->setMat4("model", model);

    polyShader->setVec4("colour", color.r, color.g, color.b, 0.5f);
    polyShader->setVec4("outerColour", color.r, color.g, color.b, 1.0f);

    polyShader->setInt("numPoints", vertexCount);

    polyShader->setFloat("lineDist", outThickness);
    polyShader->setFloat("zoom", camera->zoom);

    polyShader->setInt("numPortals", 0);

    unsigned int loc = glGetUniformLocation(polyShader->ID, "vertices");
    glUniform2fv(loc, vertexCount, (GLfloat*)vertices);

    glDrawArrays(GL_TRIANGLES, 0, (vertexCount - 2) * 3);
}

void Renderer::drawPolygonFix(PortalBody* pBody, b2Fixture* fix, b2Color color){
    polyShader->use();

    b2PolygonShape* pShape = (b2PolygonShape*)fix->GetShape();

    float transparency = 1.0f;
    if (!pBody->body->IsAwake()){
        transparency /= 1.5f;
    }

    polyShader->setMat4("ortho", camera->ortho);

    b2Vec2 pos = pBody->body->GetPosition();
    float angle = pBody->body->GetAngle();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
    model = glm::rotate(model, angle, glm::vec3(0, 0, 1));

    polyShader->setMat4("model", model);

    polyShader->setVec4("colour", color.r, color.g, color.b, transparency);
    polyShader->setVec4("outerColour", color.r, color.g, color.b, 1.0f);
    
    polyShader->setVec4("releaseColour", releaseColor.r, releaseColor.g, releaseColor.b, releaseColor.a);
    polyShader->setBool("drawReleases", drawReleases);

    polyShader->setInt("numPoints", pShape->m_count);

    polyShader->setFloat("lineDist", 0.015f);
    polyShader->setFloat("zoom", camera->zoom);

    auto colls = pBody->fixtureCollisions[fix];

    int numPortals = colls->size();
    float* portalPositions = (float*)malloc(numPortals * 4 * sizeof(float));
    int* sideMults = (int*)malloc(sizeof(int) * numPortals);

    bool fRelease = false;
    int i = 0;
    for (portalCollision* coll : *colls){
        if (coll->status == 0){
            if (!drawReleases){
                return;
            }
            else{
                fRelease = true;
                break;
            }
        }

        portalPositions[i] = coll->portal->points[0].x;
        portalPositions[i + 1] = coll->portal->points[0].y;
        portalPositions[i + 2] = coll->portal->points[1].x;
        portalPositions[i + 3] = coll->portal->points[1].y;

        sideMults[i / 4] = coll->side ? -1 : 1;
        
        i += 4;
    }

    polyShader->setBool("isFullRelease", fRelease);
    polyShader->setInt("numPortals", numPortals);

    unsigned int loc = glGetUniformLocation(polyShader->ID, "vertices");
	glUniform2fv(loc, pShape->m_count, (GLfloat*)pShape->m_vertices);

    loc = glGetUniformLocation(polyShader->ID, "portals");
	glUniform4fv(loc, numPortals, (GLfloat*)portalPositions);

    loc = glGetUniformLocation(polyShader->ID, "sideMults");
	glUniform1iv(loc, numPortals, (GLint*)sideMults);

    glDrawArrays(GL_TRIANGLES, 0, (pShape->m_count - 2) * 3);

    free(portalPositions);
    free(sideMults);
}

void Renderer::drawCircleFix(PortalBody* pBody, b2Fixture* fix, b2Color color){
    circShader->use();

    float transparency = 1.0f;
    if (!pBody->body->IsAwake()){
        transparency /= 1.5f;
    }

    b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
    float r = shape->m_radius;

    circShader->setMat4("ortho", camera->ortho);

    b2Vec2 pos = pBody->body->GetWorldPoint(shape->m_p);

    circShader->setVec2("pos", pos.x, pos.y);
    circShader->setFloat("size", r * 2);

    circShader->setVec4("colour", color.r, color.g, color.b, transparency);
    circShader->setVec4("outerColour", color.r, color.g, color.b, 1);

    circShader->setVec4("releaseColour", releaseColor.r, releaseColor.g, releaseColor.b, releaseColor.a);
    circShader->setBool("drawReleases", drawReleases);

    circShader->setFloat("lineDist", 0.015f);
    circShader->setFloat("zoom", camera->zoom);
    circShader->setBool("isPoint", false);

    auto colls = pBody->fixtureCollisions[fix];

    int numPortals = colls->size();
    float* portalPositions = (float*)malloc(numPortals * 4 * sizeof(float));
    int* sideMults = (int*)malloc(sizeof(int) * numPortals);

    bool fRelease = false;
    int i = 0;
    for (portalCollision* coll : *colls){
        if (coll->status == 0){
            if (!drawReleases){
                return;
            }
            else{
                fRelease = true;
                break;
            }
        }

        portalPositions[i] = coll->portal->points[0].x;
        portalPositions[i + 1] = coll->portal->points[0].y;
        portalPositions[i + 2] = coll->portal->points[1].x;
        portalPositions[i + 3] = coll->portal->points[1].y;

        sideMults[i / 4] = coll->side ? -1 : 1;
        
        i += 4;
    }

    circShader->setBool("isFullRelease", fRelease);
    circShader->setInt("numPortals", numPortals);

    unsigned int loc = glGetUniformLocation(circShader->ID, "portals");
	glUniform4fv(loc, numPortals, (GLfloat*)portalPositions);

    loc = glGetUniformLocation(circShader->ID, "sideMults");
	glUniform1iv(loc, numPortals, (GLint*)sideMults);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    free(portalPositions);
    free(sideMults);
}

void Renderer::debug_Line(b2Vec2 p1, b2Vec2 p2, float thickness, b2Color color){
    lineShader->use();

    lineShader->setMat4("ortho", camera->ortho);

    lineShader->setVec4("colour", color.r, color.g, color.b, color.a);
    lineShader->setFloat("zoom", camera->zoom);

    lineShader->setFloat("thickness", thickness);
    lineShader->setVec2("p1", p1.x, p1.y);
    lineShader->setVec2("p2", p2.x, p2.y);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::debug_Circle(b2Vec2 p, float r, float outThickness, b2Color color, bool isPoint, b2Color outerColor){
    circShader->use();

    circShader->setMat4("ortho", camera->ortho);

    circShader->setVec2("pos", p.x, p.y);
    circShader->setFloat("size", r);
    circShader->setVec4("colour", color.r, color.g, color.b, color.a);
    circShader->setVec4("outerColour", outerColor.r, outerColor.g, outerColor.b, outerColor.a);

    circShader->setFloat("lineDist", outThickness);
    circShader->setFloat("zoom", camera->zoom);
    circShader->setBool("isPoint", isPoint);

    circShader->setInt("numPortals", 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}