#pragma once

float frand();

void normalize(float *vector, int dimension);

// dot product
float dot(float *a, float *b, int dimension);

// change vector so that it is normal again.
// Both must be normalized.
// TODO: will fail, if they go the same way.
void reNormal(float *vec, float *normal, int dimension);

// only for unit length vectors!
// source and result may be the same.
void slerp(float *source, float *dest, float *result, int dimension, float t);

// polygonal interpolation using 4 Stuetzstellen.
// TODO: This should be done properly with some sort of spline.
// Derivation of the interpolator has 0 at -1, 0, 1
// ==> f'(x) ~ (x-1)(x+1)x = (x^2-1)x = x^3 - x
// ==> f(x) ~ x^4/4 - x^2/2 - b
void interpolation(float *source, float *dest, float *result, int dimension, float t);