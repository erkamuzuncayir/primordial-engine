#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body {
// Finds the closest point on a line segment (A to B) to a given Point.
static Math::RVec3 ClosestPointOnSegment(const Math::RVec3 &segmentStart, const Math::RVec3 &segmentEnd,
										 const Math::RVec3 &targetPoint) {
	// 1. The vector representing the entire line segment from start to end
	const Math::RVec3 segmentVector = segmentEnd - segmentStart;

	// 2. The vector from the start of the segment to the target point in space
	const Math::RVec3 vectorToTarget = targetPoint - segmentStart;

	// 3. Project the target point onto the segment line to find the ratio (0.0 to 1.0).
	// - Math::Dot(vectorToTarget, segmentVector) projects the target onto the segment.
	// - Math::Dot(segmentVector, segmentVector) gets the squared length of the segment.
	// - Dividing them gives us the exact percentage/ratio along the segment.
	Math::real projectionRatio = Math::Dot(vectorToTarget, segmentVector) / Math::Dot(segmentVector, segmentVector);

	// 4. Clamp the ratio between 0.0 and 1.0 to ensure the result doesn't shoot past the endpoints
	projectionRatio = Math::Clamp(projectionRatio, static_cast<Math::real>(0.0), static_cast<Math::real>(1.0));

	// 5. Calculate and return the final 3D world coordinates of the closest point
	return segmentStart + (segmentVector * projectionRatio);
}

// Finds the closest points between TWO line segments (p1-q1 and p2-q2).
static void ClosestPointsBetweenSegments(const Math::RVec3 &start1, const Math::RVec3 &end1, const Math::RVec3 &start2,
										 const Math::RVec3 &end2, Math::RVec3 &outClosest1, Math::RVec3 &outClosest2) {
	// 1. Vectors representing the direction and length of the segments
	const Math::RVec3 vec1 = end1 - start1;
	const Math::RVec3 vec2 = end2 - start2;

	// The vector between the starting points of the two segments
	const Math::RVec3 startOffset = start1 - start2;

	// 2. Dot products (for squared lengths and projections)
	const Math::real sqLen1 = Math::Dot(vec1, vec1);  // Squared length of segment 1
	const Math::real sqLen2 = Math::Dot(vec2, vec2);  // Squared length of segment 2

	const Math::real dotVec1Vec2 = Math::Dot(vec1, vec2);  // Measures how parallel the segments are

	const Math::real dotVec1Offset = Math::Dot(vec1, startOffset);	// Projection of start offset onto segment 1
	const Math::real dotVec2Offset = Math::Dot(vec2, startOffset);	// Projection of start offset onto segment 2

	// 3. Ratios (0.0 to 1.0) representing where the closest point lies on each segment
	Math::real ratio1 = 0.0f;
	Math::real ratio2 = 0.0f;

	// Denominator for the linear equations (used to check for parallel lines)
	const Math::real denominator = (sqLen1 * sqLen2) - (dotVec1Vec2 * dotVec1Vec2);

	// If segments are not parallel, calculate the closest point ratio on segment 1
	if (denominator > Math::REpsilon) {
		ratio1 = Math::Clamp((dotVec1Vec2 * dotVec2Offset - dotVec1Offset * sqLen2) / denominator,
							 static_cast<Math::real>(0.0), static_cast<Math::real>(1.0));
	} else {
		// If parallel, default to the start of segment 1 to prevent division by zero
		ratio1 = 0.0f;
	}

	// Calculate the ratio on segment 2 based on the position found on segment 1
	ratio2 = (dotVec1Vec2 * ratio1 + dotVec2Offset) / sqLen2;

	// If the point on segment 2 falls outside its boundaries, clamp it and recalculate segment 1
	if (ratio2 < 0.0f) {
		ratio2 = 0.0f;	// Clamp to the start of segment 2

		// Recalculate ratio1 since ratio2 was artificially moved
		ratio1 = Math::Clamp(-dotVec1Offset / sqLen1, static_cast<Math::real>(0.0), static_cast<Math::real>(1.0));

	} else if (ratio2 > 1.0f) {
		ratio2 = 1.0f;	// Clamp to the end of segment 2

		// Recalculate ratio1 since ratio2 was artificially moved
		ratio1 = Math::Clamp((dotVec1Vec2 - dotVec1Offset) / sqLen1, static_cast<Math::real>(0.0),
							 static_cast<Math::real>(1.0));
	}

	// Calculate the final 3D world coordinates of the closest points using the clamped ratios
	outClosest1 = start1 + (vec1 * ratio1);
	outClosest2 = start2 + (vec2 * ratio2);
}
}  // namespace PE::Physics::Body