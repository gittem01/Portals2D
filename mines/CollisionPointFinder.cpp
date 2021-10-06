#include "Portal.h"

std::vector<b2Vec2> Portal::getCollisionPoints(b2Fixture*& fix1, b2Fixture*& fix2) {
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
		return collidePolygonOther(fix2, fix1); // for now
	}
	if (type1 == b2Shape::e_circle && type2 == b2Shape::e_circle) {
		return collideCircleCircle(fix1, fix2);
	}
	else {
		return returnVector;
	}
}

std::vector<b2Vec2> Portal::collideCircleCircle(b2Fixture*& fix1, b2Fixture*& fix2) {
	std::vector<b2Vec2> returnVector;
	return returnVector;
}

std::vector<b2Vec2> Portal::collidePolygonPolygon(b2Fixture*& fix1, b2Fixture*& fix2) {
	std::vector<b2Vec2> returnVector;

	return returnVector;
}

std::vector<b2Vec2> Portal::collidePolygonOther(b2Fixture*& fix1, b2Fixture*& fix2) {
	std::vector<b2Vec2> returnVector;

	return returnVector;
}

std::vector<b2Vec2> Portal::collideEdgeOther(b2Fixture*& fix1, b2Fixture*& fix2) {
	std::vector<b2Vec2> returnVector;

	b2RayCastOutput rcOutput;
	b2RayCastInput rcInput;

	rcInput.maxFraction = 1.0f;
	rcInput.p1 = ((b2EdgeShape*)fix1->GetShape())->m_vertex1;
	rcInput.p2 = ((b2EdgeShape*)fix1->GetShape())->m_vertex2;

	bool res;
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

b2Vec2 Portal::getRayPoint(b2RayCastInput& rcInput, b2RayCastOutput& rcOutput) {
	b2Vec2 result = rcInput.p1 +
				b2Vec2(
					(rcInput.p2.x - rcInput.p1.x) * rcOutput.fraction,
					(rcInput.p2.y - rcInput.p1.y) * rcOutput.fraction
				);

	return result;
}