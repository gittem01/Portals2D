#include <Camera.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define RENDER_COLOURFUL 0
#define FULLY_RANDOM 0

#if RENDER_COLOURFUL
    b2Color colors[] = {
        b2Color(0, 1, 0, 0.5f),
        b2Color(0, 0, 1, 0.5f),
        b2Color(0, 1, 1, 0.5f),
        b2Color(1, 1, 0, 0.5f),
        b2Color(1, 1, 1, 0.2f)
    };
#endif

#if FULLY_RANDOM
    std::random_device rd05;
    std::mt19937 mt05(rd05());
#else
    std::mt19937 mt05(109050);
#endif

std::uniform_real_distribution<double> rand05(-0.5f, 0.5f);

b2Vec2 rotateVector(b2Vec2 vec, float angle){
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return {x, y};
}

float r05(){
    rand05(mt05);
    return rand05(mt05);
}

Renderer::Renderer(PortalWorld* pWorld, Camera* camera)
{
    this->pWorld = pWorld;
    this->camera = camera;
    
    this->polyShader = new Shader("assets/shaders/PolygonShaders/");
    this->circShader = new Shader("assets/shaders/CircleShaders/");
    this->lineShader = new Shader("assets/shaders/LineShaders/");
    this->dotShader = new Shader("assets/shaders/DotShaders/");
    this->portalShader = new Shader("assets/shaders/PortalShaders/");

    this->drawReleases = false;
    this->releaseColor = b2Color(1.0f, 1.0f, 1.0f, 0.2f);

    createVAO();

    float sMult = 0.25f;
    for (int i = 0; i < numDots; i++){
        colors.push_back(glm::vec4(r05() + 0.5f, r05() + 0.5f, 1, 1));
        float rSize = r05() * 0.5f + 0.3f;
        sizes.push_back(glm::vec2(rSize, rSize));

        // prlx multi, inperct, outCurve
        mults.push_back(glm::vec3(sMult, r05() * 0.5 - 0.05f, (r05() + 1.5f) * 2));
        positions.push_back(glm::vec2(r05() * 320 * sMult, r05() * 180 * sMult));
        sMult += 0.5 / numDots;
    }
    prepareDots();
}

void Renderer::prepareDots(){
    glGenBuffers(1, &pos_SBO);
    glBindBuffer(GL_ARRAY_BUFFER, pos_SBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(positions.at(0)), positions.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &size_SBO);
    glBindBuffer(GL_ARRAY_BUFFER, size_SBO);
    glBufferData(GL_ARRAY_BUFFER, sizes.size() * sizeof(sizes.at(0)), sizes.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &mult_SBO);
    glBindBuffer(GL_ARRAY_BUFFER, mult_SBO);
    glBufferData(GL_ARRAY_BUFFER, mults.size() * sizeof(mults.at(0)), mults.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &color_SBO);
    glBindBuffer(GL_ARRAY_BUFFER, color_SBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(colors.at(0)), colors.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
}

void Renderer::createVAO(){
    float vertices[] = {
        -0.5f, +0.5f,
        -0.5f, -0.5f,
        +0.5f, -0.5f,
        +0.5f, -0.5f,
        +0.5f, +0.5f,
        -0.5f, +0.5f,
    };

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

    for (int side = 0; side < 2; side++){
        for (int i = 0; i < portal->connections[side].size(); i++){
            Portal* portal2 = portal->connections[side].at(i)->portal2;
            int side2 = portal->connections[side].at(i)->side2;

            if (addedPortals.find({{ portal, side }, { portal2, side2 }}) != addedPortals.end() ||
                addedPortals.find({{ portal2, side2 }, { portal, side }}) != addedPortals.end())
            {
                continue;
            }
            addedPortals.insert({{ portal, side }, {portal2, side2 }});

            glm::vec4 baseColor = glm::vec4(r05() + 0.5f, r05() + 0.5f, 0, 1);
            baseColor.z = 1 - baseColor.x;
            float portalLen = b2Distance(portal->points[0], portal->points[1]);
            for (int j = 0; j < numPortalPs * portalLen; j++){
                pSizes.push_back(r05() * 0.2f + 0.2f);
                pSides.push_back(side);
                pSpeeds.push_back(r05() + 1.0f);
                pPortals.push_back(glm::vec4(   portal->points[side].x, portal->points[side].y, 
                                                portal->points[side ^ 1].x, portal->points[side ^ 1].y));
                if (portal->isVoid[side]){
                    pColors.push_back(glm::vec4(0.3, 0.3, 0.3, 1));
                }
                else{
                    pColors.push_back(baseColor + glm::vec4(r05() * 0.5f, r05() * 0.5f, r05() * 0.5f, 1));
                }
                pMults.push_back(glm::vec4(r05(), 0.1, 5.0f, 1));
            }

            for (int j = 0; j < numPortalPs * portalLen; j++){
                pSizes.push_back(pSizes.at(pSizes.size() - numPortalPs * portalLen));
                pSides.push_back(side2);
                pSpeeds.push_back(pSpeeds.at(pSpeeds.size() - numPortalPs * portalLen));
                pPortals.push_back(glm::vec4(   portal2->points[side2].x, portal2->points[side2].y, 
                                                portal2->points[side2 ^ 1].x, portal2->points[side2 ^ 1].y));
                pColors.push_back(pColors.at(pColors.size() - numPortalPs * portalLen));
                glm::vec4 lastPmult = pMults.at(pMults.size() - numPortalPs * portalLen);
                if (!portal->connections[side].at(i)->isReversed){
                    lastPmult.x *= -1;
                }
                pMults.push_back(lastPmult);
            }
        }
    }
}

void Renderer::portalAddEnd(){
    glGenBuffers(1, &pBufferObjects[0]);
    glBindBuffer(GL_ARRAY_BUFFER, pBufferObjects[0]);
    glBufferData(GL_ARRAY_BUFFER, pSizes.size() * sizeof(pSizes.at(0)), pSizes.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &pBufferObjects[1]);
    glBindBuffer(GL_ARRAY_BUFFER, pBufferObjects[1]);
    glBufferData(GL_ARRAY_BUFFER, pSides.size() * sizeof(pSides.at(0)), pSides.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &pBufferObjects[2]);
    glBindBuffer(GL_ARRAY_BUFFER, pBufferObjects[2]);
    glBufferData(GL_ARRAY_BUFFER, pSpeeds.size() * sizeof(pSpeeds.at(0)), pSpeeds.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &pBufferObjects[3]);
    glBindBuffer(GL_ARRAY_BUFFER, pBufferObjects[3]);
    glBufferData(GL_ARRAY_BUFFER, pMults.size() * sizeof(pMults.at(0)), pMults.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &pBufferObjects[4]);
    glBindBuffer(GL_ARRAY_BUFFER, pBufferObjects[4]);
    glBufferData(GL_ARRAY_BUFFER, pColors.size() * sizeof(pColors.at(0)), pColors.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &pBufferObjects[5]);
    glBindBuffer(GL_ARRAY_BUFFER, pBufferObjects[5]);
    glBufferData(GL_ARRAY_BUFFER, pPortals.size() * sizeof(pPortals.at(0)), pPortals.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);
    glEnableVertexAttribArray(7);
    glEnableVertexAttribArray(8);
    glEnableVertexAttribArray(9);
    glEnableVertexAttribArray(10);

    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);
    glVertexAttribDivisor(9, 1);
    glVertexAttribDivisor(10, 1);
}

void Renderer::addPortalBody(PortalBody* pBody, b2Color color, const char* texture){
    unsigned int texObject = 0;
    if (texture){
        unsigned char *data;
        int width, height, nrChannels;
        std::string newTexPath = std::string(texture).c_str();
        for (int i = 0; i < 4; i++){
            data = stbi_load(newTexPath.c_str(), &width, &height, &nrChannels, 0);
            if (!data){
                newTexPath = "../" + newTexPath;
            }
            else{
                break;
            }
        }
        if (data){
            glGenTextures(1, &texObject);
            glBindTexture(GL_TEXTURE_2D, texObject);
            // set the texture wrapping/filtering options (on the currently bound texture object)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            if (nrChannels == 3)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            else if (nrChannels == 4)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            stbi_image_free(data);
        }
        else{
            std::cout << "Failed to load texture" << std::endl;
            std::exit(-1);
        }
    }
    
    glm::vec2 horz = glm::vec2(+9999999.0f, -9999999.0f);
    glm::vec2 vert = glm::vec2(+9999999.0f, -9999999.0f);
    if (texture){
        for (b2Fixture* fix = pBody->body->GetFixtureList(); fix; fix = fix->GetNext()){
            if (fix->GetType() == b2Shape::e_polygon){
                b2PolygonShape* poly = (b2PolygonShape*)fix->GetShape();

                for (int i = 0; i < poly->m_count; i++){
                    b2Vec2 vertex = poly->m_vertices[i];

                    horz.x = std::min(horz.x, vertex.x);
                    horz.y = std::max(horz.y, vertex.x);
                    vert.x = std::min(vert.x, vertex.y);
                    vert.y = std::max(vert.y, vertex.y);
                }
            }
            else if (fix->GetType() == b2Shape::e_circle){
                b2CircleShape* circ = (b2CircleShape*)fix->GetShape();
                
                b2Vec2 center = circ->m_p;
                float radius = circ->m_radius;

                horz.x = std::min(horz.x, center.x - radius);
                horz.y = std::max(horz.y, center.x + radius);
                vert.x = std::min(vert.x, center.y - radius);
                vert.y = std::max(vert.y, center.y + radius);
            }
        }
    }

    bodyRenderData* bodyData = new bodyRenderData;
    bodyData->worldIndex = pBody->worldIndex;
    bodyData->color = color;
    bodyData->texture = texObject;

    bodyData->xSides = horz;
    bodyData->ySides = vert;

    bodyData->mirroredXSides = glm::vec2(-horz.y, -horz.x);

    for (b2Fixture* fix = pBody->body->GetFixtureList(); fix; fix = fix->GetNext()){
        fixData* fData = new fixData;
        if (fix->GetType() == b2Shape::e_polygon){
            b2PolygonShape* poly = (b2PolygonShape*)fix->GetShape();
            fData->type = POLYGON;
            fData->vertexCount = poly->m_count;

            for (int i = 0; i < poly->m_count; i++){
                b2Vec2 vertex = poly->m_vertices[i];
                fData->vertices[i] = vertex;
                fData->mirroredVertices[i] = pWorld->mirror(b2Vec2(0, 1), vertex);
            }
            bodyData->fixtureDatas.push_back(fData);
        }
        else if (fix->GetType() == b2Shape::e_circle){
            b2CircleShape* circ = (b2CircleShape*)fix->GetShape();
            fData->type = CIRCLE;
            fData->cPos = circ->m_p;
            fData->mirroredCPos = pWorld->mirror(b2Vec2(0, 1), circ->m_p);
            fData->radius = circ->m_radius;
            bodyData->fixtureDatas.push_back(fData);
        }
    }

    portalBodies.push_back(bodyData);
}

void Renderer::render(){
    glfwGetFramebufferSize(camera->window, &win_width, &win_height);
    minLineThck = 20.0f / win_width;

    portalRender();

    for (portalRenderData* prd : portals){
        portalRender(prd);
    }

    for (bodyRenderData* brd : portalBodies){
        bodyRender(brd);
    }
}

void Renderer::dotRender(){
    dotShader->use();
    
    dotShader->setMat4("ortho", camera->ortho);
    dotShader->setFloat("zoom", camera->zoom);

    glBindBuffer(pos_SBO, GL_ARRAY_BUFFER);
    glBindBuffer(size_SBO, GL_ARRAY_BUFFER);
    glBindBuffer(mult_SBO, GL_ARRAY_BUFFER);
    glBindBuffer(color_SBO, GL_ARRAY_BUFFER);

    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, numDots);
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
    debug_Line(prd->portal->points[0], prd->portal->points[1], 0.01f, prd->color);

    // draw dir
    // drawArrow(prd->portal->pos, prd->portal->pos + 1.0f * prd->portal->dir, prd->color);
}

void Renderer::portalRender(){
    gameTime += 1.0f / 60.0f;

    portalShader->use();
    
    portalShader->setMat4("ortho", camera->ortho);
    portalShader->setFloat("zoom", camera->zoom);
    portalShader->setFloat("gameTime", gameTime);
    portalShader->setInt("numParticles", numPortalPs);

    for (int i = 0; i < 6; i++){
        glBindBuffer(pBufferObjects[i], GL_ARRAY_BUFFER);
    }

    glBindVertexArray(VAO);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, pPortals.size());
}

void Renderer::bodyRender(bodyRenderData* brd){
    for (PortalBody* pBody : *brd->worldIndex){
        int i = 0;
        for (b2Fixture* fix = pBody->body->GetFixtureList(); fix; fix = fix->GetNext()){
            if (fix->GetType() == b2Shape::Type::e_polygon){                
                drawPolygonFix(pBody, fix, brd, i++);
            }
            else if (fix->GetType() == b2Shape::Type::e_circle){
                drawCircleFix(pBody, fix, brd, i++);
            }
        }
#if 0
        b2Transform t = pBody->body->GetTransform();
        t.q.Set(pBody->offsetAngle + pBody->body->GetAngle());
        
        const float axisScale = 0.3f / (camera->zoom * camera->zoom);
	
        b2Vec2 p1 = t.p, p2;

        p2 = p1 + axisScale * t.q.GetXAxis();
        debug_Line(p1, p2, 0.01f, b2Color(1, 0, 0, 1));

        p2 = p1 + axisScale * t.q.GetYAxis();
        debug_Line(p1, p2, 0.01f, b2Color(0, 0, 1, 1));
#endif
    }
}

void Renderer::drawPolygonFix(PortalBody* pBody, b2Fixture* fix, bodyRenderData* brd, int renderIndex){
    polyShader->use();

    b2Color color = brd->color;

    float transparency = 1.0f;
    if (!pBody->body->IsAwake()){
        transparency /= 1.5f;
    }

    if (brd->texture){
        polyShader->setFloat("lineDist", 0.0f);
        polyShader->setBool("hasTexture", true);
        if (pBody->isMirrored){
            polyShader->setVec2("xSides", brd->mirroredXSides.x, brd->mirroredXSides.y);
        }
        else{
            polyShader->setVec2("xSides", brd->xSides.x, brd->xSides.y);
        }
        polyShader->setVec2("ySides", brd->ySides.x, brd->ySides.y);
        glBindTexture(GL_TEXTURE_2D, brd->texture);
    }
    else{
        polyShader->setFloat("lineDist", glm::max(minLineThck, 0.015f));
        polyShader->setBool("hasTexture", false);
    }

    polyShader->setMat4("ortho", camera->ortho);

    b2Vec2 pos = pBody->body->GetPosition();
    float angle = pBody->body->GetAngle() + pBody->offsetAngle;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
    model = glm::rotate(model, angle, glm::vec3(0, 0, 1));

    polyShader->setMat4("model", model);

    polyShader->setVec4("colour", color.r, color.g, color.b, transparency);
    polyShader->setVec4("outerColour", color.r, color.g, color.b, 1.0f);
    
    polyShader->setVec4("releaseColour", releaseColor.r, releaseColor.g, releaseColor.b, releaseColor.a);
    polyShader->setBool("drawReleases", drawReleases);

    fixData* fData = brd->fixtureDatas.at(renderIndex);

    polyShader->setInt("numPoints", fData->vertexCount);

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
                free(portalPositions);
                free(sideMults);
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
    if (pBody->isMirrored){
        glUniform2fv(loc, fData->vertexCount, (GLfloat*)fData->mirroredVertices);
        polyShader->setBool("isMirrored", true);
    }
    else{
	    glUniform2fv(loc, fData->vertexCount, (GLfloat*)fData->vertices);
        polyShader->setBool("isMirrored", false);
    }

    loc = glGetUniformLocation(polyShader->ID, "portals");
	glUniform4fv(loc, numPortals, (GLfloat*)portalPositions);

    loc = glGetUniformLocation(polyShader->ID, "sideMults");
	glUniform1iv(loc, numPortals, (GLint*)sideMults);

    glDrawArrays(GL_TRIANGLES, 0, (fData->vertexCount - 2) * 3);

    free(portalPositions);
    free(sideMults);
}

void Renderer::drawCircleFix(PortalBody* pBody, b2Fixture* fix, bodyRenderData* brd, int renderIndex){
    circShader->use();

    b2Color color = brd->color;

    float transparency = 1.0f;
    if (!pBody->body->IsAwake()){
        transparency /= 1.5f;
    }

    if (brd->texture){
        circShader->setFloat("lineDist", 0.0f);
        circShader->setBool("hasTexture", true);
        if (pBody->isMirrored){
            circShader->setVec2("xSides", brd->mirroredXSides.x, brd->mirroredXSides.y);
        }
        else{
            circShader->setVec2("xSides", brd->xSides.x, brd->xSides.y);
        }
        circShader->setVec2("ySides", brd->ySides.x, brd->ySides.y);
        
        glBindTexture(GL_TEXTURE_2D, brd->texture);
    }
    else{
        circShader->setFloat("lineDist", glm::max(minLineThck, 0.015f));
        circShader->setBool("hasTexture", false);
    }

    b2CircleShape* shape = (b2CircleShape*)fix->GetShape();

    circShader->setMat4("ortho", camera->ortho);

    b2Vec2 pos = pBody->body->GetPosition();
    float angle = pBody->body->GetAngle() + pBody->offsetAngle;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
    model = glm::rotate(model, angle, glm::vec3(0, 0, 1));
    circShader->setMat4("model", model);

    fixData* fData = brd->fixtureDatas.at(renderIndex);

    if (pBody->isMirrored){
        circShader->setVec2("pos", fData->mirroredCPos.x, fData->mirroredCPos.y);
        circShader->setBool("isMirrored", true);
    }
    else{
        circShader->setVec2("pos", fData->cPos.x, fData->cPos.y);
        circShader->setBool("isMirrored", false);
    }
    circShader->setFloat("size", fData->radius * 2);

    circShader->setVec4("colour", color.r, color.g, color.b, transparency);
    circShader->setVec4("outerColour", color.r, color.g, color.b, 1);

    circShader->setVec4("releaseColour", releaseColor.r, releaseColor.g, releaseColor.b, releaseColor.a);
    circShader->setBool("drawReleases", drawReleases);

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
                free(portalPositions);
                free(sideMults);
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

    glBindVertexArray(VAO);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    free(portalPositions);
    free(sideMults);
}

void Renderer::debug_Line(b2Vec2 p1, b2Vec2 p2, float thickness, b2Color color){
    lineShader->use();

    lineShader->setMat4("ortho", camera->ortho);

    lineShader->setVec4("colour", color.r, color.g, color.b, color.a);
    lineShader->setFloat("zoom", camera->zoom);

    thickness = glm::max(minLineThck, thickness);

    lineShader->setFloat("thickness", thickness);
    lineShader->setVec2("p1", p1.x, p1.y);
    lineShader->setVec2("p2", p2.x, p2.y);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::debug_Polygon(const b2Vec2* vertices, int vertexCount, float outThickness, b2Color color, b2Color outerColor){
    polyShader->use();

    polyShader->setBool("hasTexture", false);
    polyShader->setMat4("ortho", camera->ortho);

    glm::mat4 model = glm::mat4(1.0f);

    polyShader->setMat4("model", model);

    polyShader->setVec4("colour", color.r, color.g, color.b, 0.5f);
    polyShader->setVec4("outerColour", color.r, color.g, color.b, 1.0f);

    polyShader->setInt("numPoints", vertexCount);

    polyShader->setFloat("lineDist", glm::max(minLineThck, outThickness));
    polyShader->setFloat("zoom", camera->zoom);

    polyShader->setInt("numPortals", 0);

    unsigned int loc = glGetUniformLocation(polyShader->ID, "vertices");
    glUniform2fv(loc, vertexCount, (GLfloat*)vertices);

    glDrawArrays(GL_TRIANGLES, 0, (vertexCount - 2) * 3);
}

void Renderer::debug_Circle(b2Vec2 p, float r, float outThickness, b2Color color, bool isPoint, b2Color outerColor){
    circShader->use();

    circShader->setBool("hasTexture", false);
    circShader->setMat4("ortho", camera->ortho);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(p.x, p.y, 0.0f));
    circShader->setMat4("model", model);

    circShader->setVec2("pos", 0, 0);
    circShader->setFloat("size", r);
    circShader->setVec4("colour", color.r, color.g, color.b, color.a);
    circShader->setVec4("outerColour", outerColor.r, outerColor.g, outerColor.b, outerColor.a);

    circShader->setFloat("lineDist", glm::max(minLineThck, outThickness));
    circShader->setFloat("zoom", camera->zoom);
    circShader->setBool("isPoint", isPoint);

    circShader->setInt("numPortals", 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
