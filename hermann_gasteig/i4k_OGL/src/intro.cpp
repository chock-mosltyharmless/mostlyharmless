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
#include "Swarm.h"

float frand();
int rand();
extern int whatToDo;

// ---------------------------------------------------------------
//					Parameter interpolation stuff
// ------------------------------------------------------------
// I want to interpolate the new values from the old ones.
const int maxNumParameters = 25;
const static float defaultParameters[maxNumParameters] = 
{
	-1.0f, -1.0f,
	0.4f, 0.3f,			// 2-3: size,spread X				1-2
	0.2f, 0.3f,			// 4-5: size,spread Y				3-4
	0.8f,				// 6: speed							5
	-1.0f,
	0.5f,				// 8: spiking spread				6
	0.8f,               // 9: spiking brightness			7
	-1.0f, -1.0f,
	0.0f,				// 12: color reduction		        8
	0.5f,				// 13: brightness hermann			9
	0.4f,				// 14: color variation				1b
	0.4f, 0.15f, 0.3f,	// 15-17: mainColor					2-4b
	0.5f,				// 18: highlightAmount				5b
	0.14f,				// 19: hypnoglow					6b
	1.0f,				// 20: Bauchigkeit					7b
	0.0f,				// 21: line strength				8b
	0.0f,				// 22: texture vignette				9b
};
static float interpolatedParameters[maxNumParameters];
const int NUM_KEYS = 127;
static int keyPressed[NUM_KEYS] = {0};


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
	backgroundTexture[0].init("background.tga");
	backgroundTexture[0].setTexture();
	backgroundTexture[1].init("background2.tga");
	backgroundTexture[1].setTexture();
	backgroundTexture[2].init("background3.tga");
	backgroundTexture[2].setTexture();
	hLogoTexture.init("../her_16x9_1920.tga");
	hLogoTexture.setTexture();

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

	// Initialize Swarm positions and stuff
	initSwarm();
}

void rhytmPolice(float ftime)
{
	ftime += 0.0f;
	static float oldtime = 0.0f;
	float fDeltaTime = ftime - oldtime;
	if (fDeltaTime > 0.1f) fDeltaTime = 0.1f;
	oldtime = ftime;

	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

	/* Rhythmic stuff */
	static float camDist = 0.0f;
	camDist *= exp(-2.0f * fDeltaTime);
	if (keyPressed[41] == 1) camDist = 0.3f;
	static float brightAdder = 0.0f;
	brightAdder *= exp(-2.0f * fDeltaTime);
	if (keyPressed[40] == 1) brightAdder = 1.0f;
	static float moveAdder = 0.0f;
	moveAdder *= exp(-5.0f * fDeltaTime);
	if (keyPressed[39] == 1) moveAdder = 10.0f;

	/* init */
	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Distance to the center object
	// TODO: Hitchcock-effect by means of gluPerspective!
	float cameraDist = 20.0f;
	float cameraComeTime = 150.0f;
	cameraDist = 2.5f + 17.0f * interpolatedParameters[2] * interpolatedParameters[2];

	// set up matrices
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// TODO aspect
	gluPerspective(300.0f / cameraDist,  1.8, 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(sin(0.5f*ftime)*cameraDist * (1.0f - camDist), 0.5, -cos(0.5f * ftime) * cameraDist * (1.0f - camDist),
			  2.0f * interpolatedParameters[3], 2.0f * interpolatedParameters[4], 0.0,
			  0.0, 1.0, 0.0);

	// lighting:
	float b = interpolatedParameters[18];
	float ambient[4] = {0.1f*b, 0.1f*b, 0.1f*b, 1.0f};
	float diffuse[4] = {0.9f*b, 0.85f*b, 0.4f*b, 1.0f};
	float diffuse2[4] = {0.45f*b, 0.475f*b, 0.2f*b, 1.0f};
	float specular[4] = {0.5f*b, 0.4f*b, 0.2f*b, 1.0f};
	float lightDir[4] = {0.7f, 0.0f, 0.7f, 0.0f};
	float allOnes[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	static int signedDistanceID = 1;
	static int pathID = 0;
	float metaAmount = interpolatedParameters[5];

	// Get the status fields (I need the numbers....)
	const int BPMParameterIndex[4] = {23, 24, 25, 26,};
	const int pathParameterIndex[2] = {33, 34};
	for (int i = 0; i < 4; i++)
	{
		// If a key press changed to pressed:
		if (keyPressed[BPMParameterIndex[i]] == 1)
		{
			signedDistanceID = i;
		}
	}
	for (int i = 0; i < 2; i++)
	{
		if (keyPressed[pathParameterIndex[i]])
		{
			pathID = i;
		}
	}

	// generate destiations
	float overshoot = 4.0f * (interpolatedParameters[6] - 0.5f);
	updateSwarmDestinations(pathID, fDeltaTime, overshoot);

	// update direction of the Triangles
	updateSwarmWithSignedDistance(signedDistanceID, fDeltaTime * (1.0f + moveAdder), metaAmount);

	// move all triangles
	moveSwarm(fDeltaTime);

	// Set how many triangles to render for each path
	int numTrisRender1 = NUM_TRIANGLES;
	int numTrisRender2 = NUM_TRIANGLES;
	//float triBrightness = ftime / 30.0f + 0.2f;
	float triBrightness = interpolatedParameters[17];
	if (triBrightness > 1.0f) triBrightness = 1.0f;

	// render tris
	glBegin(GL_TRIANGLES);
	glDisable(GL_BLEND);
	for (int i = 0; i < numTrisRender1; i++)
	{
		//glNormal3f(0.3f, 0.5f, 0.2f);
		float right[3];
		right[0] = tris.direction[i][1] * tris.normal[i][2] - tris.direction[i][2] * tris.normal[i][1];
		right[1] = tris.direction[i][2] * tris.normal[i][0] - tris.direction[i][0] * tris.normal[i][2];
		right[2] = tris.direction[i][0] * tris.normal[i][1] - tris.direction[i][1] * tris.normal[i][0];
		glNormal3fv(tris.normal[i]);
		glVertex3f(tris.position[i][0] + 0.20f * tris.direction[i][0], tris.position[i][1] + 0.2f * tris.direction[i][1], tris.position[i][2] + 0.15f * tris.direction[i][2]);
		glVertex3f(tris.position[i][0] + 0.20f * right[0], tris.position[i][1] + 0.2f * right[1], tris.position[i][2] + 0.15f * right[2]);
		glVertex3f(tris.position[i][0] - 0.20f * right[0], tris.position[i][1] - 0.2f * right[0], tris.position[i][2] - 0.15f * right[0]);
	}
	glEnd();

	glDepthMask(FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE); 
	glDisable(GL_LIGHTING);
	//float beating = 1.0f - 0.25f * fabsf((float)sin(ftime*4.652f));
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < numTrisRender2; i++)
	{
		float color[3];
		float colorT = interpolatedParameters[15];
		float randNum = ((i * i)%17) * 3.2132342f + (i%21) * 46.2246231f + ((i * i * i)%31) * 7.23421f;
		if (randNum < 0.0f) randNum = -randNum;
		randNum = randNum - (int)randNum - 0.5f;
		colorT += randNum * interpolatedParameters[14];
		if (colorT < 0.0f) colorT += 1.0f;
		if (colorT > 1.0f) colorT -= 1.0f;
		float sat = interpolatedParameters[16];
		if (colorT < 0.33f)
		{
			color[0] = 1.0f - 3.0f * colorT * sat;
			color[1] = colorT * 3.0f * sat + 1.0f - sat;
			color[2] = 1.0f - sat;
		}
		else
		{
			if (colorT < 0.67f)
			{
				colorT -= 0.33f;
				color[0] = 1.0f - sat;
				color[1] = 1.0f - 3.0f * colorT * sat;
				color[2] = 3.0f * colorT * sat + 1.0f - sat;
			}
			else
			{
				colorT -= 0.67f;
				color[0] = 3.0f * colorT * sat + 1.0f - sat;
				color[1] = 1.0f - sat;
				color[2] = 1.0f - 3.0f * colorT * sat;
			}
		}

		//glNormal3f(0.3f, 0.5f, 0.2f);
		glColor4f(color[0], color[1], color[2], (0.1f+0.1f*randNum) * triBrightness * (1.0f + brightAdder) + 0.01f * randNum - 0.005f);
		float right[3];
		right[0] = tris.direction[i][1] * tris.normal[i][2] - tris.direction[i][2] * tris.normal[i][1];
		right[1] = tris.direction[i][2] * tris.normal[i][0] - tris.direction[i][0] * tris.normal[i][2];
		right[2] = tris.direction[i][0] * tris.normal[i][1] - tris.direction[i][1] * tris.normal[i][0];
		glNormal3fv(tris.normal[i]);
		glVertex3f(tris.position[i][0] + 0.6f * tris.direction[i][0],
					tris.position[i][1] + 0.6f * tris.direction[i][1],
					tris.position[i][2] + 0.6f * tris.direction[i][2] - 0.05f);
		glVertex3f(tris.position[i][0] + 0.6f * right[0] - 0.20f * tris.direction[i][0],
					tris.position[i][1] + 0.6f * right[1] - 0.20f * tris.direction[i][1],
					tris.position[i][2] + 0.6f * right[2] - 0.20f * tris.direction[i][2] - 0.001f);
		glVertex3f(tris.position[i][0] - 0.6f * right[0] - 0.20f * tris.direction[i][0],
					tris.position[i][1] - 0.6f * right[0] - 0.20f * tris.direction[i][1],
					tris.position[i][2] - 0.6f * right[0] - 0.20f * tris.direction[i][2] - 0.001f);
	}
	glEnd();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(TRUE);

	//glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

void hermaniak(float ftime)
{
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	// Beat-dependent variables:
	const int NUM_BPMS = 6;
	const float BPMArray[NUM_BPMS] = {124.0f, 96.0f, 106.0f, 128.0f, 136.0f, 136.0f};
	const int BPMParameterIndex[NUM_BPMS] = {23, 24, 25, 26, 27, 28};
	static int BPMIndex = 0; // This is static because it changes when I press a key...
	// Get the status fields (I need the numbers....)
	for (int i = 0; i < NUM_BPMS; i++)
	{
		// If a key press changed to pressed:
		if (keyPressed[BPMParameterIndex[i]] == 1)
		{
			BPMIndex = i;
		}
	}
	float BPM = BPMArray[BPMIndex];
	float BPS = BPM / 60.0f;
	// Here I should make something more sophisticated. I am thinking about an array... that has
	// Lengthes until the next jump in order to make a beat like ||..||..||..|.|.
	float jumpsPerSecond = BPS / 1.0f; // Jump on every fourth beat.
	static float phase = 0.0f;
	float jumpTime = (ftime * jumpsPerSecond) + phase;
	//float jumpTime = (ftime * jumpsPerSecond) + interpolatedParameters[22]; // JumpTime goes from 0 to 1 between two beats
	jumpTime -= floor(jumpTime);
	if (keyPressed[41] == 1)
	{
		phase -= jumpTime;
		jumpTime = 0.0f;
		if (phase < 0.0f) phase += 1.0;
	}
	jumpTime = jumpTime * jumpTime;
	// spike is between 0.0 and 1.0 depending on the position within whatever.
	float spike = 0.5f * cosf(jumpTime * 3.1415926f * 2.0f) + 0.5f;

	//parameterMatrix[0] = ftime; // time	
	// Steuerbare time
	static float shaderTime = 0.0f;
	static float lightShaderTime = 0.0f; // Time of the light background shader that paints on Hermann
	static float oldTime = 0.0f;
	float deltaTime = ftime - oldTime;
	oldTime = ftime;
	lightShaderTime += deltaTime * interpolatedParameters[6] * 3.0f;
	// here I need a time update based on beat time.
	// The problem is that I have the same timing on the
	// Hermann. I might not want that?
	// THe 1.6 is 1 divided by the integral of spike.
	// shaderTime and light shader Time shall be synchroneous, for the MainCOlorHSB.r!
	shaderTime = lightShaderTime + interpolatedParameters[6] * (interpolatedParameters[8] * spike - 0.5f);

	// Which background texture to use:
	static int bgTexture = 2;
	static float bgTextureXFade = 0.0f;
	// Go to texture change mode if necessary:
	if ((keyPressed[33] == 1) || (keyPressed[34] == 1) ||
		(keyPressed[35] == 1) || (keyPressed[36] == 1) ||
		(keyPressed[37] == 1))
	{
		glActiveTexture(GL_TEXTURE1);
		if (bgTexture > 1) backgroundTexture[bgTexture-2].setTexture();
		else
		{
			if (bgTexture == 0) videoTexture.captureFrame();
			else glBindTexture(GL_TEXTURE_2D, offscreenTexture[0]);
		}
		glActiveTexture(GL_TEXTURE0);
		bgTextureXFade = 1.0f;
	}
	bgTextureXFade -= deltaTime * 0.5f;
	bgTextureXFade = bgTextureXFade < 0.0f ? 0.0f : bgTextureXFade;
	if (keyPressed[33] == 1) bgTexture = 0;
	if (keyPressed[34] == 1) bgTexture = 1;
	if (keyPressed[35] == 1) bgTexture = 2;
	if (keyPressed[36] == 1) bgTexture = 3;
	if (keyPressed[37] == 1) bgTexture = 4;

	parameterMatrix[0] = lightShaderTime;
	/* shader parameters */
	parameterMatrix[1] = 10.0f * interpolatedParameters[20] * interpolatedParameters[20];  // bauchigkeit
	parameterMatrix[2] = interpolatedParameters[21];			// line strength
	parameterMatrix[3] = interpolatedParameters[14];			// color variation
	parameterMatrix[4] = interpolatedParameters[2];			// size.x			
	parameterMatrix[5] = interpolatedParameters[4];			// size.y
	parameterMatrix[6] = interpolatedParameters[3];			// spread.x	
	parameterMatrix[7] = interpolatedParameters[5];			// spread.y
	parameterMatrix[8] = interpolatedParameters[15];			// mainColor.h
	parameterMatrix[9] = interpolatedParameters[16];		// mainColor.s
	parameterMatrix[10] = interpolatedParameters[17] * 2.0f;		// mainColor.b
	parameterMatrix[11] = 5.0f * interpolatedParameters[18]; // highlightAmount
	parameterMatrix[12] = 0.9f * interpolatedParameters[12]; // color reduction
	parameterMatrix[13] = 2.0f * interpolatedParameters[13]; // hermann brightness
	parameterMatrix[14] = bgTextureXFade; // used texture.
	parameterMatrix[15] = interpolatedParameters[22]; // texture vignette
	glLoadMatrixf(parameterMatrix);

	int xres = windowRect.right - windowRect.left;
	int yres = windowRect.bottom - windowRect.top;
#ifndef NO_HERMANN_LIGHTING
	// DRAW the right hand view (lighting on hermann): I need to remove the bumping colors here
	glViewport(xres/2 + 10, 0, xres / 2 - 10, yres);
	glUseProgram(shaderPrograms[SHADER_BACKGROUND_LIGHT]);
	glRectf(-1.0, -1.0, 1.0, 1.0);
#endif

	// After left hand view drawing, apply the squiggly stuff
	parameterMatrix[10] += spike * interpolatedParameters[9]; // spiked mainColor.brightness
	parameterMatrix[0] = shaderTime;
	glLoadMatrixf(parameterMatrix);

	// DRAW the left hand view (background on projection)
	// draw offscreen (full offscreen resolution)
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_BACKGROUND]);
	// Use the video or normal texture:
	if (bgTexture > 1) backgroundTexture[bgTexture-2].setTexture();
	else
	{
		if (bgTexture == 0) videoTexture.captureFrame();
		else glBindTexture(GL_TEXTURE_2D, offscreenTexture[0]);
	}
	glRectf(-1.0, -1.0, 1.0, 1.0);
}

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

	int xres = windowRect.right - windowRect.left;
	int yres = windowRect.bottom - windowRect.top;
#ifndef NO_HERMANN_LIGHTING
	// DRAW the right hand view (lighting on hermann): I need to remove the bumping colors here
	glViewport(xres/2 + 10, 0, xres / 2 - 10, yres);
	glUseProgram(shaderPrograms[SHADER_BACKGROUND_LIGHT]);
	glRectf(-1.0, -1.0, 1.0, 1.0);
#endif

	// Those are key-Press indicators. I only act on 0-to-1.
	for (int i = 0; i < maxNumParameters; i++)
	{
		interpolatedParameters[i] = 0.95f * interpolatedParameters[i] +
									0.05f * params.getParam(i, defaultParameters[i]);
	}
	// Update key press events.
	for (int i = 0; i < NUM_KEYS; i++)
	{
		if (params.getParam(i, 0.0) > 0.5f)
		{
			keyPressed[i]++;
		}
		else
		{
			keyPressed[i] = 0;
		}
	}

    // render
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_BLEND);
    //glEnable( GL_CULL_FACE );
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


	if (whatToDo == 0)
	{
		hermaniak(ftime);
	}
	else
	{
		rhytmPolice(ftime);
	}


	/*
	 * GLOW
	 */
	// downsample to highlight resolution
	parameterMatrix[1] = 1.0f;
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = 1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 0.0f; // scanline amount
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[0]);
	// Copy backbuffer to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_COPY]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// horizontal blur
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = 1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 2.0f; // reduction
	parameterMatrix[5] = 0.01f * interpolatedParameters[19]; // gain
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer (small) to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_BLUR]);
	glRectf(-1.0, -1.0, 1.0, 1.0); // TODO: I think I can optimize this???

	// vertical blur
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = -1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 1.0f; // reduction
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_BLUR]);	
	glRectf(-1.0, -1.0, 1.0, 1.0); // TODO: I think I can optimze this???
	// Copy backbuffer to texture (for highlight)
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);

	// stranger blur
	// horizontal blur
	parameterMatrix[2] = 0.5f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = 1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 2.0f; // reduction
	parameterMatrix[5] = 0.01f * interpolatedParameters[19]; // gain
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer (small) to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_BLUR]);
	glRectf(-1.0, -1.0, 1.0, 1.0); // TODO: I think I can optimize this???

	// vertical blur
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = -0.5f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 1.0f; // reduction
	glLoadMatrixf(parameterMatrix);
	glViewport(0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	// Copy backbuffer to texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);
	glUseProgram(shaderPrograms[SHADER_BLUR]);	
	glRectf(-1.0, -1.0, 1.0, 1.0); // TODO: I think I can optimze this???
	// Copy backbuffer to texture (for highlight)
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT);

	// draw normal image
	parameterMatrix[1] = 1.0f;
	parameterMatrix[2] = 1.0f / OFFSCREEN_WIDTH;
	parameterMatrix[3] = 1.0f / OFFSCREEN_HEIGHT;
	parameterMatrix[4] = 0.0f; // 0.1fscanline amount
	glLoadMatrixf(parameterMatrix);
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
#ifndef NO_HERMANN_LIGHTING
	glViewport(0, 0, xres / 2, yres);
#else
	glViewport(0, 0, xres, yres);
#endif
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[0]); // was already copied!
	glUseProgram(shaderPrograms[SHADER_COPY]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	if (whatToDo == 0)
	{
		// Here I add the logo:
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		hLogoTexture.setTexture();
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

	// add highlight to front
	parameterMatrix[1] = 0.0f;
	parameterMatrix[2] = 1.0f / HIGHLIGHT_WIDTH;
	parameterMatrix[3] = 1.0f / HIGHLIGHT_HEIGHT;
	parameterMatrix[4] = 0.0f; // scanline amount
	glLoadMatrixf(parameterMatrix);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
#ifndef NO_HERMANN_LIGHTING
	glViewport(0, 0, xres / 2, yres);
#else
	glViewport(0, 0, xres, yres);
#endif
	glBindTexture(GL_TEXTURE_2D, offscreenTexture[1]);
	glUseProgram(shaderPrograms[SHADER_COPY]);
	glRectf(-1.0, -1.0, 1.0, 1.0);
}

