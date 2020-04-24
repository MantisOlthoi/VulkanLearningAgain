#pragma once

namespace GraphicsMath
{
	struct alignas(16) Matrix
	{
		float m[16];

		static Matrix frustrum(float left, float right, float bottom, float top, float nearDepth, float farDepth);
		static Matrix orthographic(float left, float right, float bottom, float top, float nearDepth, float farDepth);
		Matrix operator *(const Matrix &other);
		void rotate(float yaw, float pitch, float roll);
	};
}
