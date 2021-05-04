#include "Gun.h"

b2Vec2 normalize(b2Vec2 vec, float factor) {
	float length = sqrt(vec.x * vec.x + vec.y * vec.y);
	if (length == 0) { return b2Vec2(0.0f, 0.0f); }

	vec.x = (vec.x * factor) / length;
	vec.y = (vec.y * factor) / length;

	return vec;
}

Gun::Gun(b2Vec2 pos, b2Vec2 targetPos, b2World* world) {
	this->pos = pos;
	this->targetPos = targetPos;
	this->world = world;
	this->currentTarget = NULL;
	this->currentFraction = 0.0f;
	this->currentNormal = b2Vec2(1.0f, 0.0f);
	this->currentCollisionPos = b2Vec2(0.0f, 0.0f);
}


void Gun::update() {
	currentFraction = 1.0f;
	b2RayCastOutput rcOutput;
	b2RayCastInput rcInput = {pos, pos + normalize(targetPos - pos, 100.f), 1.0f};
	for (b2Body* body = world->GetBodyList(); body; body=body->GetNext()) {
		for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {
			if (fixture->IsSensor()) continue;
			bool res = fixture->RayCast(&rcOutput, rcInput, fixture->GetType());
			if (res && rcOutput.fraction < currentFraction) {
				currentFraction = rcOutput.fraction;
				currentTarget = fixture;
				currentNormal = rcOutput.normal;
			}
		}
	}
	currentCollisionPos = pos + normalize(targetPos - pos, 100.f * currentFraction);
	draw();
}

void Gun::draw() {
	world->m_debugDraw->DrawSegment(pos, currentCollisionPos, b2Color(1.0f, 1.0f, 1.0f, 0.3f));
	world->m_debugDraw->DrawPoint(currentCollisionPos, 10.0f, b2Color(1.0f, 1.0f, 0.0f, 1.0f));
}