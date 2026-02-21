#include <Math/Math.h>

namespace PE::Math {
inline float AngleFromXY(float x, float y) { return atan2f(y, x); }

inline Matrix4 InverseTranspose(const Matrix4 &M) {
	Matrix4 A = M;
	A[3]	  = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

	return Transpose(Inverse(A));
}
}  // namespace PE::Math