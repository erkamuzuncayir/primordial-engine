#include <Math/Math.h>

namespace PE::Math {
inline float AngleFromXY(float x, float y) { return atan2f(y, x); }

inline Mat44 InverseTranspose(const Mat44 &M) {
	Mat44 A = M;
	A[3]	= Vec4(0.0f, 0.0f, 0.0f, 1.0f);

	return Transpose(Inverse(A));
}
}  // namespace PE::Math