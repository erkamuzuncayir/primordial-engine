#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

// Those defined in CMakeLists.txt
// GLM_FORCE_LEFT_HANDED
// GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace PE::Math {
using Vector2	 = glm::vec2;
using Vector3	 = glm::vec3;
using Vector4	 = glm::vec4;
using Matrix3	 = glm::mat3;
using Matrix4	 = glm::mat4;
using Quaternion = glm::quat;

// --- Static Members --- //
static const Vector3   Vector3Zero	  = Vector3(0.0f, 0.0f, 0.0f);
static const Vector3   Vector3One	  = Vector3(1.0f, 1.0f, 1.0f);
static const Vector3   Vector3Up	  = Vector3(0.0f, 1.0f, 0.0f);
static const Vector3   Vector3Right	  = Vector3(1.0f, 0.0f, 0.0f);
static const Vector3   Vector3Forward = Vector3(0.0f, 0.0f, 1.0f);
static constexpr float Infinity		  = FLT_MAX;
static constexpr float PI			  = 3.1415926535f;
static constexpr float TAU			  = 2.0f * PI;

// --- Angle Operations ---
inline float Radians(const float degrees) { return glm::radians(degrees); }
inline float Degrees(const float radians) { return glm::degrees(radians); }

// --- Float Operations ---
inline float Abs(const float x) { return glm::abs(x); }

// --- Vector Operations ---
inline Vector3 Vec3Radians(const Vector3 degrees) {
	return {glm::radians(degrees.x), glm::radians(degrees.y), glm::radians(degrees.z)};
};

inline Vector3 Vec3Degrees(const Vector3 radians) {
	return {glm::degrees(radians.x), glm::degrees(radians.y), glm::degrees(radians.z)};
};

inline float   Dot(const Vector3 &x, const Vector3 &y) { return glm::dot(x, y); }
inline Vector3 Cross(const Vector3 &x, const Vector3 &y) { return glm::cross(x, y); }
inline Vector3 Normalize(const Vector3 &x) { return glm::normalize(x); }
inline float   Length(const Vector3 &x) { return glm::length(x); }
inline float   Vector3Distance(const Vector3 &p0, const Vector3 &p1) { return glm::distance(p0, p1); }
inline float   LengthSq(const Vector3 &x) { return glm::length2(x); }

// --- Matrix Operations ---
inline Matrix4 Matrix4Identity() { return {1.0f}; }
inline Matrix4 Transpose(const Matrix4 &m) { return glm::transpose(m); }
inline Matrix4 Inverse(const Matrix4 &m) { return glm::inverse(m); }
inline Matrix4 InverseTranspose(const Matrix4 &M);
inline Matrix4 Translate(const Matrix4 &m, const Vector3 &v) { return glm::translate(m, v); }
inline Matrix4 Rotate(const Matrix4 &m, const float angle, const Vector3 &axis) { return glm::rotate(m, angle, axis); }
inline Matrix4 Scale(const Matrix4 &m, const Vector3 &v) { return glm::scale(m, v); }

inline Matrix4 Perspective(const float fovY, const float aspect, const float nearZ, const float farZ) {
	return glm::perspective(fovY, aspect, nearZ, farZ);
}

inline Matrix4 Mat4Ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
	return glm::ortho(left, right, bottom, top, zNear, zFar);
}

inline Matrix4 Mat4LookAt(const Vector3 &eye, const Vector3 &center, const Vector3 &up) {
	return glm::lookAt(eye, center, up);
}

struct DirectionVectors {
	Vector3 Right;
	Vector3 Up;
	Vector3 Forward;
};

inline Vector3 CalculateForwardVector(const Vector3 &rotation) {
	Matrix4 rotMat = Matrix4Identity();
	rotMat		   = Rotate(rotMat, rotation.y, Vector3(0, 1, 0));	// Yaw
	rotMat		   = Rotate(rotMat, rotation.x, Vector3(1, 0, 0));	// Pitch
	rotMat		   = Rotate(rotMat, rotation.z, Vector3(0, 0, 1));	// Roll

	return {rotMat[2]};
}

inline Vector3 CalculateRightVector(const Vector3 &rotation) {
	Matrix4 rotMat = Matrix4Identity();
	rotMat		   = Rotate(rotMat, rotation.y, Vector3(0, 1, 0));
	rotMat		   = Rotate(rotMat, rotation.x, Vector3(1, 0, 0));
	rotMat		   = Rotate(rotMat, rotation.z, Vector3(0, 0, 1));

	return {rotMat[0]};
}

inline Vector3 CalculateUpVector(const Vector3 &rotation) {
	Matrix4 rotMat = Matrix4Identity();
	rotMat		   = Rotate(rotMat, rotation.y, Vector3(0, 1, 0));
	rotMat		   = Rotate(rotMat, rotation.x, Vector3(1, 0, 0));
	rotMat		   = Rotate(rotMat, rotation.z, Vector3(0, 0, 1));

	return {rotMat[1]};
}

inline DirectionVectors CalculateDirectionVectors(const Vector3 &rotation) {
	Matrix4 rotMat = Matrix4Identity();
	rotMat		   = Rotate(rotMat, rotation.y, Vector3(0, 1, 0));
	rotMat		   = Rotate(rotMat, rotation.x, Vector3(1, 0, 0));
	rotMat		   = Rotate(rotMat, rotation.z, Vector3(0, 0, 1));

	return {
		Vector3(rotMat[0]),	 // Right
		Vector3(rotMat[1]),	 // Up
		Vector3(rotMat[2])	 // Forward
	};
}

// HELPER FUNCTIONS
template <typename T>
static T Min(const T &a, const T &b) {
	return a < b ? a : b;
}

template <typename T>
static T Max(const T &a, const T &b) {
	return a > b ? a : b;
}

template <typename T>
static T Lerp(const T &a, const T &b, float t) {
	return a + (b - a) * t;
}

template <typename T>
static T Clamp(const T &x, const T &low, const T &high) {
	if (x < low) return low;
	if (x > high) return high;
	return x;
}
}  // namespace PE::Math