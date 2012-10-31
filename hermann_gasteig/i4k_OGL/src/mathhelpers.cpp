#include <math.h>
#include "mathhelpers.h"

int rand();
float frand();

void normalize(float *vector, int dimension)
{
	float length = 0.0f;
	for (int i = 0; i < dimension; i++)
	{
		length += vector[i] * vector[i];
	}
	length = sqrtf(length);

	if (length > 0.0001f)
	{
		for (int i = 0; i < dimension; i++)
		{
			vector[i] = vector[i] / length;
		}
	}
	else
	{
		vector[0] = 1.0f;
		for (int i = 1; i < dimension; i++)
		{
			vector[i] = 0.0f;
		}
	}
}

// dot product
float dot(float *a, float *b, int dimension)
{
	float result = 0.0f;
	for (int i = 0; i < dimension; i++)
	{
		result += a[i] * b[i];
	}
	return result;
}

// change vector so that it is normal again.
// Both must be normalized.
// TODO: will fail, if they go the same way.
void reNormal(float *vec, float *normal, int dimension)
{
	float normalizer = dot(vec, normal, dimension);

	for (int i = 0; i < dimension; i++)
	{
		vec[i] -= normalizer * normal[i];
	}
	normalize(vec, dimension);
}

// only for unit length vectors!
// source and result may be the same.
void slerp(float *source, float *dest, float *result, int dimension, float t)
{
	// fake!
	for (int i = 0; i < dimension; i++)
	{
		result[i] = (1.0f - t) * source[i] + t * dest[i];
	}

	normalize(result, dimension);
}

//f(x) = x^4/2 - x^2 + 1/2
static float polynomial(float t)
{
	t = t*t;
	return t*t*0.5f - t + 0.5f;

	// adjustment for polynomial summation error
	//t = t * 0.939f;
	// summation error is normalized explicitly
}

// polygonal interpolation using 4 Stuetzstellen.
// TODO: This should be done properly with some sort of spline.
// Derivation of the interpolator has 0 at -1, 0, 1
// ==> f'(x) ~ (x-1)(x+1)x = (x^2-1)x = x^3 - x
// ==> f(x) ~ x^4/4 - x^2/2 - b
// f(-1) = 0, f(1) = 0 => 1/4 - 1/2 - b = 0
// Scaled result: f(x) = x^4/2 - x^2 + 1/2
// This thing is going to explode if far away for 0.
void interpolation(float *source1, float *source2, float *source3, float *source4,
				   float *result, int dimension, float t)
{
	float p1 = polynomial(0.5f * t + 0.5f);
	float p2 = polynomial(0.5f * t);
	float p3 = polynomial(0.5f * t - 0.5f);
	float p4 = polynomial(0.5f * t - 1.0f);

	float P = p1 + p2 + p3 + p4;
	P = 1.0f / P;

	// reset to 0
	for (int i = 0; i < dimension; i++)
	{
		result[i] = 0.0f;
	}

	// influence of leftmost part
	for (int i = 0; i < dimension; i++)
	{
		result[i] += source1[i] * p1 * P;
	}

	// influence of left part
	for (int i = 0; i < dimension; i++)
	{
		result[i] += source2[i] * p2 * P;
	}

	// influence of right part
	for (int i = 0; i < dimension; i++)
	{
		result[i] += source3[i] * p3 * P;
	}

	// influence of rightmost part
	for (int i = 0; i < dimension; i++)
	{
		result[i] += source4[i] * p4 * P;
	}
}