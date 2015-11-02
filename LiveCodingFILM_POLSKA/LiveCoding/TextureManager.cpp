#include "StdAfx.h"
#include "TextureManager.h"
#include "Configuration.h"
#include "glext.h"
#include "gl/glu.h"
#include "GLNames.h"

// Kinect Header files
#include <Kinect.h>

static float noiseData[TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4];
static unsigned char noiseIntData[TM_NOISE_TEXTURE_SIZE * TM_NOISE_TEXTURE_SIZE * 4];
static float noiseData3D[TM_3DNOISE_TEXTURE_SIZE * TM_3DNOISE_TEXTURE_SIZE * TM_3DNOISE_TEXTURE_SIZE * 4];

TextureManager::TextureManager(void)
{
	numTextures = 0;
	kinect_sensor_ = NULL;
	depth_frame_reader_ = NULL;
	cpu_depth_sensor_buffer_ = NULL;
}

TextureManager::~TextureManager(void)
{
	releaseAll();
}

void TextureManager::releaseAll(void)
{
	glDeleteTextures(numTextures, textureID);
	numTextures = 0;

	// Release kinect stuff
	if (depth_frame_reader_) {
		depth_frame_reader_->Release();
		depth_frame_reader_ = NULL;
	}

	// close the Kinect Sensor
	if (kinect_sensor_) {
		kinect_sensor_->Close();
		kinect_sensor_->Release();
		kinect_sensor_ = NULL;
	}

	if (cpu_depth_sensor_buffer_) {
		delete[] cpu_depth_sensor_buffer_;
		cpu_depth_sensor_buffer_ = NULL;
	}
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

int TextureManager::init(char *errorString)
{
	// Free everything if there was something before.
	releaseAll();

	int retVal = initializeDefaultSensor(errorString);
	if (retVal != 0) return retVal;

	// create small and large rendertarget texture
	createRenderTargetTexture(errorString, X_OFFSCREEN, Y_OFFSCREEN, TM_OFFSCREEN_NAME);
	createRenderTargetTexture(errorString, X_HIGHLIGHT, Y_HIGHLIGHT, TM_HIGHLIGHT_NAME);
	createNoiseTexture(errorString, TM_NOISE_NAME);
	createNoiseTexture3D(errorString, TM_NOISE3D_NAME);

	// Create texture for depth sensor (kinect)
	CreateSensorTexture(errorString, TM_DEPTH_SENSOR_NAME);

	// Go throught the textures directory and load all textures.
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

	return 0;
}

int TextureManager::getTextureID(const char *name, GLuint *id, char *errorString)
{
	int texture_index;
	bool texture_found = false;
	for (int i = 0; i < numTextures && !texture_found; i++)
	{
		if (strcmp(name, textureName[i]) == 0)
		{
			*id = textureID[i];
			texture_index = i;
			texture_found = true;
		}
	}

	if (!texture_found) {
		sprintf_s(errorString, MAX_ERROR_LENGTH,
			"Could not find texture '%s'", name);
		return -1;
	} else {
		// It may be necessary to update the texture
		if (strcmp(name, TM_DEPTH_SENSOR_NAME) == 0) {
			return UpdateSensorTexture(errorString, texture_index);
		}
	}

	// id was correctly set at this point.
	return 0;
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

int TextureManager::createNoiseTexture3D(char *errorString, const char *name)
{
	// create noise Texture
	for (int i = 0; i < TM_3DNOISE_TEXTURE_SIZE * TM_3DNOISE_TEXTURE_SIZE * TM_3DNOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData3D[i] = frand() - 0.5f;
	}

	glGenTextures(1, textureID + numTextures);
	strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, name);
	glBindTexture(GL_TEXTURE_3D, textureID[numTextures]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F,
				 TM_3DNOISE_TEXTURE_SIZE, TM_3DNOISE_TEXTURE_SIZE, TM_3DNOISE_TEXTURE_SIZE,
				 0, GL_RGBA, GL_FLOAT, noiseData3D);
	textureWidth[numTextures] = -1;
	textureHeight[numTextures] = -1;
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
		filterKernel[i] = 1.0f - (float)cos(position);
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

int TextureManager::initializeDefaultSensor(char *errorMessage)
{
	HRESULT hr;

	sprintf_s(errorMessage, MAX_ERROR_LENGTH, "");

	hr = GetDefaultKinectSensor(&kinect_sensor_);
	if (FAILED(hr))
	{
		sprintf_s(errorMessage, MAX_ERROR_LENGTH, "Could not initialize Kinect");
		kinect_sensor_ = NULL;
		return -1;
	}

	if (kinect_sensor_)
	{
		// Initialize the Kinect and get the depth reader
		IDepthFrameSource* depthFrameSource = NULL;

		hr = kinect_sensor_->Open();

		if (SUCCEEDED(hr))
		{
			hr = kinect_sensor_->get_DepthFrameSource(&depthFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = depthFrameSource->OpenReader(&depth_frame_reader_);
		}

		if (depthFrameSource)
		{
			depthFrameSource->Release();
			depthFrameSource = NULL;
		}
	}

	if (!kinect_sensor_ || FAILED(hr))
	{	
		sprintf_s(errorMessage, MAX_ERROR_LENGTH,
			"Could not open kinect depth reader");
		return -1;
	}

	return 0;
}

int TextureManager::CreateSensorTexture(char *errorString, const char *name) {
	if (!depth_frame_reader_) {
		sprintf_s(errorString, MAX_ERROR_LENGTH,
			"No depth sensor exists for texture creation");
		return -1;
	}

	glGenTextures(1, textureID + numTextures);
	strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, name);
	glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
	//	TM_NOISE_TEXTURE_SIZE, TM_NOISE_TEXTURE_SIZE,
	//	GL_BGRA, GL_UNSIGNED_BYTE, noiseIntData);

	IDepthFrame* pDepthFrame = NULL;
	HRESULT hr;

	bool hasSucceeded = false;
	for (int tries = 0; tries < 20 && !hasSucceeded; tries++) {
		Sleep(100);
		hr = depth_frame_reader_->AcquireLatestFrame(&pDepthFrame);
		if (SUCCEEDED(hr)) hasSucceeded = true;
	}
	if (!hasSucceeded) {
		sprintf_s(errorString, MAX_ERROR_LENGTH,
			"Could not acquire last depth sensor frame");
		return -1;
	}

	pDepthFrame->get_RelativeTime(&last_frame_time_);

	IFrameDescription* pFrameDescription = NULL;
	int nWidth = 0;
	int nHeight = 0;

	hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
	if (FAILED(hr)) {
		pDepthFrame->Release();
		sprintf_s(errorString, MAX_ERROR_LENGTH,
			"Could not get Depth Frame description");
		return -1;
	}

	pFrameDescription->get_Width(&nWidth);
	pFrameDescription->get_Height(&nHeight);

	if (cpu_depth_sensor_buffer_) delete[] cpu_depth_sensor_buffer_;
	cpu_depth_sensor_buffer_ = new unsigned char[nWidth * nHeight];
	memset(cpu_depth_sensor_buffer_, 0, nWidth * nHeight);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
		nWidth, nHeight,
		0, GL_RED, GL_UNSIGNED_BYTE, cpu_depth_sensor_buffer_);

	textureWidth[numTextures] = nWidth;
	textureHeight[numTextures] = nHeight;
	numTextures++;

	pFrameDescription->Release();
	pDepthFrame->Release();

	return 0;
}

int TextureManager::UpdateSensorTexture(char *error_string, GLuint texture_index) {
	HRESULT hr;
	glBindTexture(GL_TEXTURE_2D, texture_index);
	int width = textureWidth[texture_index];
	int height = textureHeight[texture_index];

	if (!kinect_sensor_) {
		// No kinect sensor present - just ignore!
		return 0;
	}

	IDepthFrame *depth_frame_interface = NULL;
	hr = depth_frame_reader_->AcquireLatestFrame(&depth_frame_interface);
	if (FAILED(hr)) {
		// Could not aquire new frame -> just ignore
		return 0;
	}

	INT64 current_frame_time;
	depth_frame_interface->get_RelativeTime(&current_frame_time);
	if (current_frame_time == last_frame_time_) {
		// Frame did not change --> ignore
		depth_frame_interface->Release();
		return 0;
	}
	last_frame_time_ = current_frame_time;

	UINT buffer_size = 0;
	UINT16 *buffer = NULL;

	hr = depth_frame_interface->AccessUnderlyingBuffer(&buffer_size, &buffer);
	// Transform it smartly into 8 Bit.
	UINT16 *source_pointer = buffer;
	unsigned char *dest_pointer = cpu_depth_sensor_buffer_;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			*dest_pointer = (unsigned char)((*source_pointer) >> 2);
			source_pointer++;
			dest_pointer++;
		}
	}
	
	depth_frame_interface->Release();

	// Send data to GPU
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0, width, height,
		GL_RED, GL_UNSIGNED_BYTE, cpu_depth_sensor_buffer_);

	return 0;
}