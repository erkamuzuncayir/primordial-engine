#pragma once
#include <random>
#include <algorithm>

#include "Math.h"

namespace PE::Math::Random {
template<typename T>
static T Get(const T min, const T max) {
	static std::random_device rd;
	static std::mt19937		  gen(rd());

	std::uniform_real_distribution<T> dis(std::min(min, max), std::max(min, max));
	return dis(gen);
};


static Vec3 GetPointInBox(const Vec3 &min, const Vec3 max) {
	static std::random_device rd;
	static std::mt19937		  gen(rd());

	std::uniform_real_distribution<float> disX(std::min(min.x, max.x), std::max(min.x, max.x));
	std::uniform_real_distribution<float> disY(std::min(min.y, max.y), std::max(min.y, max.y));
	std::uniform_real_distribution<float> disZ(std::min(min.z, max.z), std::max(min.z, max.z));

	return Vec3{disX(gen), disY(gen), disZ(gen)};
}

static Vec3 GetPointInSphere(const Vec3 center, const float radius) {
	static std::random_device			  rd;
	static std::mt19937					  gen(rd());
	std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

	float x, y, z, distanceSq;

	do {
		x		   = dis(gen);
		y		   = dis(gen);
		z		   = dis(gen);
		distanceSq = x * x + y * y + z * z;
	} while (distanceSq > 1.0f);

	return {center.x + x * radius, center.y + y * radius, center.z + z * radius};
}
}