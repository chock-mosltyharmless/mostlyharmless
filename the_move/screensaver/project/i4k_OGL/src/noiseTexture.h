#pragma once

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"

#if !defined(PI)
#define PI 3.141592
#endif


// Name of the 32x32x32 noise texture
//#define FLOAT_TEXTURE
//#define NOISE_TEXTURE_SIZE 16 // try smaller?
#define NOISE_TEXTURE_SIZE 256
#define FILTER_KERNEL_SIZE 32

extern GLuint noiseTexture;
extern float noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];
extern unsigned char noiseIntData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];

float frand(void);
void generateNoiseTexture(void);
