#include "StdAfx.h"
#include "CFPContainer.h"

CFPContainer::CFPContainer(void) :
	headers(NULL),
	textureGLID(NULL),
	textureWidth(NULL),
	textureHeight(NULL)
{
}

CFPContainer::~CFPContainer(void)
{
}

void CFPContainer::deInit()
{
	if (headers) delete [] headers; headers = NULL;
	if (textureGLID)
	{
		glDeleteTextures(numTextures, textureGLID);
		delete [] textureGLID; textureGLID = NULL;
	}
	if (textureWidth) delete [] textureWidth; textureWidth = NULL;
	if (textureHeight) delete [] textureHeight; textureHeight = NULL;
}

HRESULT CFPContainer::init(const char *refFileName)
{
	char line[4096];

	// read in cfp.files in order to get the number of files:
	numTextures = 0;
	FILE *fid = fopen(refFileName, "r");
	if (fid == 0) return S_FALSE;
	int retVal = 1;
	while(retVal != 0 && retVal != EOF)
	{
		retVal = fscanf(fid, "%s\n", line);
		numTextures++;
	}
	fclose(fid);
	numTextures--; // This is to correct a strange bug in my load routine!

	// allocate memory for my arrays
	deInit();
	headers = new CFPHeader[numTextures];
	textureGLID = new GLuint[numTextures];
	textureWidth = new int[numTextures];
	textureHeight = new int[numTextures];
	glGenTextures(numTextures, textureGLID);

	// read in cfp.files a second time and load all the textures
	// if it changed... screw you.
	fid = fopen(refFileName, "r");
	if (fid == 0) return S_FALSE;
	retVal = 1;
	for (int curTexture = 0; curTexture < numTextures; curTexture++)
	{
		retVal = fscanf(fid, "%s\n", line);
		FILE *cfpid = fopen(line, "rb");
		if (cfpid == 0) return S_FALSE;

		// load header
		fread(headers + curTexture, 1, sizeof(CFPHeader), cfpid);
		
		// load image data
		int textureSize = headers[curTexture].width * headers[curTexture].height * 3;
		unsigned char *textureData = new unsigned char[textureSize];
		fread(textureData, 1, textureSize, cfpid);
		
		// create openGL texture
		textureWidth[curTexture] = nextPowerOfTwo(headers[curTexture].width);
		textureHeight[curTexture] = nextPowerOfTwo(headers[curTexture].height);
		glBindTexture(GL_TEXTURE_2D, textureGLID[curTexture]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
		glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					 textureWidth[curTexture], textureHeight[curTexture],
					 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
					    headers[curTexture].width, headers[curTexture].height,
						GL_BGR, GL_UNSIGNED_BYTE, textureData);
		
		delete [] textureData;
		fclose(cfpid);
	}

	return S_OK;
}

void CFPContainer::setTexture(int index)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureGLID[index]);
}

void CFPContainer::getBoundingBox(float biometricData[][2], int numBiometricData, float *boundingBox)
{
	boundingBox[0] = 1.0f;
	boundingBox[1] = -1.0f;
	boundingBox[2] = 1.0f;
	boundingBox[3] = -1.0f;

	for (int i = 0; i < numBiometricData; i++)
	{
		if (biometricData[i][0] < boundingBox[0]) boundingBox[0] = biometricData[i][0];
		if (biometricData[i][0] > boundingBox[1]) boundingBox[1] = biometricData[i][0];
		if (biometricData[i][1] < boundingBox[2]) boundingBox[2] = biometricData[i][1];
		if (biometricData[i][1] > boundingBox[3]) boundingBox[3] = biometricData[i][1];
	}

	// buffer zone
	boundingBox[0] -= 0.05f;
	boundingBox[1] += 0.05f;
	boundingBox[2] -= 0.05f;
	boundingBox[3] += 0.05f;

	// adjust x bounding box the same as y
	float center = (boundingBox[1] + boundingBox[0]) * 0.5f;
	float width = boundingBox[1] - boundingBox[0];
	float height = boundingBox[3] - boundingBox[2];
	boundingBox[0] = center - 0.5f * height;
	boundingBox[1] = center + 0.5f * height;
}

// left, right, top, bottom
void CFPContainer::getBoundingBox(int index, float *boundingBox)
{
	getBoundingBox(headers[index].biometricData, headers[index].numBiometricData, boundingBox);
}

void CFPContainer::drawScreenAlignedQuad(int index, float startX, float startY, float endX, float endY)
{
	int imgWidth = headers[index].width;
	int imgHeight = headers[index].height;
	float TEXTURE_U_RANGE = (float)imgWidth / (float)textureWidth[index];
	float TEXTURE_V_RANGE = (float)imgHeight / (float)textureHeight[index];

	float boundingBox[4];
	getBoundingBox(index, boundingBox);
	boundingBox[0] = (boundingBox[0] * 0.5f + 0.5f) * TEXTURE_U_RANGE;
	boundingBox[1] = (boundingBox[1] * 0.5f + 0.5f) * TEXTURE_U_RANGE;
	boundingBox[2] = (boundingBox[2] * 0.5f + 0.5f) * TEXTURE_V_RANGE;
	boundingBox[3] = (boundingBox[3] * 0.5f + 0.5f) * TEXTURE_V_RANGE;

	setTexture(index);
	glBegin(GL_QUADS);
	glTexCoord2f(boundingBox[0], boundingBox[3]);
	glVertex3f(startX, endY, 0.5);
	glTexCoord2f(boundingBox[1], boundingBox[3]);
	glVertex3f(endX, endY, 0.5);
	glTexCoord2f(boundingBox[1], boundingBox[2]);
	glVertex3f(endX, startY, 0.5);
	glTexCoord2f(boundingBox[0], boundingBox[2]);
	glVertex3f(startX, startY, 0.5);
	glEnd();
}

void CFPContainer::drawLongLine(int numEdges, float coordinates[][2],
			                    float startX, float startY,
			                    float endX, float endY)
{
	float width = (endX - startX) * 0.5f;
	float height = (endY - startY) * 0.5f;
	float midX = (startX + endX) * 0.5f;
	float midY = (startY + endY) * 0.5f;
 
	for (int i = 0; i < numEdges-1; i++)
	{
		glVertex3f(midX + width * coordinates[i][0],
				   midY + height * coordinates[i][1], 0.5f);
		glVertex3f(midX + width * coordinates[i+1][0],
				   midY + height * coordinates[i+1][1], 0.5f);
	}
}

void CFPContainer::drawPolygon(int numEdges, float coordinates[][2],
			                   float startX, float startY,
			                   float endX, float endY)
{
	float width = (endX - startX) * 0.5f;
	float height = (endY - startY) * 0.5f;
	float midX = (startX + endX) * 0.5f;
	float midY = (startY + endY) * 0.5f;

	drawLongLine(numEdges, coordinates, startX, startY, endX, endY);
	glVertex3f(midX + width * coordinates[numEdges-1][0],
			   midY + height * coordinates[numEdges-1][1], 0.5f);
	glVertex3f(midX + width * coordinates[0][0],
		       midY + height * coordinates[0][1], 0.5f);
}

void CFPContainer::drawCircle(float xpos1, float ypos1, float xpos2, float ypos2)
{
	float aspectRatio = ASPECT_RATIO;

	float length = sqrtf((xpos2-xpos1)*(xpos2-xpos1) + (ypos2-ypos1)*(ypos2-ypos1)/aspectRatio/aspectRatio);
	float halfLength = 0.707f * length;
	float yLength = length * aspectRatio;
	float yHalfLength = halfLength * aspectRatio;
	glVertex3f(xpos1, ypos1 - yLength, 0.5);
	glVertex3f(xpos1 + halfLength, ypos1 - yHalfLength, 0.5);
	glVertex3f(xpos1 + halfLength, ypos1 - yHalfLength, 0.5);
	glVertex3f(xpos1 + length, ypos1, 0.5);
	glVertex3f(xpos1 + length, ypos1, 0.5);
	glVertex3f(xpos1 + halfLength, ypos1 + yHalfLength, 0.5);
	glVertex3f(xpos1 + halfLength, ypos1 + yHalfLength, 0.5);
	glVertex3f(xpos1, ypos1 + yLength, 0.5);
	glVertex3f(xpos1, ypos1 + yLength, 0.5);
	glVertex3f(xpos1 - halfLength, ypos1 + yHalfLength, 0.5);
	glVertex3f(xpos1 - halfLength, ypos1 + yHalfLength, 0.5);
	glVertex3f(xpos1 - length, ypos1, 0.5);
	glVertex3f(xpos1 - length, ypos1, 0.5);
	glVertex3f(xpos1 - halfLength, ypos1 - yHalfLength, 0.5);
	glVertex3f(xpos1 - halfLength, ypos1 - yHalfLength, 0.5);
	glVertex3f(xpos1, ypos1 - yLength, 0.5);
}

void CFPContainer::drawBiometric(float biometricData[][2],
	      		                 float startX, float startY,
			                     float endX, float endY)
{	
	float width = (endX - startX) * 0.5f;
	float height = (endY - startY) * 0.5f;
	float midX = (startX + endX) * 0.5f;
	float midY = (startY + endY) * 0.5f;
	// float width, height, midX, midY;

	// adjust to bounding box
	float boundingBox[4];
	getBoundingBox(biometricData, 22, boundingBox);
	width /= 0.5f * (boundingBox[1] - boundingBox[0]);
	height /= 0.5f * (boundingBox[3] - boundingBox[2]);
	midX -= 0.5f * (boundingBox[0] + boundingBox[1])*width;
	midY -= 0.5f * (boundingBox[2] + boundingBox[3])*height; //  from texture stuff? TODO!
	//midX += (endX + startX) * 0.5f;
	//midY += (startY + endY) * 0.5f;
	//width *= (endX - startX) * 0.5f;
	//height *= (endY - startY) * 0.5f;
	startX = midX - width;
	endX = midX + width;
	startY = midY - height;
	endY = midY + height;


	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LINE_SMOOTH);
	glShadeModel(GL_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// TODO: line width
	glBegin(GL_LINES);
	glColor3ub(230, 205, 0);
	drawCircle(midX + width * biometricData[0][0], midY + height * biometricData[0][1],
			   midX + width * biometricData[1][0], midY + height * biometricData[1][1]);
	drawCircle(midX + width * biometricData[2][0], midY + height * biometricData[2][1],
			   midX + width * biometricData[3][0], midY + height * biometricData[3][1]);
	drawLongLine(2, biometricData + 4, startX, startY, endX, endY);
	drawLongLine(2, biometricData + 6, startX, startY, endX, endY);
	drawLongLine(3, biometricData + 8, startX, startY, endX, endY);
	drawLongLine(3, biometricData + 11, startX, startY, endX, endY);
	drawPolygon(4, biometricData + 14, startX, startY, endX, endY);
	drawLongLine(4, biometricData + 18, startX, startY, endX, endY);
	glEnd();
	glDisable(GL_LINE_SMOOTH);
	//glDisable(GL_BLEND);
}

void CFPContainer::drawBiometric(int index,
	      		                 float startX, float startY,
			                     float endX, float endY)
{
	float (*biometricData)[2] = headers[index].biometricData;
	drawBiometric(biometricData, startX, startY, endX, endY);
}

void CFPContainer::drawMorphedBiometric(int index, int seed,
	      		                 float startX, float startY,
								 float endX, float endY)
{
	float (*biometricData)[2] = new float[headers[index].numBiometricData][2];
	
	srand(seed);
	rand();
	
	// copy biometric data
	for (int i = 0; i < headers[index].numBiometricData; i++)
	{
		biometricData[i][0] = headers[index].biometricData[i][0];
		biometricData[i][1] = headers[index].biometricData[i][1];
	}

	// change eye y position
	float eyeDeltaY = random() * 0.05f;
	for (int i = 0; i < 14; i++)
	{
		// apply on eyes, eyebrows and top of the nose
		biometricData[i][1] += eyeDeltaY;
	}
	// move nose to a lesser extent
	biometricData[14][1] += 0.75f * eyeDeltaY;
	biometricData[15][1] += 0.75f * eyeDeltaY;
	biometricData[16][1] += 0.25f * eyeDeltaY;
	biometricData[17][1] += 0.25f * eyeDeltaY;

	// change mouth y position
	float mouthDeltaY = random() * 0.08f;
	for (int i = 18; i < 22; i++)
	{
		biometricData[i][1] += mouthDeltaY;
	}

	// change mouth curl
	float mouthCurl = random() * 0.01f;
	biometricData[18][1] += mouthCurl;
	biometricData[21][1] += mouthCurl;

	// change mouth length
	float mouthLength = random() * 0.03f;
	biometricData[18][0] += mouthLength;
	biometricData[19][0] += 0.5f * mouthLength;
	biometricData[20][0] -= 0.5f * mouthLength;
	biometricData[21][0] -= mouthLength;

	// change eye distance
	float eyeDistDelta = random() * 0.01f;
	for (int i = 0; i < 14; i++)
	{
		// left part
		if (i < 2 || (i > 3 && i < 6) || (i > 7 && i < 11))
		{
			biometricData[i][0] += eyeDistDelta;
		}
		else
		{
			biometricData[i][0] -= eyeDistDelta;
		}
	}

	// change brow y
	float browY = random() * 0.03f;
	for (int i = 8; i < 14; i++)
	{
		biometricData[i][1] += browY;
	}

	// brow inner, outer length
	float browOuter = random() * 0.02f;
	float browInner = random() * 0.02f;
	biometricData[8][0] += browOuter;
	biometricData[9][0] += 0.5f * (browOuter + browInner);
	biometricData[10][0] += browInner;
	biometricData[11][0] -= browInner;
	biometricData[12][0] -= 0.5f * (browOuter + browInner);
	biometricData[13][0] -= browOuter;

	// curl brow
	float browCurl = random() * 0.04f;
	biometricData[8][1] += browCurl;
	biometricData[9][1] += 0.5f * browCurl;
	biometricData[12][1] += 0.5f * browCurl;
	biometricData[13][1] += browCurl;

	// nose
	float noseTopWidth = random() * 0.01f;
	float noseBaseWidth = random() * 0.01f;
	biometricData[14][0] += noseTopWidth;
	biometricData[15][0] -= noseTopWidth;
	biometricData[16][0] += noseBaseWidth;
	biometricData[17][0] -= noseBaseWidth;
	float noseTopPos = random() * 0.03f;
	float noseBottomPos = random() * 0.03f;
	biometricData[14][1] += noseTopPos;
	biometricData[15][1] += noseTopPos;
	biometricData[16][1] += noseBottomPos;
	biometricData[17][1] += noseBottomPos;
	float nosePos = random() * 0.05f;
	for (int i = 14; i < 18; i++)
	{
		biometricData[i][1] += nosePos;
	}

	// a very small additional noise
	for (int i = 0; i < headers[index].numBiometricData; i++)
	{
		biometricData[i][0] += random() * 0.005f;
		biometricData[i][1] += random() * 0.005f;
	}

	drawBiometric(biometricData, startX, startY, endX, endY);

	delete [] biometricData;
}

float CFPContainer::random()
{
	int randVal = abs((rand() >> 8) & 127);
	return (float)randVal / 64.0f - 1.0f;
}

int CFPContainer::nextPowerOfTwo(int value)
{
	int result = 1;
	while (result < value) result *= 2;
	return result;
}