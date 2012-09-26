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
static Texture backgroundTexture;

GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
const int NUM_SHADER_PROGRAMS = 3;
const int SHADER_BACKGROUND = 0;
const int SHADER_COPY = 1;
const int SHADER_BACKGROUND_LIGHT = 2;
static GLuint shaderPrograms[NUM_SHADER_PROGRAMS] = {0, 0, 0};

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
	static GLchar copyText[MAX_SHADER_SIZE];
	const GLchar *ptCopy = copyText;
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
	fid = fopen("copy.glsl", "rb");
	ZeroMemory(copyText, sizeof(copyText));
	fread(copyText, 1, MAX_SHADER_SIZE, fid);
	fclose(fid);
	fid = fopen("background_light.glsl", "rb");
	ZeroMemory(lightText, sizeof(lightText));
	fread(lightText, 1, MAX_SHADER_SIZE, fid);
	fclose(fid);

	// init objects:
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);	
	GLuint fOffscreenCopy = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint fMainLight = glCreateShader(GL_FRAGMENT_SHADER);
	shaderPrograms[0] = glCreateProgram();
	shaderPrograms[1] = glCreateProgram();
	shaderPrograms[2] = glCreateProgram();
	// compile sources:
	glShaderSource(vMainObject, 1, &ptObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(fMainBackground, 1, &ptBackground, NULL);
	glCompileShader(fMainBackground);
	glShaderSource(fOffscreenCopy, 1, &ptCopy, NULL);
	glCompileShader(fOffscreenCopy);
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
	glAttachShader(shaderPrograms[SHADER_COPY], vMainObject);
	glAttachShader(shaderPrograms[SHADER_COPY], fOffscreenCopy);
	glLinkProgram(shaderPrograms[SHADER_COPY]);
	glAttachShader(shaderPrograms[SHADER_BACKGROUND_LIGHT], vMainObject);
	glAttachShader(shaderPrograms[SHADER_BACKGROUND_LIGHT], fMainLight);
	glLinkProgram(shaderPrograms[SHADER_BACKGROUND_LIGHT]);

	// delete unneeded shaders
	glDeleteShader(fMainBackground);
	glDeleteShader(vMainObject);
	glDeleteShader(fOffscreenCopy);
	glDeleteShader(fMainLight);

#if 0
	// Set texture locations
	glUseProgram(shaderPrograms[0]);
	int my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[0], "Texture0");
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(my_sampler_uniform_location, 0);
	my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[0], "Texture1");
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(my_sampler_uniform_location, 1);
	glActiveTexture(GL_TEXTURE0);
#endif
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
	backgroundTexture.init("background.tga");
	backgroundTexture.setTexture();

	// Create a rendertarget texture
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

void fallingBall(float ftime)
{
	glDisable(GL_BLEND);

	// draw background:
#if 0
    glMatrixMode(GL_PROJECTION);	
	glLoadMatrixf(projectionMatrix);
	glTranslatef(params.getParam(20, 0.41f) * 6.0f - 3.0f, params.getParam(21, 0.56f) * 6.0f - 3.0f, -params.getParam(22, 0.36f) * 20.0f);
	glRotatef(params.getParam(9, 0.15f) * 360.0f, 1.0f, 0.0f, 0.0f);
	glRotatef(params.getParam(12, 0.31f) * 360.0f, 0.0f, 1.0f, 0.0f); 
	glRotatef(params.getParam(13, 0.35f) * 360.0f, 0.0f, 0.0f, 1.0f);
#endif
	glMatrixMode(GL_MODELVIEW);	

	// I want to interpolate the new values from the old ones.
	const int maxNumParameters = 25;
	const static float defaultParameters[maxNumParameters] = 
	{
		-1.0f, -1.0f,
		0.4f, 0.3f,			// 2-3: size,spread X				1-2
		0.2f, 0.3f,			// 4-5: size,spread Y				3-4
		0.8f,				// 6: speed							5
		-1.0f,
		0.6f,				// 8: spiking spread				6
		0.3f,               // 9: spiking brightness			7
		-1.0f, -1.0f,
		0.4f,				// 12: spiking highlightAmount		8
		0.0f,				// 13: unused						9
		0.4f,				// 14: color variation				1b
		0.4f, 0.15f, 0.3f,	// 15-17: mainColor					2-4b
		0.5f,				// 18: highlightAmount				5b
		0.34f,				// 19: hypnoglow					6b
		1.0f,				// 20: Bauchigkeit					7b
		0.0f,				// 21: line strength				8b
		0.0f,				// 22: beat phase					9b
	};
	static float interpolatedParameters[maxNumParameters];
	for (int i = 0; i < maxNumParameters; i++)
	{
		interpolatedParameters[i] = 0.95f * interpolatedParameters[i] +
									0.05f * params.getParam(i, defaultParameters[i]);
	}

	// Beat-dependent variables:
	const int NUM_BPMS = 6;
	const float BPMArray[NUM_BPMS] = {124.0f, 96.0f, 106.0f, 128.0f, 136.0f, 136.0f};
	const int BPMParameterIndex[NUM_BPMS] = {30, 32, 34, 36, 38, 40};
	int BPMIndex = 0; // I should have error here... but for the sake of stability I will assume first music.
	// Get the status fields (I need the numbers....)
	for (int i = 0; i < NUM_BPMS; i++)
	{
		if (params.getParam(BPMParameterIndex[i], 0.0f) > 0.5f) BPMIndex = i;
	}
	float BPM = BPMArray[BPMIndex];
	float BPS = BPM / 60.0f;
	// Here I should make something more sophisticated. I am thinking about an array... that has
	// Lengthes until the next jump in order to make a beat like ||..||..||..|.|.
	float jumpsPerSecond = BPS / 1.0f; // Jump on every fourth beat.
	float jumpTime = (ftime * jumpsPerSecond) + interpolatedParameters[22]; // JumpTime goes from 0 to 1 between two beats
	jumpTime -= floor(jumpTime);
	jumpTime = jumpTime * jumpTime;
	// spike is between 0.0 and 1.0 depending on the position within whatever.
	float spike = 0.5f * cosf(jumpTime * 3.1415926f * 2.0f) + 0.5f;
#if 0
	// I do not want to do this. This is the heart beating idea for movement, but it didn't
	// turn out that well. I might onlz put this back to life if I see fit for some music.
	parameterMatrix[6] = 2.0f * spike * interpolatedParameters[8] * interpolatedParameters[2]; // spiked spread.x * size.X
	parameterMatrix[7] = 2.0f * spike * interpolatedParameters[8] * interpolatedParameters[4]; // spiked spread.y * size.Y
	parameterMatrix[10] = 0.0f;
	parameterMatrix[11] = 0.0f;
#else
	parameterMatrix[6] = 0.0f;
	parameterMatrix[7] = 0.0f;
	parameterMatrix[10] = spike * interpolatedParameters[9]; // spiked mainColor.brightness
	parameterMatrix[11] = 5.0f * spike * interpolatedParameters[12]; // spiked highlightAmount
#endif

	//parameterMatrix[0] = ftime; // time	
	// Steuerbare time
	static float shaderTime = 0.0f;
	static float oldTime = 0.0f;
	float deltaTime = ftime - oldTime;
	oldTime = ftime;
	// here I need a time update based on beat time.
	// The problem is that I have the same timing on the
	// Hermann. I might not want that?
	// THe 1.6 is 1 divided by the integral of spike.
	deltaTime *= 1.6f * interpolatedParameters[8] * spike + (1.0f - interpolatedParameters[8]);
	shaderTime += deltaTime * interpolatedParameters[6] * 3.0f;
	parameterMatrix[0] = shaderTime;
	/* shader parameters */
	parameterMatrix[1] = 10.0f * interpolatedParameters[20] * interpolatedParameters[20];  // bauchigkeit
	parameterMatrix[2] = interpolatedParameters[21];			// line strength
	parameterMatrix[3] = interpolatedParameters[14];			// color variation
	parameterMatrix[4] = interpolatedParameters[2];			// size.x			
	parameterMatrix[5] = interpolatedParameters[4];			// size.y
	parameterMatrix[6] += interpolatedParameters[3];			// spread.x	
	parameterMatrix[7] += interpolatedParameters[5];			// spread.y
	parameterMatrix[8] = interpolatedParameters[15];			// mainColor.h
	parameterMatrix[9] = interpolatedParameters[16];		// mainColor.s
	parameterMatrix[10] += interpolatedParameters[17];		// mainColor.b
	parameterMatrix[11] += 5.0f * interpolatedParameters[18]; // highlightAmount

	glLoadMatrixf(parameterMatrix);

	// DRAW the right hand view (lighting on hermann): I need to remove the bumping colors here
	glViewport(XRES/2, 0, XRES / 2, YRES);
	glUseProgram(shaderPrograms[SHADER_BACKGROUND_LIGHT]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// DRAW the left hand view (background on projection)
	// draw offscreen (full offscreen resolution)
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_BACKGROUND]);
	backgroundTexture.setTexture();
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// downsample to highlight resolution
	parameterMatrix[1] = 0.0f;
	parameterMatrix[2] = 1.0f / OFFSCREEN_WIDTH;
	parameterMatrix[3] = 1.0f / OFFSCREEN_HEIGHT;
	parameterMatrix[4] = 0.0f; // scanline amount
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[0]);
	// Copy backbuffer to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_COPY]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// horizontal blur
	parameterMatrix[1] = 0.2f;
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = 1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 2.0f; // reduction
	parameterMatrix[5] = 0.01f * interpolatedParameters[19]; // gain
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer (small) to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_COPY]);
	glRectf(-1.0, -1.0, 1.0, 1.0); // TODO: I think I can optimize this???

	// vertical blur
	parameterMatrix[1] = 0.2f;
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = -1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 1.0f; // reduction
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_COPY]);	
	glRectf(-1.0, -1.0, 1.0, 1.0); // TODO: I think I can optimze this???

	// copy highlight to front
	parameterMatrix[1] = 0.0f;
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = 1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 0.0f; // scanline amount
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, XRES / 2, YRES);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_COPY]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// add normal image
	parameterMatrix[1] = 0.0f;
	parameterMatrix[2] = 1.0f / OFFSCREEN_WIDTH;
	parameterMatrix[3] = 1.0f / OFFSCREEN_HEIGHT;
	parameterMatrix[4] = 0.25f; // scanline amount
	glLoadMatrixf(parameterMatrix);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glViewport(0, 0, XRES / 2, YRES);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[0]); // was already copied!
	glUseProgram(shaderPrograms[SHADER_COPY]);
	glRectf(-1.0, -1.0, 1.0, 1.0);
}

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

    // render
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_BLEND);
    glEnable( GL_CULL_FACE );
	//glCullFace( GL_FRONT );
	//glDisable( GL_BLEND );
    //glEnable( GL_LIGHTING );
    //glEnable( GL_LIGHT0 );
    //glEnable( GL_NORMALIZE );
	//glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);	
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

	// clear screan:
	//glClear(GL_COLOR_BUFFER_BIT);

	//glBindTexture(GL_TEXTURE_3D, noiseTexture); // 3D noise?	

	/* Set everything to beginning */
	for (int i = 0; i < 16; i++)
	{
		parameterMatrix[i] = 0.0f;
	}

	fallingBall(ftime);
}

