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
#include "bass.h"

#include "config.h"
#include "intro.h"
#include "Parameter.h"
#include "noiseTexture.h"
#include "Texture.h"
#include "ParticleEngine.h"

float frand();

Texture textures;
GLuint frameBuffer;
ParticleEngine particleEngine;

// music data
HSTREAM mp3Str = 0;

extern int realXRes, realYRes;

/* Number of names of the used shaders */
#define NUM_SHADERS 5
const GLchar shaderFileName[NUM_SHADERS][128] =
{
	"shaders/verystart.shader",
	"shaders/showTex.shader",
	"shaders/Background.shader",
	"shaders/redOverlays.shader.txt",
	"shaders/simpleTex.shader"
};
/* Location where the loaded shader is stored */
#define MAX_SHADER_LENGTH 200000
GLchar fragmentMainBackground[MAX_SHADER_LENGTH];

// Constant shader code that is used for all the effects.
#if 1
const GLchar *fragmentOffscreenCopy="\
uniform sampler2D Texture0;\n\
varying vec3 objectPosition;\n\
varying mat4 parameters;\n\
\n\
void main(void)\n\
{  \n\
   float fTime0_X = parameters[0][0];\n\
   vec3 noisePos = objectPosition + fTime0_X;\n\
   vec2 noiseVal;\n\
   // Stupid antialiasing\n\
   gl_FragColor = 0.25 * texture2D(Texture0, 0.5*objectPosition.xy + vec2(0.5001, 0.49995));\n\
   gl_FragColor += 0.25 * texture2D(Texture0, 0.5*objectPosition.xy + vec2(0.49995, 0.5001));\n\
   gl_FragColor += 0.25 * texture2D(Texture0, 0.5*objectPosition.xy + vec2(0.4999, 0.50005));\n\
   gl_FragColor += 0.25 * texture2D(Texture0, 0.5*objectPosition.xy + vec2(0.50005, 0.4999));\n\
   float vignette = objectPosition.x*objectPosition.x + objectPosition.y*objectPosition.y;\n\
   gl_FragColor *= 1.0 - vignette * 0.6; // darken\n\
   float meanColor = 0.3 * gl_FragColor.r + 0.59 * gl_FragColor.r + 0.11 * gl_FragColor.b;\n\
   gl_FragColor = 0.4 * vignette * vec4(meanColor) + (1.0 - 0.4 * vignette) * gl_FragColor; // desaturate\n\
	//gl_FragColor.rgb = (1.0+0.5*parameters[1][2]) * (gl_FragColor.rgb) - 0.5*parameters[1][2] * vec3(meanColor);\n\
	gl_FragColor.rgb = (1.5+parameters[1][2]) * (gl_FragColor.rgb) - (0.5+parameters[1][2])*vec3(meanColor);\n\
   gl_FragColor *= vec4(parameters[0][2]); //blackout\n\
   \n\
   //gl_FragColor = texture2D(Texture0, 0.5*objectPosition.xy + 0.5).bgra;\n\
\n\
}";
#else
const GLchar *fragmentOffscreenCopy="\
uniform sampler2D Texture0;\n\
varying vec3 objectPosition;\n\
varying mat4 parameters;\n\
\n\
void main(void)\n\
{  \n\
   float fTime0_X = parameters[0][0];\n\
   vec3 noisePos = objectPosition + fTime0_X;\n\
   vec2 noiseVal;\n\
   gl_FragColor = texture2D(Texture0, 0.5*objectPosition.xy + 0.5);\n\
\n\
}";
#endif

const GLchar *vertexMainObject="\
#version 120\n\
varying vec3 objectPosition;\
varying mat4 parameters;\
varying vec4 color;\
\
void main(void)\
{\
   parameters = gl_ModelViewMatrix;\
   objectPosition = vec3(gl_Vertex.x, gl_Vertex.y, 1.0);\
   gl_TexCoord[0] = gl_MultiTexCoord0;\n\
   color = gl_Color;\n\
   vec2 pos;\n\
   pos.x = gl_Vertex.x * gl_ProjectionMatrix[0][0] + gl_Vertex.y * gl_ProjectionMatrix[0][1] + gl_ProjectionMatrix[0][3];\n\
   pos.y = gl_Vertex.x * gl_ProjectionMatrix[1][0] + gl_Vertex.y * gl_ProjectionMatrix[1][1] + gl_ProjectionMatrix[1][3];\n\
   gl_Position = vec4(pos.x, pos.y, 0.5, 1.0);\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

#define NUM_GL_NAMES 15
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
	 "glDeleteProgram", "glDeleteShader",
	 "glGenFramebuffers", "glBindFramebuffer", "glFramebufferTexture2D"
};

#define glCreateShader ((PFNGLCREATESHADERPROC)glFP[0])
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)glFP[1])
#define glShaderSource ((PFNGLSHADERSOURCEPROC)glFP[2])
#define glCompileShader ((PFNGLCOMPILESHADERPROC)glFP[3])
#define glAttachShader ((PFNGLATTACHSHADERPROC)glFP[4])
#define glLinkProgram ((PFNGLLINKPROGRAMPROC)glFP[5])
#define glUseProgram ((PFNGLUSEPROGRAMPROC)glFP[6])
#define glTexImage3D ((PFNGLTEXIMAGE3DPROC)glFP[7])
#define glGetShaderiv ((PFNGLGETSHADERIVPROC)glFP[8])
#define glGetShaderInfoLog ((PFNGLGETSHADERINFOLOGPROC)glFP[9])
#define glDeleteProgram ((PFNGLDELETEPROGRAMPROC)glFP[10])
#define glDeleteShader ((PFNGLDELETESHADERPROC)glFP[11])
#define glGenFramebuffers ((PFNGLGENFRAMEBUFFERSPROC)glFP[12])
#define glBindFramebuffer ((PFNGLBINDFRAMEBUFFERPROC)glFP[13])
#define glFramebufferTexture2D ((PFNGLFRAMEBUFFERTEXTURE2DPROC)glFP[14])


// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];
static float projectionMatrix[16];
static GLuint offscreenTexture;
static int noiseTmp[4];

typedef void (*GenFP)(void); // pointer to openGL functions
static GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderPrograms[NUM_SHADERS];
static GLuint shaderCopyProgram;

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

void compileShaders(void)
{
	const GLchar *shaderPointer = fragmentMainBackground;
	//const char *shaderFileName = "shaders/ball8.shader";

	// delete objects
	if (shaderPrograms[0])
	{
		for (int i = 0; i < NUM_SHADERS; i++)
		{
			glDeleteProgram(shaderPrograms[i]);
			shaderPrograms[i] = 0;
		}
		glDeleteProgram(shaderCopyProgram);
		
		shaderCopyProgram = 0;
	}

	/* Generate general programs */
	int tmp, tmp2;
	char err[4097];
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fOffscreenCopy = glCreateShader(GL_FRAGMENT_SHADER);	
	shaderCopyProgram = glCreateProgram();
	glShaderSource(vMainObject, 1, &vertexMainObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(fOffscreenCopy, 1, &fragmentOffscreenCopy, NULL);
	glCompileShader(fOffscreenCopy);
	// Check programs
	glGetShaderiv(vMainObject, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(vMainObject, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "vMainObject shader error", MB_OK);
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
	glAttachShader(shaderCopyProgram, vMainObject);
	glAttachShader(shaderCopyProgram, fOffscreenCopy);
	glLinkProgram(shaderCopyProgram);

	/* Generate shader specific programs */
	for (int shaderIdx = 0; shaderIdx < NUM_SHADERS; shaderIdx++)
	{
		// load shader from file
		FILE *fid = fopen(shaderFileName[shaderIdx], "rb");
		int numRead = fread(fragmentMainBackground, 1, MAX_SHADER_LENGTH-1,fid);
		fragmentMainBackground[numRead] = 0;
		fclose(fid);

		// init objects:
		GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);
		shaderPrograms[shaderIdx] = glCreateProgram();
	
		// compile sources:
		glShaderSource(fMainBackground, 1, &shaderPointer, NULL);
		glCompileShader(fMainBackground);
		
		// Check programs
		glGetShaderiv(fMainBackground, GL_COMPILE_STATUS, &tmp);
		if (!tmp)
		{
			glGetShaderInfoLog(fMainBackground, 4096, &tmp2, err);
			err[tmp2]=0;
			MessageBox(hWnd, err, shaderFileName[shaderIdx], MB_OK);
			return;
		}

		// link shaders:
		glAttachShader(shaderPrograms[shaderIdx], vMainObject);
		glAttachShader(shaderPrograms[shaderIdx], fMainBackground);
		glLinkProgram(shaderPrograms[shaderIdx]);
		
		glDeleteShader(fMainBackground);
	}

	glDeleteShader(vMainObject);	
	glDeleteShader(fOffscreenCopy);
}

void intro_init( HWND mainWnd )
{
	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// create noise Texture (static)
	generateNoiseTexture();

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop..
	compileShaders();

	// Set texture.
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	// Missing: mip mapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#ifdef FLOAT_TEXTURE
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
				 NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
				 0, GL_RGBA, GL_FLOAT, noiseData);
#else
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
	//			 NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
	//			 0, GL_RGBA, GL_FLOAT, noiseData);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
	//			 NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
	//			 0, GL_RGBA, GL_UNSIGNED_BYTE, noiseIntData);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
	//			      GL_RGBA, GL_FLOAT, noiseData);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
				      GL_BGRA, GL_UNSIGNED_BYTE, noiseIntData);
#endif

	// Create a rendertarget texture
	/*glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			     OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT,
				 0, GL_RGBA, GL_UNSIGNED_BYTE, &offscreenTexture);*/
	glGenTextures(1, &offscreenTexture);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT, 0,
		         GL_RGBA, GL_UNSIGNED_BYTE, 0);
	//glBindTexture(GL_TEXTURE_2D, 0);

	// Framebuffer
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER,
						   GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, offscreenTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Load textures
	if (!textures.init())
	{
		while(1);
	}

	// RLY?
	//glEnable(GL_CULL_FACE);

	// start music playback
	int bassinit = BASS_Init(1,44100,0,mainWnd,NULL);
	mp3Str=BASS_StreamCreateFile(FALSE,"../../soumu_ruten.mp3",0,0,0);
	int bassplay = BASS_ChannelPlay(mp3Str, TRUE);
	// stream forward...
	//BASS_ChannelSetPosition(mp3Str, 6760000, BASS_POS_BYTE); // short before boom
	//BASS_ChannelSetPosition(mp3Str, 94560000, BASS_POS_BYTE); // 250
	//BASS_ChannelSetPosition(mp3Str, 115250000, BASS_POS_BYTE); // 300
	BASS_Start();
}

void veryStartScene(float ftime)
{
	static float lastTime[20] = {0.0f};
	float deltaTime = (ftime - lastTime[19]) / 20.0f;
	for (int i = 19; i > 0; i--)
	{
		lastTime[i] = lastTime[i-1];
	}
	lastTime[0] = ftime;
	static float fCurTime = 00.0f;
	fCurTime += deltaTime;

	float cameraCenterPoint[2] = {0.0f, 0.0f};
	static float updateCenterPoint[2] = {0.0f, 0.0f};
	float updateCenterDirection;
	float updateCenterSpeed;
	static float cameraRotationPhase = 0.0f;
	
	GLUquadric* quad = gluNewQuadric();

	// calculate update center position	
	float firstMDur = 258.25f;
	float musicSpeed = sin((fCurTime-35.0f-firstMDur) * 0.016f);
	if (fCurTime < 35.0f+firstMDur) musicSpeed = 0.0f;
	if (fCurTime > 183.0f+firstMDur) musicSpeed /= (fCurTime - 183.0f - firstMDur + 1.0f);
	cameraRotationPhase += deltaTime * musicSpeed;
	updateCenterDirection = 1.23f * sin(cameraRotationPhase * 0.073f + 2.51f) +
							0.82f * sin(cameraRotationPhase * 0.143f + 1.52f) +
							0.34f * sin(cameraRotationPhase * 0.311f + 4.55f) +
							0.31f * sin(cameraRotationPhase * 0.331f + 5.55f);
	updateCenterSpeed = 0.2f*musicSpeed;
	updateCenterPoint[0] += updateCenterSpeed * deltaTime * sin(0.2f * updateCenterDirection);
	updateCenterPoint[1] += updateCenterSpeed * deltaTime * cos(0.2f * updateCenterDirection);

	// Shatter light
	//static float nextShatterTime = EMISSION_UPDATE_TIME;
	static float nextShatterTime = 47.8f+firstMDur;
	static int shatterIdx = 0;
	if (fCurTime - nextShatterTime > 2.0f)
	{
		shatterIdx++;
		if (fCurTime < 103.0f+firstMDur || fCurTime > 145.0f+firstMDur) nextShatterTime += EMISSION_UPDATE_TIME;
		else nextShatterTime += EMISSION_UPDATE_TIME / 2;
		//if (fCurTime > 173.0f+firstMDur) nextShatterTime = 1000.0f+firstMDur;
	}
	float shatterTimeDistance = (fCurTime - nextShatterTime);
	if (shatterTimeDistance > 0.0f && shatterIdx > 17) shatterTimeDistance = -100.0f;
	float shatter1amount = shatterTimeDistance;
	if (shatter1amount < 0.0f) shatter1amount = 1000.0f; // no backtime
	parameterMatrix[1] = 1.0f - shatter1amount * 2.0f;
	parameterMatrix[1] = parameterMatrix[1] < 0.0f ? 0.0f : parameterMatrix[1];
	parameterMatrix[1] = sqrtf(parameterMatrix[1]);
	parameterMatrix[1] *= 0.5f*musicSpeed*musicSpeed + 0.1f;

	// set matrices
	float rotation = 0.3f * updateCenterDirection + 3.1415926f;
	float inverseTime = (260.0f - fCurTime);
	if (inverseTime > 0.0f)
	{
		if (inverseTime < 50.0f) rotation -= inverseTime * inverseTime / 50.0f * 0.5f * 0.12f;
		else rotation -= (inverseTime - 25.0f) * 0.12f;
	}

	// rotation is increased on shatter
	if (shatterTimeDistance > 0.0)
	{
		float ramp = shatterTimeDistance * 10.0f;
		if (ramp > 0.3f) ramp = 0.3f;
		float rotationAdder = ramp / (float)exp(shatterTimeDistance * 4.0f);
		if (rotationAdder > 0.0f)
		{
			if ((shatterIdx % 2) == 0) rotationAdder = -rotationAdder;
			rotation += rotationAdder;

			updateCenterPoint[0] += 3.0f * deltaTime * sin(rotation) * fabsf(rotationAdder);
			updateCenterPoint[1] += 3.0f * deltaTime * cos(rotation) * fabsf(rotationAdder);
		}		
	}

	cameraCenterPoint[0] = updateCenterPoint[0];
	cameraCenterPoint[1] = updateCenterPoint[1] - 0.3f;

	parameterMatrix[8] = cameraCenterPoint[0];
	parameterMatrix[9] = cameraCenterPoint[1];


	glMatrixMode(GL_MODELVIEW);

	if (fCurTime < 240.0f) parameterMatrix[0] = fCurTime * fCurTime / 480.0f + 120.0f;
	else parameterMatrix[0] = fCurTime; // time
	if (fCurTime > 155.0f+firstMDur) parameterMatrix[0] = sqrtf(fCurTime-155.0f-firstMDur) + 155.0f + firstMDur;
	parameterMatrix[0] *= 0.4f; // slow down...
	
	// fog amount
	//else if (fCurTime < 30.0f) parameterMatrix[3] = (fCurTime - 11.0f) / 19.0f;
	//else parameterMatrix[3] = 1.0f;
	parameterMatrix[3] = 0.0f;
	if (fCurTime > 260.0f) parameterMatrix[3] = (fCurTime - 260.0f) / 30.0f;
	if (parameterMatrix[3] > 1.0f) parameterMatrix[3] = 1.0f;

	// Fade to black
	if (fCurTime < 5.0f) parameterMatrix[2] = 0.0f;
	else if (fCurTime < 25.0f) parameterMatrix[2] = (fCurTime-5.0f) / 20.0f;
	else parameterMatrix[2] = 1.0f;
	if (fCurTime > 180.0f+firstMDur) parameterMatrix[2] = 1.0f - (fCurTime - 180.0f - firstMDur) / 10.0f;
	if (parameterMatrix[2] < 0.0f) parameterMatrix[2] = 0.0f;
	if (shatterTimeDistance > -2.0f && shatterTimeDistance < 0.0f)
	{
		float amount = 0.5f * sin(4.0f / (shatterTimeDistance-0.2f)) + 0.5f;
		amount *= 0.12f * (2.0f + shatterTimeDistance);
		parameterMatrix[2] *= 1.0f - amount;
	}

	// Beginning blackness of the red overlay
	parameterMatrix[5] = 1.0f;
	if (fCurTime > 57.0f) parameterMatrix[5] = 1.0f - (fCurTime - 57.0f) * 0.02f;
	if (parameterMatrix[5] < 0.0f) parameterMatrix[5] = 0.0f;
	// What does this do?
	parameterMatrix[5] = 0.0f;

	// kimono strength
	parameterMatrix[4] = (fCurTime - firstMDur) * 0.02f;
	if (parameterMatrix[4] < 0.0f) parameterMatrix[4] = 0.0f;
	if (parameterMatrix[4] > 0.9f) parameterMatrix[4] = 0.9f;
	// This is the background musterung of the stoff

	// Fog overdrive
	parameterMatrix[6] = 0.0f;
#if 0
	if (fCurTime > 125.0f) parameterMatrix[6] = (fCurTime - 125) * 0.02f;
	if (parameterMatrix[6] > 0.5f) parameterMatrix[6] = 0.5f;
	if (fCurTime > 190.0f) parameterMatrix[6] = 0.5f - (fCurTime - 190.0f) * 0.02f;
	if (parameterMatrix[6] < 0.0f) parameterMatrix[6] = 0.0f;
#endif
	if (fCurTime > 260.0f)
	{
		parameterMatrix[6] = (fCurTime - 260.0f) / 50.0f;
		if (parameterMatrix[6] > 0.45f) parameterMatrix[6] = 0.45f;
	}
	// I do not need it?

	glLoadMatrixf(parameterMatrix);
	glMatrixMode(GL_PROJECTION);
	projectionMatrix[0] = cos(rotation);
	projectionMatrix[1] = -sin(rotation);
	projectionMatrix[4] = sin(rotation);
	projectionMatrix[5] = cos(rotation);
	projectionMatrix[3] = -(cameraCenterPoint[0] * cos(rotation) - cameraCenterPoint[1] * sin(rotation));
	projectionMatrix[7] = -(cameraCenterPoint[1] * cos(rotation) + cameraCenterPoint[0] * sin(rotation));
	glLoadMatrixf(projectionMatrix);

	// Bind the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	// draw background offscreen
	// TODO: less y-range!
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[0]);
	textures.setTexture(32); // background texture
	// draw a quad...
	//float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	textures.drawScreenAlignedQuad(color, cameraCenterPoint[0] - 2.0f, cameraCenterPoint[1] - 2.0f, cameraCenterPoint[0] + 2.0f, cameraCenterPoint[1] + 2.0f);

	// draw the particle engine
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 0
	if (fCurTime > firstMDur)
	{
		// TODO: put motion blur here, together with short camera updates!
		particleEngine.update(deltaTime, updateCenterPoint, 0.2f - 0.17f * musicSpeed);		
		glUseProgram(shaderPrograms[1]);
		particleEngine.draw(cameraCenterPoint);
	}
#endif

	// Draw the noise
	glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
	//glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
	glUseProgram(shaderPrograms[2]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	// draw a quad...
	color[0] = 0.7f;
	color[1] = 0.8f;
	color[2] = 1.0f;
	color[3] = 1.0f;
	textures.drawScreenAlignedQuad(color, cameraCenterPoint[0] - 2.0f, cameraCenterPoint[1] - 2.0f, cameraCenterPoint[0] + 2.0f, cameraCenterPoint[1] + 2.0f);

	// Draw the red overlay
#if 0
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(shaderPrograms[3]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	textures.drawScreenAlignedQuad(color, cameraCenterPoint[0] - 2.0f, cameraCenterPoint[1] - 2.0f, cameraCenterPoint[0] + 2.0f, cameraCenterPoint[1] + 2.0f);
#endif

	// Draw the rotating muster stuff
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(shaderPrograms[4]);
	const int numSquares = 920;
	for (int i = numSquares - 1; i > 0; i--)
	{
		float z = (float)i * 0.5f + 50.0f - fCurTime * 2.0f;
		if (z > 1.0f && z < 50.0f)
		{
			//textures.setTexture(TEX_SQUARE1 + ((i * (i+11) + i)%TEX_NUM_SQUARES));
			textures.setTexture(TEX_SQUARE1 + (i)%TEX_NUM_SQUARES);
			// draw a quad...
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			if (z < 4.0f) color[3] = (z - 1.0f) / 3.0f;
			if (z > 40.0f) color[3] = (50.0f - z) / 10.0f;

			// Position based on i
			float xPos = (((i * (i + 37)) % 59) + (i * i * i) % 41) * 0.4f - 20.0f;
			float yPos = (((i * (i + 27)) % 49) + (i * i * (i+10)) % 51) * 0.4f - 20.0f;

			textures.drawScreenAlignedQuad(color, (xPos-2.5f) / z, (yPos-2.5f) / z, (xPos+2.5f) / z, (yPos+2.5f) / z);
			//glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		}
	}

	// projection based on whatever.
	glMatrixMode(GL_PROJECTION);
	projectionMatrix[0] = 1.0f;
	projectionMatrix[1] = 0.0f;
	projectionMatrix[4] = 0.0f;
	projectionMatrix[5] = 1.0f;
	projectionMatrix[3] = 0.0f;
	projectionMatrix[7] = 0.0f;
	glLoadMatrixf(projectionMatrix);

	// Draw the final butterfly
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(shaderPrograms[4]); // simpleTex.shader
	float buttRotation = 0.5f + 0.05f * sin(fCurTime * 2.0f) + 0.05f * sin(fCurTime * 3.1f);
	float buttX = (fCurTime-186.0f-firstMDur) * 0.2f + sin(fCurTime * 3.7f) * 0.05f + 0.3f;
	float buttY = (fCurTime-186.0f-firstMDur) * 0.2f + sin(fCurTime * 4.3f) * 0.02f;
	float wingPos = 0.5f + 0.5f * sin(fCurTime * 13.0f);
	glColor4f(0.75f, 0.8f, 0.5f, 1.0f);
	textures.setTexture(TEX_BUTTERFLY_WING);
	glBegin(GL_QUADS);
	//textures.drawScreenAlignedQuad(color, -0.1f, 0.035f, 0.16f, 0.16f);
	float xpw1[4] = {-0.1f, 0.16f, 0.16f, -0.1f};
	float ypw1[4] = {0.16f, 0.16f, 0.035f, 0.035f};
	for (int i = 0; i < 4; i++)
	{
		if (i < 2) ypw1[i] -= wingPos * 0.05f;
		else ypw1[i] += wingPos*0.05f;
		float x = xpw1[i]*cos(buttRotation) - ypw1[i]*sin(buttRotation);
		float y = ypw1[i]*cos(buttRotation) + xpw1[i]*sin(buttRotation);
		if (i < 2) x += wingPos * 0.03f;
		else x += wingPos * 0.02f;
		x+=buttX;
		y+=buttY;
		xpw1[i] = x;
		ypw1[i] = y;
	}
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(xpw1[0], ypw1[0], 0.5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(xpw1[1], ypw1[1], 0.5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(xpw1[2], ypw1[2], 0.5);
	glTexCoord2f(0.0, 0.0f);
	glVertex3f(xpw1[3], ypw1[3], 0.5);
	glEnd();	
	textures.setTexture(TEX_BUTTERFLY_BODY);
	glColor4f(0.85f, 0.95f, 0.6f, 1.0f);
	//textures.drawScreenAlignedQuad(color, -0.05f, -0.08f, 0.06f, 0.05f);
	glBegin(GL_QUADS);
	float xpb[4] = {-0.05f, 0.06f, 0.06f, -0.05f};
	float ypb[4] = {0.05f, 0.05f, -0.08f, -0.08f};
	for (int i = 0; i < 4; i++)
	{
		ypb[i] += wingPos * 0.04f;
		float x = xpb[i]*cos(buttRotation) - ypb[i]*sin(buttRotation);
		float y = ypb[i]*cos(buttRotation) + xpb[i]*sin(buttRotation);
		x+=buttX;
		y+=buttY;
		xpb[i] = x;
		ypb[i] = y;
	}
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(xpb[0], ypb[0], 0.5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(xpb[1], ypb[1], 0.5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(xpb[2], ypb[2], 0.5);
	glTexCoord2f(0.0, 0.0f);
	glVertex3f(xpb[3], ypb[3], 0.5);
	glEnd();
	textures.setTexture(TEX_BUTTERFLY_WING);
#if 1
	glBegin(GL_QUADS);
	//textures.drawScreenAlignedQuad(color, -0.1f, 0.035f, 0.16f, 0.16f);
	float xpw2[4] = {-0.1f, 0.16f, 0.16f, -0.1f};
	float ypw2[4] = {0.16f, 0.16f, 0.035f, 0.035f};
	for (int i = 0; i < 4; i++)
	{
		if (i < 2) ypw2[i] -= wingPos * 0.08f;
		else ypw2[i] += wingPos*0.04f;
		float x = xpw2[i]*cos(buttRotation) - ypw2[i]*sin(buttRotation);
		float y = ypw2[i]*cos(buttRotation) + xpw2[i]*sin(buttRotation);
		if (i < 2) x -= wingPos * 0.07f;
		else x -= wingPos * 0.01f;
		x+=buttX;
		y+=buttY;
		xpw2[i] = x;
		ypw2[i] = y;
	}
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(xpw2[0], ypw2[0], 0.5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(xpw2[1], ypw2[1], 0.5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(xpw2[2], ypw2[2], 0.5);
	glTexCoord2f(0.0, 0.0f);
	glVertex3f(xpw2[3], ypw2[3], 0.5);
	glEnd();
#endif



	// reset the projection stuff.
	glMatrixMode(GL_PROJECTION);
	projectionMatrix[0] = 1.0f;
	projectionMatrix[1] = 0.0f;
	projectionMatrix[4] = 0.0f;
	projectionMatrix[5] = (float)realXRes / (float)realYRes;
	projectionMatrix[3] = 0.0f;
	projectionMatrix[7] = 0.0f;
	glLoadMatrixf(projectionMatrix);

	// Go back to regular drawing
	glDisable(GL_BLEND);

	// unbind the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	//glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

    // render
	glDisable(GL_DEPTH_TEST);
	//glEnable(GL_BLEND);
    glDisable( GL_CULL_FACE );
	//glCullFace( GL_FRONT );
	//glDisable( GL_BLEND );
    glDisable(GL_LIGHTING);
    //glEnable( GL_LIGHT0 );
    //glEnable( GL_NORMALIZE );
	//glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);	
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

	// clear screan:
	//glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	//glClear(GL_COLOR_BUFFER_BIT);

	//glBindTexture(GL_TEXTURE_3D, noiseTexture); // 3D noise?	

	/* Set everything to beginning */
	for (int i = 0; i < 16; i++)
	{
		parameterMatrix[i] = 0.0f;
	}
	
	float tt = ftime;
	veryStartScene(tt);
}

void intro_end()
{
	// music uninit
	BASS_ChannelStop(mp3Str);
	BASS_StreamFree(mp3Str);
	BASS_Free();
}