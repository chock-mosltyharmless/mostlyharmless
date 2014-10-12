//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"

#include "config.h"
#include "intro.h"
#include "mzk.h"
#include "Parameter.h"
#include "Texture.h"
#include "mathhelpers.h"

float frand();
int rand();


// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

#define NUM_GL_NAMES 16
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
	 "glDeleteProgram", "glDeleteShader",
	 "glActiveTexture", "glGetUniformLocation", "glUniform1i",
	 "glMultiTexCoord2f"
};

// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];

// Two offsceen textures, one for normal rendering, one for highlights
const int NUM_OFFSCREEN_TEXTURES = 2;
static GLuint offscreenTexture[NUM_OFFSCREEN_TEXTURES];
static Texture backgroundTexture[3];
static Texture hLogoTexture;

GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
const int NUM_SHADER_PROGRAMS = 4;
const int SHADER_BACKGROUND = 0;
const int SHADER_COPY = 1;
const int SHADER_BACKGROUND_LIGHT = 2;
const int SHADER_BLUR = 3;
static GLuint shaderPrograms[NUM_SHADER_PROGRAMS] = {0, 0, 0, 0};

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

#define MAX_SHADER_SIZE 1000000
void loadShaders(void)
{
	static GLchar backgroundText[MAX_SHADER_SIZE];
	const GLchar *ptBackground = backgroundText;
	static GLchar objectText[MAX_SHADER_SIZE];
	const GLchar *ptObject = objectText;
	static GLchar objectNCText[MAX_SHADER_SIZE];
	const GLchar *ptObjectNC = objectNCText;
	static GLchar copyText[MAX_SHADER_SIZE];
	const GLchar *ptCopy = copyText;
	static GLchar blurText[MAX_SHADER_SIZE];
	const GLchar *ptBlur = blurText;
	static GLchar lightText[MAX_SHADER_SIZE];
	const GLchar *ptLight = lightText;
	
	// delete objects
	if (shaderPrograms[0])
	{
		for (int i = 0; i < NUM_SHADER_PROGRAMS; i++)
		{
			glDeleteProgram(shaderPrograms[i]);
			shaderPrograms[i] = 0;
		}
	}

	// load from file
	FILE *fid = fopen("background.glsl", "rb");
	ZeroMemory(backgroundText, sizeof(backgroundText));
	fread(backgroundText, 1, MAX_SHADER_SIZE, fid);
	fclose(fid);
	fid = fopen("object.glsl", "rb");
	ZeroMemory(objectText, sizeof(objectText));
	fread(objectText, 1, MAX_SHADER_SIZE, fid);
	fclose(fid);
	fid = fopen("object_noCenters.glsl", "rb");
	ZeroMemory(objectNCText, sizeof(objectNCText));
	fread(objectNCText, 1, MAX_SHADER_SIZE, fid);
	fclose(fid);
	fid = fopen("copy.glsl", "rb");
	ZeroMemory(copyText, sizeof(copyText));
	fread(copyText, 1, MAX_SHADER_SIZE, fid);
	fclose(fid);
	fid = fopen("blur.glsl", "rb");
	ZeroMemory(blurText, sizeof(blurText));
	fread(blurText, 1, MAX_SHADER_SIZE, fid);
	fclose(fid);
	fid = fopen("background_light.glsl", "rb");
	ZeroMemory(lightText, sizeof(lightText));
	fread(lightText, 1, MAX_SHADER_SIZE, fid);
	fclose(fid);

	// init objects:
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint vMainObjectNC = glCreateShader(GL_VERTEX_SHADER);
	GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);	
	GLuint fOffscreenCopy = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint fOffscreenBlur = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint fMainLight = glCreateShader(GL_FRAGMENT_SHADER);
	for (int i = 0; i < NUM_SHADER_PROGRAMS; i++)
	{
		shaderPrograms[i] = glCreateProgram();
	}
	// compile sources:
	glShaderSource(vMainObject, 1, &ptObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(vMainObjectNC, 1, &ptObjectNC, NULL);
	glCompileShader(vMainObjectNC);
	glShaderSource(fMainBackground, 1, &ptBackground, NULL);
	glCompileShader(fMainBackground);
	glShaderSource(fOffscreenCopy, 1, &ptCopy, NULL);
	glCompileShader(fOffscreenCopy);
	glShaderSource(fOffscreenBlur, 1, &ptBlur, NULL);
	glCompileShader(fOffscreenBlur);
	glShaderSource(fMainLight, 1, &ptLight, NULL);
	glCompileShader(fMainLight);
	
	// Check programs
	int tmp, tmp2;
	char err[4097];
	glGetShaderiv(vMainObject, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(vMainObject, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "vMainObject shader error", MB_OK);
		return;
	}
	glGetShaderiv(vMainObjectNC, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(vMainObjectNC, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "vMainObject no centers shader error", MB_OK);
		return;
	}
	glGetShaderiv(fMainBackground, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(fMainBackground, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "fMainBackground shader error", MB_OK);
		return;
	}
	glGetShaderiv(fOffscreenCopy, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(fOffscreenCopy, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "fOffscreeCopy shader error", MB_OK);
		return;
	}
	glGetShaderiv(fOffscreenBlur, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(fOffscreenBlur, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "fOffscreeBlur shader error", MB_OK);
		return;
	}
	glGetShaderiv(fMainLight, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(fMainLight, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "fMainLight shader error", MB_OK);
		return;
	}

	// link shaders:
	glAttachShader(shaderPrograms[SHADER_BACKGROUND], vMainObject);
	glAttachShader(shaderPrograms[SHADER_BACKGROUND], fMainBackground);
	glLinkProgram(shaderPrograms[SHADER_BACKGROUND]);
	glAttachShader(shaderPrograms[SHADER_COPY], vMainObjectNC);
	glAttachShader(shaderPrograms[SHADER_COPY], fOffscreenCopy);
	glLinkProgram(shaderPrograms[SHADER_COPY]);
	glAttachShader(shaderPrograms[SHADER_BLUR], vMainObjectNC);
	glAttachShader(shaderPrograms[SHADER_BLUR], fOffscreenBlur);
	glLinkProgram(shaderPrograms[SHADER_BLUR]);
	glAttachShader(shaderPrograms[SHADER_BACKGROUND_LIGHT], vMainObject);
	glAttachShader(shaderPrograms[SHADER_BACKGROUND_LIGHT], fMainLight);
	glLinkProgram(shaderPrograms[SHADER_BACKGROUND_LIGHT]);

	// delete unneeded shaders
	glDeleteShader(fMainBackground);
	glDeleteShader(vMainObject);
	glDeleteShader(fOffscreenCopy);
	glDeleteShader(fOffscreenBlur);
	glDeleteShader(fMainLight);

	// Set texture locations
	glUseProgram(shaderPrograms[SHADER_BACKGROUND]);
	int my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[0], "Texture0");
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(my_sampler_uniform_location, 0);
	my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[0], "Texture1");
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(my_sampler_uniform_location, 1);
	glActiveTexture(GL_TEXTURE0);
}

void intro_init( void )
{
	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop...
	loadShaders();

	// Set texture.
#if 0
	backgroundTexture[0].init("background.tga");
	backgroundTexture[0].setTexture();
	backgroundTexture[1].init("background2.tga");
	backgroundTexture[1].setTexture();
	backgroundTexture[2].init("background3.tga");
	backgroundTexture[2].setTexture();
	hLogoTexture.init("../her_16x9_1920.tga");
	hLogoTexture.setTexture();
#endif

	// Create a rendertarget texture (unused?)
	glGenTextures(NUM_OFFSCREEN_TEXTURES, offscreenTexture);
	for (int i = 0; i < NUM_OFFSCREEN_TEXTURES; i++)
	{
		glBindTexture(GL_TEXTURE_2D, offscreenTexture[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (i == 0)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT, 0,
						 GL_RGBA, GL_UNSIGNED_BYTE, 0);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT, 0,
						 GL_RGBA, GL_UNSIGNED_BYTE, 0);
		}
	}

	// RLY?
	//glEnable(GL_CULL_FACE);
}

void intro_do( long itime )
{
	// Just copy the video to the front
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	videoTexture.captureFrame();
	parameterMatrix[1] = -2.0f;
	parameterMatrix[2] = 0.0f;
	parameterMatrix[3] = 0.0f;
	parameterMatrix[4] = 0.0f; // scanline amount
	glLoadMatrixf(parameterMatrix);
	//glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	// Copy backbuffer to texture
	glUseProgram(shaderPrograms[SHADER_COPY]);
	glRectf(-1.0, -1.0, 1.0, 1.0);
	glDisable(GL_BLEND);
}

// This function is called by Parameter.cpp when something changed.
// I need this to get proper BPMs.
// I need a timer here... Hmmm...
void registerParameterChange(int keyID) {
}