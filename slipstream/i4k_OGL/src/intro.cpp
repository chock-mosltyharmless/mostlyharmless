//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"

#include "config.h"
#include "intro.h"
#include "mzk.h"
#include "Parameter.h"
#include "GLTools.h"
#include "ParticleSystem.h"

float frand();
int rand();

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

// Name of the 32x32x32 noise texture
#define FLOAT_TEXTURE
#define NOISE_TEXTURE_SIZE 16 // try smaller?
static GLuint noiseTexture;
#ifdef FLOAT_TEXTURE
static float noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];
#else
static unsigned char noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];
#endif
static int noiseTmp[4];

// data for the vertex buffer stuff
GLuint vertexBufferID;

static GLuint shaderPrograms[1];

// The particle system is a singleton:
static ParticleSystem particleSystem;

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

void intro_init( void )
{
	initGLTools();

	// create noise Texture
#ifdef FLOAT_TEXTURE
	for (int i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData[i] = frand() - 0.5f;
	}
#else
	for (int i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData[i] = (unsigned char)rand();
	}
#endif

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop...
	
	if (!createShaderProgram("vertex.glsl", NULL, "fragment.glsl",
						     &(shaderPrograms[0])))
	{
		return;
	}

	// Set texture.
	glEnable(GL_TEXTURE_3D); // automatic?
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	//glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, 0, GL_RGBA, 
	//	         GL_UNSIGNED_BYTE, noiseData);
#ifdef FLOAT_TEXTURE
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F,
				 NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
				 0, GL_RGBA, GL_FLOAT, noiseData);
#else
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8,
				 NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
				 0, GL_RGBA, GL_UNSIGNED_BYTE, noiseData);
#endif

	// Create vertex buffer object
	glGenBuffers(1, &vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);

	// RLY?
	//glEnable(GL_CULL_FACE);

	// Initialize the singleton particle system.
	particleSystem.init();
}

void renderEffect(float ftime)
{
	// The model matrix is used to send the parameters to the hardware...
	static float parameterMatrix[16];
	// The camera transformation matrix
	static float cameraMatrix[3][4];

	glDisable(GL_BLEND);

	// draw background:
	glClearColor(0.0f, 0.0f, 0.3f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);	

	// Clear shader parameters:
	for (int i = 0; i < 16; i++)
	{
		parameterMatrix[i] = 0.0f;
	}

	/* Set non-zero shader parameters */
	parameterMatrix[0] = ftime; // time		
	// parameterMatrix[1] = params.getParam(2, 0.75f);
	glLoadMatrixf(parameterMatrix);

	// draw some test triangle.
	//glViewport(0, 0, XRES, YRES);
	//glUseProgram(shaderPrograms[0]);
	//glBindTexture(GL_TEXTURE_3D, noiseTexture);
	//glColor3f(0.3f, 0.5f, 0.7f);
	//glRectf(-0.0f, -0.0f, 1.0f, 1.0f);

	// Set the camera matrix:
	for (int k = 0; k < 3; k++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (j == k)
			{
				cameraMatrix[k][j] = 1.0f;
			}
			else
			{
				cameraMatrix[k][j] = 0.0f;
			}
		}
	}

	glUseProgram(shaderPrograms[0]);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	particleSystem.render(cameraMatrix);
}

void intro_do( long itime )
{
	// TODO: I need some sort of lowpass filter of the time
	// Some sort of median or the like, so that the display does
	// not hop that much around on missed frames.
	static float oldTime = 1.0e10f;
	float ftime = 0.001f*(float)itime;
	float deltaTime = ftime - oldTime;
	if (deltaTime < 0.0f) deltaTime = 0.0f;
	oldTime = ftime;

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

	// update the particle system:
	particleSystem.update(deltaTime);

	// This is the effect function
	renderEffect(ftime);
}

void intro_end()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vertexBufferID);
}
