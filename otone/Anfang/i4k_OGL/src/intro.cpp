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
#define NUM_SHADERS 4
const GLchar shaderFileName[NUM_SHADERS][128] =
{
	"shaders/verystart.shader",
	"shaders/showTex.shader",
	"shaders/Background.shader",
	"shaders/redOverlays.shader.txt"
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
	float cameraCenterPoint[2] = {0.0f, 0.0f};
	static float updateCenterPoint[2] = {0.0f, 0.0f};
	float updateCenterDirection;
	float updateCenterSpeed;
	static float cameraRotationPhase = 0.0f;
	
	GLUquadric* quad = gluNewQuadric();

	// calculate update center position	
	float firstMDur = 258.25f;
	float musicSpeed = sin((ftime-35.0f-firstMDur) * 0.016f);
	if (ftime < 35.0f+firstMDur) musicSpeed = 0.0f;
	if (ftime > 183.0f+firstMDur) musicSpeed /= (ftime - 183.0f - firstMDur + 1.0f);
	cameraRotationPhase += deltaTime * musicSpeed;
	updateCenterDirection = 2.23f * sin(cameraRotationPhase * 0.173f + 2.51f) +
							1.82f * sin(cameraRotationPhase * 0.343f + 1.52f) +
							0.64f * sin(cameraRotationPhase * 0.611f + 4.55f) +
							0.61f * sin(cameraRotationPhase * 0.731f + 5.55f);
	updateCenterSpeed = 0.3f*musicSpeed;

	updateCenterPoint[0] += updateCenterSpeed * deltaTime * sin(0.2f * updateCenterDirection);
	updateCenterPoint[1] += updateCenterSpeed * deltaTime * cos(0.2f * updateCenterDirection);

	cameraCenterPoint[0] = updateCenterPoint[0];
	cameraCenterPoint[1] = updateCenterPoint[1];

	// set matrices
	float rotation = 0.3f * updateCenterDirection + 3.1415926f;
	glMatrixMode(GL_MODELVIEW);

	if (ftime < 240.0f) parameterMatrix[0] = ftime * ftime / 480.0f + 120.0f;
	else parameterMatrix[0] = ftime; // time
	if (ftime > 155.0f+firstMDur) parameterMatrix[0] = sqrtf(ftime-155.0f-firstMDur) + 155.0f + firstMDur;
	
	// Shatter light
	//static float nextShatterTime = EMISSION_UPDATE_TIME;
	static float nextShatterTime = 47.8f+firstMDur;
	if (ftime - nextShatterTime > 2.0f)
	{
		if (ftime < 103.0f+firstMDur || ftime > 136.0f+firstMDur) nextShatterTime += EMISSION_UPDATE_TIME;
		else nextShatterTime += EMISSION_UPDATE_TIME / 2;
		if (ftime > 135.0f+firstMDur) nextShatterTime = 1000.0f+firstMDur;
	}
	float shatterTimeDistance = fabsf(nextShatterTime - ftime + 0.5f);
	parameterMatrix[1] = 1.0f - shatterTimeDistance * 2.0f;
	parameterMatrix[1] = parameterMatrix[1] < 0.0f ? 0.0f : parameterMatrix[1];
	parameterMatrix[1] = sqrtf(parameterMatrix[1]);
	parameterMatrix[1] *= 0.5f*musicSpeed*musicSpeed + 0.1f;
	
	// fog amount
	if (ftime < 11.0f) parameterMatrix[3] = 0.0f;
	else if (ftime < 30.0f) parameterMatrix[3] = (ftime - 11.0f) / 19.0f;
	else parameterMatrix[3] = 1.0f;

	// Fade to black
	if (ftime < 5.0f) parameterMatrix[2] = 0.0f;
	else if (ftime < 25.0f) parameterMatrix[2] = (ftime-5.0f) / 20.0f;
	else parameterMatrix[2] = 1.0f;
	if (ftime > 185.0f+firstMDur) parameterMatrix[2] = 1.0f - (ftime - 185.0f - firstMDur) / 8.0f;
	if (parameterMatrix[2] < 0.0f) parameterMatrix[2] = 0.0f;

	// Beginning blackness of the red overlay
	parameterMatrix[5] = 1.0f;
	if (ftime > 57.0f) parameterMatrix[5] = 1.0f - (ftime - 57.0f) * 0.02f;
	if (parameterMatrix[5] < 0.0f) parameterMatrix[5] = 0.0f;

	// kimono strength
	parameterMatrix[4] = (ftime - firstMDur) * 0.02f;
	if (parameterMatrix[4] < 0.0f) parameterMatrix[4] = 0.0f;
	if (parameterMatrix[4] > 0.9f) parameterMatrix[4] = 0.9f;

	// singing red adder to yellow
	parameterMatrix[6] = 0.0f;
	if (ftime > 125.0f) parameterMatrix[6] = (ftime - 125) * 0.02f;
	if (parameterMatrix[6] > 0.5f) parameterMatrix[6] = 0.5f;
	if (ftime > 190.0f) parameterMatrix[6] = 0.5f - (ftime - 190.0f) * 0.02f;
	if (parameterMatrix[6] < 0.0f) parameterMatrix[6] = 0.0f;

	glLoadMatrixf(parameterMatrix);
	glMatrixMode(GL_PROJECTION);
	projectionMatrix[0] = cos(rotation);
	projectionMatrix[1] = -sin(rotation);
	projectionMatrix[4] = sin(rotation);
	projectionMatrix[5] = cos(rotation);
	projectionMatrix[3] = -(cameraCenterPoint[0] * cos(rotation) - cameraCenterPoint[1] * sin(rotation));
	projectionMatrix[7] = -(cameraCenterPoint[1] * cos(rotation) + cameraCenterPoint[0] * sin(rotation)) - 0.3f;
	glLoadMatrixf(projectionMatrix);

	// Bind the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	// draw background offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[0]);
	textures.setTexture(32); // background texture
	// draw a quad...
	float color[3] = {0.95f, 1.0f, 0.95f};
	textures.drawScreenAlignedQuad(color, cameraCenterPoint[0] - 2.0f, cameraCenterPoint[1] - 2.0f, cameraCenterPoint[0] + 2.0f, cameraCenterPoint[1] + 2.0f);

	// draw the particle engine
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (ftime > firstMDur)
	{
		// TODO: put motion blur here, together with short camera updates!
		particleEngine.update(deltaTime, updateCenterPoint, 0.2f - 0.17f * musicSpeed);		
		glUseProgram(shaderPrograms[1]);
		particleEngine.draw(cameraCenterPoint);
	}

	// Draw the noise
	glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
	//glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
	glUseProgram(shaderPrograms[2]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	// draw a quad...
	textures.drawScreenAlignedQuad(color, cameraCenterPoint[0] - 2.0f, cameraCenterPoint[1] - 2.0f, cameraCenterPoint[0] + 2.0f, cameraCenterPoint[1] + 2.0f);

	// Draw the red overlay
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(shaderPrograms[3]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	textures.drawScreenAlignedQuad(color, cameraCenterPoint[0] - 2.0f, cameraCenterPoint[1] - 2.0f, cameraCenterPoint[0] + 2.0f, cameraCenterPoint[1] + 2.0f);

	// Go back to regular drawing
	glDisable(GL_BLEND);

	// reset the projection stuff.
	glMatrixMode(GL_PROJECTION);
	projectionMatrix[0] = 1.0f;
	projectionMatrix[1] = 0.0f;
	projectionMatrix[4] = 0.0f;
	projectionMatrix[5] = (float)realXRes / (float)realYRes;
	projectionMatrix[3] = 0.0f;
	projectionMatrix[7] = 0.0f;
	glLoadMatrixf(projectionMatrix);

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