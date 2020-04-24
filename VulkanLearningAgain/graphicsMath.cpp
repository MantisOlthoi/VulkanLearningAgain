#include <immintrin.h>
#include <xmmintrin.h>
#include <math.h>
#include <stdint.h>
#include "graphicsMath.h"

GraphicsMath::Matrix GraphicsMath::Matrix::frustrum(float left, float right, float bottom, float top, float nearDepth, float farDepth)
{
	// Using the glFrustrum definition found at https://www.glprogramming.com/red/appendixf.html
	//   Swapped Top and Bottom though to match that Vulkan assumes 0,0 is the top left, not the bottom left.
	GraphicsMath::Matrix outMat = {
		2 * nearDepth / (right - left), // 0,0
		0, // 0,1
		(right + left) / (right - left), // 0,2
		0, // 0,3

		0, // 1,0
		2 * nearDepth / (bottom - top), // 1,1
		(bottom + top) / (bottom - top), // 1,2
		0, // 1,3

		0, // 2,0
		0, // 2,1
		-1 * (farDepth + nearDepth) / (farDepth - nearDepth), // 2,2
		-2 * farDepth * nearDepth / (farDepth - nearDepth), // 2,3

		0, // 3,0
		0, // 3,1
		-1.0f, // 3,2
		0 // 3,3
	};

	return outMat;
}

GraphicsMath::Matrix GraphicsMath::Matrix::orthographic(float left, float right, float bottom, float top, float nearDepth, float farDepth)
{
	// Using glOrtho definition found at https://www.glprogramming.com/red/appendixf.html
	//   Swapped Top and Bottom though to match that Vulkan assumes 0,0 is the top left, not the bottom left.
	GraphicsMath::Matrix outMat = {
		2 / (right - left), // 0,0
		0, // 0,1
		0, // 0,2
		(right + left) /  (right - left), // 0,3

		0, // 1,0
		2 / (bottom - top), // 1,1
		0, // 1,2
		(bottom + top) / (bottom - top), // 1,3

		0, // 2,0
		0, // 2,1
		-2 / (farDepth - nearDepth), // 2,2
		(farDepth + nearDepth) / (farDepth - nearDepth), // 2,3

		0, // 3,0
		0, // 3,1
		0, // 3,2
		1.0f, // 3,3
	};
	return outMat;
}

GraphicsMath::Matrix GraphicsMath::Matrix::operator*(const GraphicsMath::Matrix &other)
{
	// Manually using SSE to speed things up faster than the usual Release optimizations do
	//	on the naive method.
	// Swapped the normal i,j ordering for speed because of the need to stride the load on 'other'.
	GraphicsMath::Matrix outMatrix;
	for (uint32_t j = 0; j < 4; j++)
	{
		__m128 bCol = _mm_set_ps(other.m[j+0], other.m[j+4], other.m[j+8], other.m[j+12]);
		for (uint32_t i = 0; i < 4; i++)
		{
			__m128 aRow = _mm_load_ps(&m[i * 4]);
			aRow = _mm_dp_ps(aRow, bCol, 0xFF);
			_mm_store_ss(&outMatrix.m[i * 4 + j], aRow);
		}
	}

	return outMatrix;
}

void GraphicsMath::Matrix::rotate(float yaw, float pitch, float roll)
{
	// Going to be lazy here for a moment and generate the three rotational matrices and combine them.
	// TODO: Convert this to using quaternions later on.
	GraphicsMath::Matrix yawMatrix = {
		cosf(yaw), 0, sinf(yaw), 0,
		0, 1, 0, 0,
		-sinf(yaw), 0, cosf(yaw), 0,
		0, 0, 0, 1
	};
	GraphicsMath::Matrix pitchMatrix = {
		1, 0, 0, 0,
		0, cosf(pitch), -sinf(pitch), 0,
		0, sinf(pitch), cosf(pitch), 0,
		0, 0, 0, 1
	};
	GraphicsMath::Matrix rollMatrix = {
		cosf(roll), -sinf(roll), 0, 0,
		sinf(roll), cosf(roll), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	*this = rollMatrix * pitchMatrix * yawMatrix * (*this);
}
