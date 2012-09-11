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
static GLuint shaderPrograms[2] = {0, 0};

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
	
	// delete objects
	if (shaderPrograms[0])
	{
		for (int i = 0; i < 2; i++)
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

	// init objects:
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);	
	GLuint fOffscreenCopy = glCreateShader(GL_FRAGMENT_SHADER);
	shaderPrograms[0] = glCreateProgram();
	shaderPrograms[1] = glCreateProgram();
	// compile sources:
	glShaderSource(vMainObject, 1, &ptObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(fMainBackground, 1, &ptBackground, NULL);
	glCompileShader(fMainBackground);
	glShaderSource(fOffscreenCopy, 1, &ptCopy, NULL);
	glCompileShader(fOffscreenCopy);
	
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

	// link shaders:
	glAttachShader(shaderPrograms[0], vMainObject);
	glAttachShader(shaderPrograms[0], fMainBackground);
	glLinkProgram(shaderPrograms[0]);
	glAttachShader(shaderPrograms[1], vMainObject);
	glAttachShader(shaderPrograms[1], fOffscreenCopy);
	glLinkProgram(shaderPrograms[1]);

	// delete unneeded shaders
	glDeleteShader(fMainBackground);
	glDeleteShader(vMainObject);
	glDeleteShader(fOffscreenCopy);

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
	const int maxNumParameters = 20;
	const static float defaultParameters[maxNumParameters] = 
	{
		0.0f, 0.0f,
		1.0f,				// 2: Bauchigkeit
		0.0f,				// 3: line strength
		0.4f,				// 4: color variation
		0.4f, 0.2f,			// 5-6: size
		0.0f,
		0.3f, 0.3f,			// 8-9: spread	
		0.0f, 0.0f,
		0.4f, 0.15f, 0.3f,	// 12-14: mainColor.h
		0.5f,				// 15: highlightAmount
		1.0f,				// 16: spiking spread
		0.5f,               // 17: spiking brightness
		0.8f,				// 18: spiking highlightAmount
	};
	static float interpolatedParameters[maxNumParameters];
	for (int i = 0; i < maxNumParameters; i++)
	{
		interpolatedParameters[i] = 0.95f * interpolatedParameters[i] +
									0.05f * params.getParam(i, defaultParameters[i]);
	}

	// Beat-dependent variables:
	float jumpTime = (ftime * 1.0f);
	jumpTime -= floor(jumpTime);
	jumpTime = jumpTime * jumpTime;
	float spike = 0.5f * cosf(jumpTime * 3.1415926f * 2.0f) + 0.5f;
	parameterMatrix[6] = 2.0f * spike * interpolatedParameters[16] * interpolatedParameters[5]; // spiked spread.x
	parameterMatrix[7] = 2.0f * spike * interpolatedParameters[16] * interpolatedParameters[6]; // spiked spread.y
	parameterMatrix[10] = spike * interpolatedParameters[17]; // spiked mainColor.brightness
	parameterMatrix[11] = 5.0f * spike * interpolatedParameters[18]; // spiked highlightAmount

	parameterMatrix[0] = ftime; // time	
	/* shader parameters */
	parameterMatrix[1] = 10.0f * interpolatedParameters[2] * interpolatedParameters[2];  // bauchigkeit
	parameterMatrix[2] = interpolatedParameters[3];			// line strength
	parameterMatrix[3] = interpolatedParameters[4];			// color variation
	parameterMatrix[4] = interpolatedParameters[5];			// size.x			
	parameterMatrix[5] = interpolatedParameters[6];			// size.y
	parameterMatrix[6] += interpolatedParameters[8];			// spread.x	
	parameterMatrix[7] += interpolatedParameters[9];			// spread.y
	parameterMatrix[8] = interpolatedParameters[12];			// mainColor.h
	parameterMatrix[9] = interpolatedParameters[13];		// mainColor.s
	parameterMatrix[10] += interpolatedParameters[14];		// mainColor.b
	parameterMatrix[11] += 5.0f * interpolatedParameters[15]; // highlightAmount

	glLoadMatrixf(parameterMatrix);

	// draw offscreen (full offscreen resolution)
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glUseProgram(shaderPrograms[0]);
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
	glUseProgram(shaderPrograms[1]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// horizontal blur
	parameterMatrix[1] = 0.2f;
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = 1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 0.4f; // subtractor
	parameterMatrix[5] = 0.007f; // gain
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer (small) to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[1]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// vertical blur
	parameterMatrix[1] = 0.2f;
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = -1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 0.0f; // subtractor
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[1]);	
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// copy highlight to front
	parameterMatrix[1] = 0.0f;
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = 1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 0.0f; // scanline amount
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, XRES, YRES);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[1]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// add normal image
	parameterMatrix[1] = 0.0f;
	parameterMatrix[2] = 1.0f / OFFSCREEN_WIDTH;
	parameterMatrix[3] = 1.0f / OFFSCREEN_HEIGHT;
	parameterMatrix[4] = 0.25f; // scanline amount
	glLoadMatrixf(parameterMatrix);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glViewport(0, 0, XRES, YRES);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[0]); // was already copied!
	glUseProgram(shaderPrograms[1]);
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

