#include "noiseTexture.h"
#include <cstdlib>

GLuint noiseTexture;
float noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];

float frand()
{
	return (float)(rand()) / (float)RAND_MAX;
}

// normalize filter kernel
void normalizeKernel(float *kernel, int kernelLength)
{
	float sum = 0.0f;

	for (int i = 0; i < kernelLength; i++)
	{
		sum += kernel[i];
	}

	for (int i = 0; i < kernelLength; i++)
	{
		kernel[i] /= sum;
	}
}

// set min/max to [0..1]
void normalizeTexture(float *texture, int textureSize)
{
	float min[4] = {1.0e20f, 1.0e20f, 1.0e20f, 1.0e20f};
	float max[4] = {-1.0e20f, -1.0e20f, -1.0e20f, -1.0e20f};

	// estimate
	for (int i = 0; i < textureSize; i++)
	{
		for (int k = 0; k < 4; k++)
		{
			if (texture[i*4+k] < min[k]) min[k] = texture[i*4+k];
			if (texture[i*4+k] > max[k]) max[k] = texture[i*4+k];
		}
	}

	// apply
	for (int i = 0; i < textureSize; i++)
	{
		for (int k = 0; k < 4; k++)
		{
			texture[i*4+k] -= min[k];
			texture[i*4+k] /= (max[k] - min[k]);
		}
	}
}

// function that generates the 2D noise texture
void generateNoiseTexture(void)
{
	float *largeNoiseData1, *largeNoiseData2;
	float *filterKernel;
	// With copied left-right
	largeNoiseData1 = new float [NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4 * 2 * 2];
	largeNoiseData2 = new float [NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4 * 2 * 2];
	filterKernel = new float [FILTER_KERNEL_SIZE];

	// Generate white noise baseline
	for (int i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData[i] = frand();
	}

	// copy noise baseline in repeated border buffer
	for (int y = 0; y < NOISE_TEXTURE_SIZE * 2; y++)
	{
		for (int x = 0; x < NOISE_TEXTURE_SIZE * 2; x++)
		{
			int srcIdx = (y % NOISE_TEXTURE_SIZE) * NOISE_TEXTURE_SIZE * 4
					   + (x % NOISE_TEXTURE_SIZE) * 4;
			int dstIdx = y * NOISE_TEXTURE_SIZE * 4 * 2 + x * 4;
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
		}
	}

#if 0
	// Debug: Clear input
	for (int y = 0; y < NOISE_TEXTURE_SIZE; y++)
	{
		for (int x = 0; x < NOISE_TEXTURE_SIZE; x++)
		{
			int srcIdx = y * NOISE_TEXTURE_SIZE * 4 + x * 4;
			noiseData[srcIdx++] = 0.0f;
			noiseData[srcIdx++] = 0.0f;
			noiseData[srcIdx++] = 0.0f;
			noiseData[srcIdx++] = 0.0f;
		}
	}
#endif

	// genereate filter kernel
	for (int i = 0; i < FILTER_KERNEL_SIZE; i++)
	{
		float position = (float)i / (float)FILTER_KERNEL_SIZE * 2.0f * (float)PI;
		filterKernel[i] = 1.0f - cos(position);
		//filterKernel[i] = 1.0f;
	}
	normalizeKernel(filterKernel, FILTER_KERNEL_SIZE);

	float *curSrc = largeNoiseData1;
	float *curDst = largeNoiseData2;
	for (int mode = 0; mode < 2; mode++)
	{
		for (int y = 0; y < NOISE_TEXTURE_SIZE + FILTER_KERNEL_SIZE; y++)
		{
			for (int x = 0; x < NOISE_TEXTURE_SIZE; x++)
			{
				float filtered[4];
				filtered[0] = 0.0f;
				filtered[1] = 0.0f;
				filtered[2] = 0.0f;
				filtered[3] = 0.0f;

				int srcIdx = y * NOISE_TEXTURE_SIZE * 2 * 4 + x * 4;
				for (int xp = 0; xp < FILTER_KERNEL_SIZE; xp++)
				{
					filtered[0] += curSrc[srcIdx++] * filterKernel[xp];
					filtered[1] += curSrc[srcIdx++] * filterKernel[xp];
					filtered[2] += curSrc[srcIdx++] * filterKernel[xp];
					filtered[3] += curSrc[srcIdx++] * filterKernel[xp];
				}

				int dstIdx = x * NOISE_TEXTURE_SIZE * 2 * 4 + y * 4;
				curDst[dstIdx] = filtered[0];
				curDst[dstIdx+1] = filtered[0];
				curDst[dstIdx+2] = filtered[0];
				curDst[dstIdx+3] = filtered[0];
			}
		}

		curSrc = largeNoiseData2;
		curDst = largeNoiseData1;
	}

	// apply filter kernel:
#if 0
	for (int y = 0; y < NOISE_TEXTURE_SIZE * 2 - FILTER_KERNEL_SIZE; y++)
	{
		for (int x = 0; x < NOISE_TEXTURE_SIZE * 2 - FILTER_KERNEL_SIZE; x++)
		{
			float filtered[4];
			filtered[0] = 0.0f;
			filtered[1] = 0.0f;
			filtered[2] = 0.0f;
			filtered[3] = 0.0f;

			for (int yp = 0; yp < FILTER_KERNEL_SIZE; yp++)
			{
				int srcIdx = (y + yp) * NOISE_TEXTURE_SIZE * 2 * 4 + x * 4;

				filtered[0] += largeNoiseData[srcIdx] * filterKernel[yp];
				filtered[1] += largeNoiseData[srcIdx + 1] * filterKernel[yp];
				filtered[2] += largeNoiseData[srcIdx + 2] * filterKernel[yp];
				filtered[3] += largeNoiseData[srcIdx + 3] * filterKernel[yp];
			}

			int dstIdx = y * NOISE_TEXTURE_SIZE * 2 * 4 + x * 4;
			largeNoiseData[dstIdx] = filtered[0];
			largeNoiseData[dstIdx+1] = filtered[1];
			largeNoiseData[dstIdx+2] = filtered[2];
			largeNoiseData[dstIdx+3] = filtered[3];
		}
	}
	for (int y = 0; y < NOISE_TEXTURE_SIZE; y++)
	{
		for (int x = 0; x < NOISE_TEXTURE_SIZE; x++)
		{
			float filtered[4];
			filtered[0] = 0.0f;
			filtered[1] = 0.0f;
			filtered[2] = 0.0f;
			filtered[3] = 0.0f;

			for (int xp = 0; xp < FILTER_KERNEL_SIZE; xp++)
			{
				int srcIdx = y * NOISE_TEXTURE_SIZE * 2 * 4 + (x + xp) * 4;

				filtered[0] += largeNoiseData[srcIdx] * filterKernel[xp];
				filtered[1] += largeNoiseData[srcIdx + 1] * filterKernel[xp];
				filtered[2] += largeNoiseData[srcIdx + 2] * filterKernel[xp];
				filtered[3] += largeNoiseData[srcIdx + 3] * filterKernel[xp];
			}

			int dstIdx = y * NOISE_TEXTURE_SIZE * 2 * 4 + x * 4;
			largeNoiseData[dstIdx] = filtered[0];
			largeNoiseData[dstIdx+1] = filtered[0];
			largeNoiseData[dstIdx+2] = filtered[0];
			largeNoiseData[dstIdx+3] = filtered[0];
		}
	}
#endif

#if 1
	// copy back to small array
	for (int y = 0; y < NOISE_TEXTURE_SIZE; y++)
	{
		for (int x = 0; x < NOISE_TEXTURE_SIZE; x++)
		{
			int dstIdx = y * NOISE_TEXTURE_SIZE * 4 + x * 4;
			int srcIdx = y * NOISE_TEXTURE_SIZE * 4 * 2 + x * 4;

			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
		}
	}
#endif

	// normalize small Array
	normalizeTexture(noiseData, NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE);

	delete [] largeNoiseData1;
	delete [] largeNoiseData2;
	delete [] filterKernel;
}

