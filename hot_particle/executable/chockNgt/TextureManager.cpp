#include "StdAfx.h"
#include "TextureManager.h"
#include "Configuration.h"
#include "glext.h"
#include "gl/glu.h"

float noiseData[TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4];
unsigned char noiseIntData[TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4];
float noiseData3D[TM_3DNOISE_TEXTURE_SIZE * TM_3DNOISE_TEXTURE_SIZE * TM_3DNOISE_TEXTURE_SIZE * 4];

TextureManager::TextureManager(void)
{
	numTextures = 0;
	numVideos = 0;
}

TextureManager::~TextureManager(void)
{
	AVIFileExit();
}

void TextureManager::releaseAll(void)
{
	glDeleteTextures(numTextures, textureID);
	numTextures = 0;

	for (int i = 0; i < numVideos; i++)
	{
		AVIStreamGetFrameClose(pgf[i]);
		AVIStreamRelease(pavi[i]);
		DeleteObject(hBitmap);
	}
	DrawDibClose(hdd);
	DeleteDC(hdc);
	glDeleteTextures(numVideos, videoTextureID);
	numVideos = 0;
}

int TextureManager::loadAVI(const char *filename, char *errorString)
{
	char combinedName[TM_MAX_FILENAME_LENGTH+1];
	sprintf_s(combinedName, TM_MAX_FILENAME_LENGTH,
			  TM_DIRECTORY "%s", filename);

	if (numTextures >= TM_MAX_NUM_TEXTURES)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Too many textures.");
		return -1;
	}

	if (AVIStreamOpenFromFile(&(pavi[numVideos]), combinedName, streamtypeVIDEO, 0, OF_READ, NULL))
	{
		return -1;
	}
	AVIStreamInfo(pavi[numVideos], &(psi[numVideos]), sizeof(psi[numVideos])); // Reads stream info
	videoWidth[numVideos] = psi[numVideos].rcFrame.right - psi[numVideos].rcFrame.left;
	videoHeight[numVideos] = psi[numVideos].rcFrame.bottom - psi[numVideos].rcFrame.top;
	videoNumFrames[numVideos] = AVIStreamLength(pavi[numVideos]);

	BITMAPINFOHEADER bmih;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biPlanes = 1;
	bmih.biBitCount = 24;
	bmih.biWidth = videoWidth[numVideos];
	bmih.biHeight = videoHeight[numVideos];
	bmih.biCompression = BI_RGB;
	hBitmap[numVideos] = CreateDIBSection(hdc, (BITMAPINFO*)(&bmih), DIB_RGB_COLORS, (void**)(&(videoData[numVideos])), NULL, NULL);
	SelectObject(hdc, hBitmap);
	GdiFlush();

	pgf[numVideos] = AVIStreamGetFrameOpen(pavi[numVideos], NULL);
	if (pgf == NULL)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Could not open AVI stream for %s", filename);
		return -1;
	}

	// create openGL texture
	int textureSize = videoWidth[numVideos] * videoHeight[numVideos] * 4;
	unsigned char *textureData = new unsigned char[textureSize];
	glGenTextures(1, &videoTextureID[numVideos]);
	glBindTexture(GL_TEXTURE_2D, videoTextureID[numVideos]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				 videoWidth[numVideos], videoHeight[numVideos],
				 0, GL_BGRA, GL_UNSIGNED_BYTE, textureData);
	delete [] textureData;
	strcpy_s(videoName[numVideos], TM_MAX_FILENAME_LENGTH, filename);

	numVideos++;
	return 0;
}

int TextureManager::loadTGA(const char *filename, char *errorString)
{
	char combinedName[TM_MAX_FILENAME_LENGTH+1];

	TGAHeader tgaHeader;
	int numRead;

	sprintf_s(combinedName, TM_MAX_FILENAME_LENGTH,
			  TM_DIRECTORY "%s", filename);

	if (numTextures >= TM_MAX_NUM_TEXTURES)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Too many textures.");
		return -1;
	}

	FILE *fid;
	if (fopen_s(&fid, combinedName, "rb"))
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Could not load texture '%s'",
				  filename);
		return -1;
	}

	// load header
	numRead = fread(&tgaHeader, 1, sizeof(tgaHeader), fid);
	if (numRead != sizeof(tgaHeader))
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "TGA Header of %s too short",
			      filename);
		return -1;
	}
		
	// load image data
	int textureSize = tgaHeader.width * tgaHeader.height * 4;
	unsigned char *textureData = new unsigned char[textureSize];
	numRead = fread(textureData, 1, textureSize, fid);
	if (numRead != textureSize)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Bogus tga file: '%s'",
			      filename);
		delete [] textureData;
		return -1;
	}
		
	// create openGL texture
	glGenTextures(1, &textureID[numTextures]);
	textureWidth[numTextures] = tgaHeader.width;
	textureHeight[numTextures] = tgaHeader.height;
	glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
#if 0 // Use simple mipmap generation	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				 textureWidth[numTextures], textureHeight[numTextures],
				 0, GL_BGRA, GL_UNSIGNED_BYTE, textureData);
#else
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
					  textureWidth[numTextures], textureHeight[numTextures],
					  GL_BGRA, GL_UNSIGNED_BYTE, textureData);
#endif

	delete [] textureData;
	fclose(fid);
	strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, filename);
	numTextures++;

	return 0;
}

int TextureManager::init(char *errorString, HDC mainDC)
{
	// Free everything if there was something before.
	releaseAll();

	// Initialize video rendering
	hdd = DrawDibOpen(); // Used for scaling/drawing the avi to a RAM buffer
	hdc = CreateCompatibleDC(mainDC);
	AVIFileInit(); // Opens the AVIFile Library

	// create small and large rendertarget texture
	createRenderTargetTexture(errorString, X_OFFSCREEN, Y_OFFSCREEN, TM_OFFSCREEN_NAME);
	createRenderTargetTexture(errorString, X_HIGHLIGHT, Y_HIGHLIGHT, TM_HIGHLIGHT_NAME);
	createNoiseTexture(errorString, TM_NOISE_NAME);

	// Go throught the shaders directory and load all shaders.
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;

	// Go to first file in textures directory
	char *dirname = TM_DIRECTORY TM_SHADER_WILDCARD;
	hFind = FindFirstFile(TM_DIRECTORY TM_SHADER_WILDCARD, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "IO Error\nThere are no textures in " TM_DIRECTORY);
		return -1;
	}

	// Load all the textures in the directory
	do
	{		
		// Note that the number of textures is increased automatically
		int retVal = loadTGA(ffd.cFileName, errorString);
		if (retVal) return retVal;
	} while (FindNextFile(hFind, &ffd));

	// Go to video first file in textures directory
	hFind = INVALID_HANDLE_VALUE;
	dirname = TM_DIRECTORY TM_VIDEO_WILDCARD;
	hFind = FindFirstFile(TM_DIRECTORY TM_VIDEO_WILDCARD, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "IO Error\nThere are no videos in " TM_DIRECTORY);
		// No videos are not necessarily an error.
		return 0;
	}

	// Load all the videos in the directory
	do
	{		
		// Note that the number of videos is increased automatically
		int retVal = loadAVI(ffd.cFileName, errorString);
		if (retVal) return retVal;
	} while (FindNextFile(hFind, &ffd));

	return 0;
}

int TextureManager::getVideoID(const char *name, GLuint *id, char *errorString, int frameID)
{	
	for (int i = 0; i < numVideos; i++)
	{
		if (strcmp(name, videoName[i]) == 0)
		{
			if (frameID >= videoNumFrames[i])
			{
				int retVal = getTextureID("black.tga", id, errorString);
				if (retVal < 0) return retVal;
				else return 1; // mark finished
			}

			*id = videoTextureID[i];

			LPBITMAPINFOHEADER lpbi;
			lpbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(pgf[i], frameID % videoNumFrames[i]);
			char *pdata = (char *)lpbi + lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD); // Skip header info to get to data
			// Convert data to requested bitmap format
			int width = videoWidth[i];
			int height = videoHeight[i];
			SelectObject(hdc, hBitmap[i]);
			DrawDibDraw(hdd, hdc, 0, 0, width, height, lpbi, pdata, 0, 0, width, height, 0);
			GdiFlush();
			// create openGL texture
			glEnable(GL_TEXTURE_2D);				// Enable Texture Mapping
			glBindTexture(GL_TEXTURE_2D, *id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, videoData[i]);

			return 0;
		}
	}

	// Got here without finding a texture, return error.
	sprintf_s(errorString, MAX_ERROR_LENGTH,
			  "Could not find video '%s'", name);
	return -1;
}

int TextureManager::getTextureID(const char *name, GLuint *id, char *errorString)
{
	for (int i = 0; i < numTextures; i++)
	{
		if (strcmp(name, textureName[i]) == 0)
		{
			*id = textureID[i];
			return 0;
		}
	}

	// Got here without finding a texture, return error.
	sprintf_s(errorString, MAX_ERROR_LENGTH,
			  "Could not find texture '%s'", name);
	return -1;
}

int TextureManager::createNoiseTexture(char *errorString, const char *name)
{
	glGenTextures(1, textureID + numTextures);
	strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, name);
	glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
	generateNoiseTexture();
	makeIntTexture();
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
					  TM_NOISE_TEXTURE_SIZE, TM_NOISE_TEXTURE_SIZE,
					  GL_BGRA, GL_UNSIGNED_BYTE, noiseIntData);
	textureWidth[numTextures] = TM_NOISE_TEXTURE_SIZE;
	textureHeight[numTextures] = TM_NOISE_TEXTURE_SIZE;
	numTextures++;
	return 0;
}

int TextureManager::createRenderTargetTexture(char *errorString, int width, int height,
										      const char *name)
{
	glGenTextures(1, textureID + numTextures);
	strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, name);
	glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
		         GL_UNSIGNED_BYTE, 0);
	textureWidth[numTextures] = width;
	textureHeight[numTextures] = height;
	numTextures++;
	return 0;
}

float TextureManager::frand(void)
{
	return (float)(rand()) / (float)RAND_MAX;
}

// normalize filter kernel
void TextureManager::normalizeKernel(float *kernel, int kernelLength)
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
void TextureManager::normalizeTexture(float *texture, int textureSize)
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

void TextureManager::makeIntTexture(void)
{
	for (int i = 0; i < TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4; i++)
	{
		if (noiseData[i] < 0.0f)
		{
			noiseIntData[i] = 0;
		}
		else if (noiseData[i] >= 1.0f)
		{
			noiseIntData[i] = 255;
		}
		else
		{
			noiseIntData[i] = (unsigned char)(noiseData[i] * 256.0f);
		}
	}
}

// function that generates the 2D noise texture
void TextureManager::generateNoiseTexture(void)
{
	float *largeNoiseData1, *largeNoiseData2;
	float *filterKernel;
	// With copied left-right
	largeNoiseData1 = new float [TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4 * 2 * 2];
	largeNoiseData2 = new float [TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4 * 2 * 2];
	filterKernel = new float [TM_FILTER_KERNEL_SIZE];

	// Generate white noise baseline
	for (int i = 0; i < TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData[i] = frand();
	}

	// copy noise baseline in repeated border buffer
	for (int y = 0; y < TM_NOISE_TEXTURE_SIZE * 2; y++)
	{
		for (int x = 0; x < TM_NOISE_TEXTURE_SIZE * 2; x++)
		{
			int srcIdx = (y % TM_NOISE_TEXTURE_SIZE) * TM_NOISE_TEXTURE_SIZE * 4
					   + (x % TM_NOISE_TEXTURE_SIZE) * 4;
			int dstIdx = y * TM_NOISE_TEXTURE_SIZE * 4 * 2 + x * 4;
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
			largeNoiseData1[dstIdx++] = noiseData[srcIdx++];
		}
	}

	// genereate filter kernel
	for (int i = 0; i < TM_FILTER_KERNEL_SIZE; i++)
	{
		float position = (float)i / (float)TM_FILTER_KERNEL_SIZE * 2.0f * 3.1415926f;
		filterKernel[i] = 1.0f - cosf(position);
		//filterKernel[i] = 1.0f;
	}
	normalizeKernel(filterKernel, TM_FILTER_KERNEL_SIZE);

	float *curSrc = largeNoiseData1;
	float *curDst = largeNoiseData2;
	for (int mode = 0; mode < 2; mode++)
	{
		for (int y = 0; y < TM_NOISE_TEXTURE_SIZE + TM_FILTER_KERNEL_SIZE; y++)
		{
			for (int x = 0; x < TM_NOISE_TEXTURE_SIZE; x++)
			{
				float filtered[4];
				filtered[0] = 0.0f;
				filtered[1] = 0.0f;
				filtered[2] = 0.0f;
				filtered[3] = 0.0f;

				int srcIdx = y * TM_NOISE_TEXTURE_SIZE * 2 * 4 + x * 4;
				for (int xp = 0; xp < TM_FILTER_KERNEL_SIZE; xp++)
				{
					filtered[0] += curSrc[srcIdx++] * filterKernel[xp];
					filtered[1] += curSrc[srcIdx++] * filterKernel[xp];
					filtered[2] += curSrc[srcIdx++] * filterKernel[xp];
					filtered[3] += curSrc[srcIdx++] * filterKernel[xp];
				}

				int dstIdx = x * TM_NOISE_TEXTURE_SIZE * 2 * 4 + y * 4;
				curDst[dstIdx] = filtered[0];
				curDst[dstIdx+1] = filtered[1];
				curDst[dstIdx+2] = filtered[2];
				curDst[dstIdx+3] = filtered[3];
			}
		}

		curSrc = largeNoiseData2;
		curDst = largeNoiseData1;
	}

	// copy back to small array
	for (int y = 0; y < TM_NOISE_TEXTURE_SIZE; y++)
	{
		for (int x = 0; x < TM_NOISE_TEXTURE_SIZE; x++)
		{
			int dstIdx = y * TM_NOISE_TEXTURE_SIZE * 4 + x * 4;
			int srcIdx = y * TM_NOISE_TEXTURE_SIZE * 4 * 2 + x * 4;

			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
			noiseData[dstIdx++] = (float)largeNoiseData1[srcIdx++];
		}
	}

	// normalize small Array
	normalizeTexture(noiseData, TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE);

	delete [] largeNoiseData1;
	delete [] largeNoiseData2;
	delete [] filterKernel;

	makeIntTexture();
}
