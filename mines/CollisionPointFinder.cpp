#include "Portal.h"

std::vector<b2Vec2> Portal::getCollisionPoints(b2Fixture* fix1, b2Fixture* fix2) {
	std::vector<b2Vec2> returnVector;

	b2Shape::Type type1 = fix1->GetShape()->GetType();
	b2Shape::Type type2 = fix2->GetShape()->GetType();

	if (type2 == b2Shape::e_edge) {
		return collideEdgeOther(fix2, fix1);
	}
	else if (type1 == b2Shape::e_polygon && type2 != b2Shape::e_polygon) {
		return collidePolygonOther(fix1, fix2);
	}
	else if (type1 != b2Shape::e_polygon && type2 == b2Shape::e_polygon) {
		return collidePolygonOther(fix2, fix1);
	}
	else if (type1 == b2Shape::e_polygon && type2 == b2Shape::e_polygon) {
		return collidePolygonOther(fix1, fix2); // for now
	}
	if (type1 == b2Shape::e_circle && type2 == b2Shape::e_circle) {
		return collideCircleCircle(fix1, fix2);
	}
	else {
		return returnVector;
	}
}

std::vector<b2Vec2> Portal::collideCircleCircle(b2Fixture* fix1, b2Fixture* fix2) {
	std::vector<b2Vec2> returnVector;

	return returnVector;
}

std::vector<b2Vec2> Portal::collidePolygonPolygon(b2Fixture* fix1, b2Fixture* fix2) {
	std::vector<b2Vec2> returnVector;

	return returnVector;
}

std::vector<b2Vec2> Portal::collidePolygonOther(b2Fixture* fix1, b2Fixture* fix2) {
	std::vector<b2Vec2> returnVector;

	b2PolygonShape* shape = (b2PolygonShape*)(fix1->GetShape());
	b2Body* body = fix1->GetBody();

	for (int i = 0; i < shape->m_count; i++) {
		int j = (i + 1) % shape->m_count; // next vertex

		b2RayCastOutput rcOutput;
		b2RayCastInput rcInput;
		rcInput.maxFraction = 1.0f;

		bool res;
		
		rcInput.p1 = body->GetWorldPoint(shape->m_vertices[i]);
		rcInput.p2 = body->GetWorldPoint(shape->m_vertices[j]);

		res = fix2->RayCast(&rcOutput, rcInput, fix2->GetBody()->GetType());
		if (res) {
			b2Vec2 result = getRayPoint(rcInput, rcOutput);
			returnVector.push_back(result);
		}

		rcInput.p1 = body->GetWorldPoint(shape->m_vertices[j]);
		rcInput.p2 = body->GetWorldPoint(shape->m_vertices[i]);
		res = fix2->RayCast(&rcOutput, rcInput, fix2->GetBody()->GetType());
		if (res) {
			b2Vec2 result = getRayPoint(rcInput, rcOutput);
			returnVector.push_back(result);
		}
	}

	// inside of the other fixture or just started colliding
	if (returnVector.size() == 0) {
		returnVector.push_back(getFixtureCenter(fix2));
	}

	return returnVector;
}

std::vector<b2Vec2> Portal::collideEdgeOther(b2Fixture* fix1, b2Fixture* fix2) {
	std::vector<b2Vec2> returnVector;

	b2RayCastOutput rcOutput;
	b2RayCastInput rcInput;
	rcInput.maxFraction = 1.0f;

	bool res;

	rcInput.p1 = ((b2EdgeShape*)fix1->GetShape())->m_vertex1;
	rcInput.p2 = ((b2EdgeShape*)fix1->GetShape())->m_vertex2;
	res = fix2->RayCast(&rcOutput, rcInput, fix2->GetBody()->GetType());
	if (res) {
		b2Vec2 result = getRayPoint(rcInput, rcOutput);
		returnVector.push_back(result);
	}

	rcInput.p1 = ((b2EdgeShape*)fix1->GetShape())->m_vertex2;
	rcInput.p2 = ((b2EdgeShape*)fix1->GetShape())->m_vertex1;
	res = fix2->RayCast(&rcOutput, rcInput, fix2->GetBody()->GetType());
	if (res) {
		b2Vec2 result = getRayPoint(rcInput, rcOutput);
		returnVector.push_back(result);
	}

	return returnVector;
}

b2Vec2 Portal::getFixtureCenter(b2Fixture* fix){
	if (fix->GetType() == b2Shape::Type::e_circle){
		b2Vec2 point = ((b2CircleShape*)fix->GetShape())->m_p;
		return fix->GetBody()->GetWorldPoint(point);
	}
	else{
		b2Vec2 avgPoints = b2Vec2(0, 0);
		b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();
		for (int i = 0; i < polyShape->m_count; i++){
			avgPoints += polyShape->m_vertices[i];
		}
		return (1.0f / polyShape->m_count) * avgPoints;
	}
}

b2Vec2 Portal::getRayPoint(b2RayCastInput& rcInput, b2RayCastOutput& rcOutput) {
	b2Vec2 result = rcInput.p1 +
				b2Vec2(
					(rcInput.p2.x - rcInput.p1.x) * rcOutput.fraction,
					(rcInput.p2.y - rcInput.p1.y) * rcOutput.fraction
				);

	return result;
}