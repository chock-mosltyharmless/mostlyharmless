#pragma once

// Matrix class, mainly for camera projections and stuff.
// The matrix is row major?
class Matrix
{
public: // methods
	void lookAt(float eyeX, float eyeY, float eyeZ,
				float centerX, float centerY, float centerZ,
				float upX, float upY, float upZ);
	// Maybe I need a 3->4 or a 4->4 version...
	void transform(float input[3], float output[3]);
	// This is the wrapper around glVertex3f
	// TODO: This is debug only!!!
	void vertex3f(float x, float y, float z);

public: // data
	float data[4][4];
};

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