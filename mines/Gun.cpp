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
}


void Gun::update() {
	float minFraction = b2_maxFloat;
	b2RayCastOutput rcOutput;
	b2RayCastInput rcInput = {pos, pos + normalize(targetPos - pos, 100.f), 1.0f};
	for (b2Body* body = world->GetBodyList(); body; body=body->GetNext()) {
		for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {
			if (fixture->IsSensor()) continue;
			bool res = fixture->RayCast(&rcOutput, rcInput, fixture->GetType());
			if (res && rcOutput.fraction < minFraction) {
				minFraction = rcOutput.fraction;
				currentTarget = fixture;
				currentFraction = rcOutput.fraction;
			}
		}
	}

	draw();
}

void Gun::draw() {
	b2Vec2 p2 = pos + normalize(targetPos - pos, 100.f * currentFraction);
	world->m_debugDraw->DrawSegment(pos, p2, b2Color(1.0f, 1.0f, 1.0f, 1.0f));
	world->m_debugDraw->DrawPoint(p2, 10.0f, b2Color(1.0f, 1.0f, 0.0f, 1.0f));
}