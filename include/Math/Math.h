#pragma once

#include <cfloat>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

// Those defined in CMakeLists.txt
// GLM_FORCE_LEFT_HANDED
// GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace PE::Math {
// ============================================================================
// 1. GENERAL / GRAPHICS MATH TYPES (Strictly 32-bit / Float)
// ============================================================================
using Vec2	= glm::vec2;
using Vec3	= glm::vec3;
using Vec4	= glm::vec4;
using Mat33 = glm::mat3;
using Mat44 = glm::mat4;
using Quat	= glm::quat;

static constexpr float Infinity = FLT_MAX;
static constexpr float PI		= 3.1415926535f;
static constexpr float TAU		= 2.0f * PI;

static const Vec3 Vec3Zero	  = Vec3(0.0f, 0.0f, 0.0f);
static const Vec3 Vec3One	  = Vec3(1.0f, 1.0f, 1.0f);
static const Vec3 Vec3Up	  = Vec3(0.0f, 1.0f, 0.0f);
static const Vec3 Vec3Right	  = Vec3(1.0f, 0.0f, 0.0f);
static const Vec3 Vec3Forward = Vec3(0.0f, 0.0f, 1.0f);

inline Mat44 Mat44Identity() { return Mat44(1.0f); }
// ============================================================================
// 2. PHYSICS MATH TYPES (Toggled Precision - 'R' Prefix)
// ============================================================================
#ifdef PE_PHYSICS_USE_DOUBLE_PRECISION
using real	 = double;
using RVec2	 = glm::dvec2;
using RVec3	 = glm::dvec3;
using RVec4	 = glm::dvec4;
using RMat33 = glm::dmat3;
using RMat44 = glm::dmat4;
using RQuat	 = glm::dquat;

static constexpr real RInfinity = DBL_MAX;
#else
using real	 = float;
using RVec2	 = glm::vec2;
using RVec3	 = glm::vec3;
using RVec4	 = glm::vec4;
using RMat33 = glm::mat3;
using RMat44 = glm::mat4;
using RQuat	 = glm::quat;

static constexpr real RInfinity = FLT_MAX;
#endif

static constexpr real RPI  = static_cast<real>(3.14159265358979323846);
static constexpr real RTAU = static_cast<real>(2.0) * RPI;

static const RVec3 RVec3Zero	= RVec3(static_cast<real>(0.0));
static const RVec3 RVec3One		= RVec3(static_cast<real>(1.0));
static const RVec3 RVec3Up		= RVec3(static_cast<real>(0.0), static_cast<real>(1.0), static_cast<real>(0.0));
static const RVec3 RVec3Right	= RVec3(static_cast<real>(1.0), static_cast<real>(0.0), static_cast<real>(0.0));
static const RVec3 RVec3Forward = RVec3(static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(1.0));

inline RMat44 RMat44Identity() { return RMat44(1.0f); }
// ============================================================================
// 3. UNIFIED TEMPLATED MATH OPERATIONS
// ============================================================================

// --- Scalar & Vector Operations ---
template <typename T>
auto Radians(const T &val) {
	return glm::radians(val);
}

template <typename T>
auto Degrees(const T &val) {
	return glm::degrees(val);
}

template <typename T>
auto Abs(const T &val) {
	return glm::abs(val);
}

template <typename VecT>
auto Dot(const VecT &x, const VecT &y) {
	return glm::dot(x, y);
}

template <typename VecT>
auto Cross(const VecT &x, const VecT &y) {
	return glm::cross(x, y);
}

template <typename VecT>
auto Normalize(const VecT &x) {
	return glm::normalize(x);
}

template <typename VecT>
auto Length(const VecT &x) {
	return glm::length(x);
}

template <typename VecT>
auto Distance(const VecT &p0, const VecT &p1) {
	return glm::distance(p0, p1);
}

template <typename VecT>
auto LengthSq(const VecT &x) {
	return glm::length2(x);
}

// --- Matrix Operations ---
template <typename MatT>
auto Transpose(const MatT &m) {
	return glm::transpose(m);
}

template <typename MatT>
auto Inverse(const MatT &m) {
	return glm::inverse(m);
}

template <typename MatT>
MatT InverseTranspose(const MatT &M) {
	MatT A		 = M;
	using Scalar = MatT::value_type;
	A[3]		 = glm::vec<4, Scalar, glm::defaultp>(0, 0, 0, 1);
	return glm::transpose(glm::inverse(A));
}

template <typename MatT, typename VecT>
auto Translate(const MatT &m, const VecT &v) {
	return glm::translate(m, v);
}

template <typename MatT, typename Scalar, typename VecT>
auto Rotate(const MatT &m, const Scalar angle, const VecT &axis) {
	return glm::rotate(m, angle, axis);
}

template <typename MatT, typename VecT>
auto Scale(const MatT &m, const VecT &v) {
	return glm::scale(m, v);
}

// --- Camera & Projection Matrices ---
template <typename Scalar>
auto Perspective(const Scalar fovY, const Scalar aspect, const Scalar nearZ, const Scalar farZ) {
	return glm::perspective(fovY, aspect, nearZ, farZ);
}

template <typename Scalar>
auto Mat4Ortho(Scalar left, Scalar right, Scalar bottom, Scalar top, Scalar zNear, Scalar zFar) {
	return glm::ortho(left, right, bottom, top, zNear, zFar);
}

template <typename VecT>
auto Mat4LookAt(const VecT &eye, const VecT &center, const VecT &up) {
	return glm::lookAt(eye, center, up);
}

// --- Direction Vectors ---
template <typename VecT>
struct DirectionVectors {
	VecT Right;
	VecT Up;
	VecT Forward;
};

template <typename VecT>
VecT CalculateForwardVector(const VecT &rotation) {
	using Scalar = typename VecT::value_type;
	glm::mat<4, 4, Scalar, glm::defaultp> rotMat(1.0);
	rotMat = Rotate(rotMat, rotation.y, VecT(Scalar(0), Scalar(1), Scalar(0)));
	rotMat = Rotate(rotMat, rotation.x, VecT(Scalar(1), Scalar(0), Scalar(0)));
	rotMat = Rotate(rotMat, rotation.z, VecT(Scalar(0), Scalar(0), Scalar(1)));
	return VecT(rotMat[2]);
}

template <typename VecT>
VecT CalculateRightVector(const VecT &rotation) {
	using Scalar = typename VecT::value_type;
	glm::mat<4, 4, Scalar, glm::defaultp> rotMat(1.0);
	rotMat = Rotate(rotMat, rotation.y, VecT(Scalar(0), Scalar(1), Scalar(0)));
	rotMat = Rotate(rotMat, rotation.x, VecT(Scalar(1), Scalar(0), Scalar(0)));
	rotMat = Rotate(rotMat, rotation.z, VecT(Scalar(0), Scalar(0), Scalar(1)));
	return VecT(rotMat[0]);
}

template <typename VecT>
VecT CalculateUpVector(const VecT &rotation) {
	using Scalar = typename VecT::value_type;
	glm::mat<4, 4, Scalar, glm::defaultp> rotMat(1.0);
	rotMat = Rotate(rotMat, rotation.y, VecT(Scalar(0), Scalar(1), Scalar(0)));
	rotMat = Rotate(rotMat, rotation.x, VecT(Scalar(1), Scalar(0), Scalar(0)));
	rotMat = Rotate(rotMat, rotation.z, VecT(Scalar(0), Scalar(0), Scalar(1)));
	return VecT(rotMat[1]);
}

template <typename VecT>
DirectionVectors<VecT> CalculateDirectionVectors(const VecT &rotation) {
	using Scalar = typename VecT::value_type;
	glm::mat<4, 4, Scalar, glm::defaultp> rotMat(1.0);
	rotMat = Rotate(rotMat, rotation.y, VecT(Scalar(0), Scalar(1), Scalar(0)));
	rotMat = Rotate(rotMat, rotation.x, VecT(Scalar(1), Scalar(0), Scalar(0)));
	rotMat = Rotate(rotMat, rotation.z, VecT(Scalar(0), Scalar(0), Scalar(1)));

	return {VecT(rotMat[0]), VecT(rotMat[1]), VecT(rotMat[2])};
}

// --- Core Math Helpers ---
template <typename T>
static T Min(const T &a, const T &b) {
	return a < b ? a : b;
}

template <typename T>
static T Max(const T &a, const T &b) {
	return a > b ? a : b;
}

template <typename T, typename Scalar = T>
static T Lerp(const T &a, const T &b, Scalar t) {
	return a + (b - a) * t;
}

template <typename T>
static T Clamp(const T &x, const T &low, const T &high) {
	if (x < low) return low;
	if (x > high) return high;
	return x;
}
}  // namespace PE::Math