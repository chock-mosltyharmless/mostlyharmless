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
#include "Parameter.h"
#include "noiseTexture.h"

struct TGAHeader
{
	unsigned char identSize;
	unsigned char colourmapType;
	unsigned char imageType;

	// This is a stupid hack to fool the compiler.
	// I do not know what happens if I compile it
	// under release conditions.
	unsigned char colourmapStart1;
	unsigned char colourmapStart2;
	unsigned char colourmapLength1;
	unsigned char colourmapLength2;
	unsigned char colourmapBits;

	short xStart;
	short yStart;
	short width;
	short height;
	unsigned char bits;
	unsigned char descriptor;
};


float frand();

extern int realXRes;
extern int realYRes;

/* Number of names of the used shaders */
#define NUM_SHADERS 9
const GLchar shaderFileName[NUM_SHADERS][128] =
{
	"shaders/verystart.shader",
	"shaders/showTex.shader",
	"shaders/cloudstuff.1.shader",
	"shaders/gras20.shader",
	"shaders/gras10.shader",
	"shaders/gras12.2.shader",
	"shaders/ball8.shader",
	"shaders/Background.shader",
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
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43711.5453);\n\
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43717.5453);\n\
   gl_FragColor = 0.33 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy);\n\
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43731.5453);\n\
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43737.5453);\n\
   gl_FragColor += 0.33 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy);\n\
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43758.5453);\n\
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43753.5453);\n\
   gl_FragColor += 0.33 * texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.03 - 0.03;\n\
\n\
   float vignette = objectPosition.x*objectPosition.x + objectPosition.y*objectPosition.y;\n\
   //vignette = sqrt(vignette);\n\
   gl_FragColor *= 1.2 - vignette * 0.3; // darken\n\
   float meanColor = 0.3 * gl_FragColor.r + 0.59 * gl_FragColor.r + 0.11 * gl_FragColor.b;\n\
   gl_FragColor = 0.2 * vignette * vec4(meanColor) + (1.0 - 0.6 * vignette) * gl_FragColor; // desaturate\n\
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
   gl_TexCoord[0] = gl_MultiTexCoord0;\n\
   color = gl_Color;\n\
   objectPosition = vec3(gl_Vertex.x, gl_Vertex.y, 1.0);\
   gl_Position = vec4(gl_Vertex.x, gl_Vertex.y, 0.5, 1.0);\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

#define NUM_GL_NAMES 12
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
	 "glDeleteProgram", "glDeleteShader"
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


// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];
static GLuint offscreenTexture;
static int noiseTmp[4];
static GLuint creditsTexture;
static unsigned int creditsTexData[1024][1024];

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

void intro_init( void )
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

	// Load credits texture
	glGenTextures(1, &creditsTexture);
	glBindTexture(GL_TEXTURE_2D, creditsTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	FILE *cfpid = fopen("credits.tga", "rb");
	if (cfpid == 0) return;
	// load header
	TGAHeader tgaHeader;
	fread(&tgaHeader, 1, sizeof(tgaHeader), cfpid);		
	// load image data
	int textureSize = 1024 * 1024 * 4;
	fread(creditsTexData, 1, textureSize, cfpid);	
	// TODO: Mip Mapping!!!!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, 1024, 1024,
					  GL_BGRA, GL_UNSIGNED_BYTE, creditsTexData);
	fclose(cfpid);

	// RLY?
	//glEnable(GL_CULL_FACE);
}



void drawQuad(float startX, float startY, float startV, float alpha)
{
	float endY = startY + 0.15f;
	float endX = startX + 2.0f;
	float endV = startV + 0.1f;

	glColor4f(0.75f + 0.25f*alpha, 1.0f, 1.0f, alpha);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, endV);
	glVertex3f(startX, endY, 0.5);
	glTexCoord2f(1.0f, endV);
	glVertex3f(endX, endY, 0.5);
	glTexCoord2f(1.0f, startV);
	glVertex3f(endX, startY, 0.5);
	glTexCoord2f(0.0, startV);
	glVertex3f(startX, startY, 0.5);
	glEnd();

}


void veryStartScene(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	parameterMatrix[0] = ftime; // time
	float totalSize = 0.0f;
	if (ftime > 33.0f)
	{
		totalSize += (ftime - 33.0f) * (ftime - 33.0f) * 0.4f;
	}
	if (ftime < 12.0f)
	{
		totalSize += (12.0f - ftime) * (12.0f - ftime) * 0.05f;
	}
	parameterMatrix[1] = totalSize;
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[0]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);

	// draw credits
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, creditsTexture);
	glUseProgram(shaderPrograms[8]);

	if (ftime > 5.0f && ftime < 11.0f)
	{
		float ctime = ftime - 8.0f;
		float xp = -1.3f - 0.003f*ctime*ctime*ctime;
		float alpha = 1.0f - ctime*ctime/8.0f;
		drawQuad(xp, 0.45f, 0.9f, alpha);
		xp = -1.3f + 0.003f*ctime*ctime*ctime;
		drawQuad(xp, 0.3f, 0.79f, alpha);
	}
	else if (ftime < 19.0f)
	{
		float ctime = ftime - 16.0f;
		float xp = -1.3f - 0.003f*ctime*ctime*ctime;
		float alpha = 1.0f - ctime*ctime/8.0f;
		drawQuad(xp, 0.45f, 0.68f, alpha);
		xp = -1.3f + 0.003f*ctime*ctime*ctime;
		drawQuad(xp, 0.3f, 0.57f, alpha);
	}
	else if (ftime < 27.0f)
	{
		float ctime = ftime - 24.0f;
		float xp = -1.3f - 0.003f*ctime*ctime*ctime;
		float alpha = 1.0f - ctime*ctime/8.0f;
		drawQuad(xp, 0.45f, 0.46f, alpha);
		xp = -1.3f + 0.003f*ctime*ctime*ctime;
		drawQuad(xp, 0.3f, 0.35f, alpha);
	}
	else if (ftime < 35.0f)
	{
		float ctime = ftime - 32.0f;
		float xp = -1.3f - 0.003f*ctime*ctime*ctime;
		float alpha = 1.0f - ctime*ctime/8.0f;
		drawQuad(xp, 0.45f, 0.24f, alpha);
		xp = -1.3f + 0.003f*ctime*ctime*ctime;
		drawQuad(xp, 0.3f, 0.13f, alpha);
	}


	
	glDisable(GL_BLEND);
}






void otoneScene(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	parameterMatrix[0] = sqrt(ftime) * 3.0f; // time
	parameterMatrix[3] = sqrtf(ftime * 0.1f); // time
	if (parameterMatrix[3] > 1.0f) parameterMatrix[3] = 1.0f;
	parameterMatrix[1] = 0.0f;
	parameterMatrix[6] = 0.0f;
	// translation
	float rotSpeed = 0.3f;
	float rotAmount = 1.6f;
	float moveSpeed = 0.25f;
	float posX = rotAmount * sin(ftime * rotSpeed);
	float posY = rotAmount * cos(ftime * rotSpeed) - moveSpeed * ftime;
	float deltaX = rotAmount * rotSpeed * cos(ftime * rotSpeed);
	float deltaY = -moveSpeed - rotAmount * rotSpeed * sin(ftime * rotSpeed);
	float len = sqrtf(deltaX*deltaX + deltaY*deltaY);
	if (len > 0.0001f)
	{
		deltaX /= len;
		deltaY /= len;
	}
	else
	{
		deltaX = 1.0f;
		deltaY = 0.0f;
	}
	parameterMatrix[8] = posX;
	parameterMatrix[9] = posY;
	parameterMatrix[12] = deltaX;
	parameterMatrix[13] = deltaY;
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[7]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}





void cloudStuffScene(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	parameterMatrix[0] = ftime; // time

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[2]); // background.shader
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}





void ball8Preview(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	parameterMatrix[0] = ftime; // time
	parameterMatrix[1] = 0.0f; // amount of lightstream
	if (ftime > 6.5f)
	{
		parameterMatrix[1] = (ftime - 6.5f) * 0.25f;
		if (parameterMatrix[1] > 1.1f) parameterMatrix[1] = 1.1f; 
	}
	if (ftime < 8.0f)
	{
		parameterMatrix[2] = -1.0f; // y-position of the ball
	}
	else
	{
		parameterMatrix[2] = -1.0f + sqrtf(4.0f * (ftime - 8.0f));
	}
	if (ftime < 8.0f) // noise add of implicit value
	{
		parameterMatrix[3] = ftime * ftime * 0.1f;
		if (parameterMatrix[3] > 2.5f) parameterMatrix[3] = 2.5f;
	}
	else
	{
		parameterMatrix[3] = 2.5f - 2.0f * (ftime - 8.0f);
		if (parameterMatrix[3] < 0.0f) parameterMatrix[3] = 0.0f;
	}
	parameterMatrix[4] = sqrtf(ftime);
	if (parameterMatrix[4] > 2.0f) parameterMatrix[4] = 2.0f; // amount of backlight

	parameterMatrix[14] = -2.7f; // Roation around z
	parameterMatrix[15] = ftime * 0.2f; // Rotation around y
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[6]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}





void ball8Scene(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	parameterMatrix[0] = ftime; // time
	parameterMatrix[1] = 1.2f; // amount of lightstream
	parameterMatrix[3] = 0.0f; // noise add of implicit value
	parameterMatrix[4] = 2.0f - ftime * 0.075f; // amount of backlight
	if (parameterMatrix[4] < 0.0f) parameterMatrix[4] = 0.0f;

	parameterMatrix[2] = 1.5f; // y position of the ball	
	parameterMatrix[5] = 0.0f;
	parameterMatrix[14] = -2.7f; // Rotation around z
	parameterMatrix[15] = ftime * 0.2f; // Rotation around y
	float dTime = 3.75f;
	if (ftime < dTime)
	{
		parameterMatrix[2] = 4.0f * params.getParam(2, 0.44f); // y position of the ball
		parameterMatrix[5] = params.getParam(3, 0.0f); // z position
		parameterMatrix[14] = 6.24f * params.getParam(4, 0.54f); // rotation around z
		//parameterMatrix[14] = -2.7f; // Roation around z
		parameterMatrix[15] = ftime * 0.2f + 6.24f * params.getParam(5, 0.30f); // Rotation around y
	}
	else if (ftime < 2.0f * dTime)
	{
		parameterMatrix[2] = 4.0f * params.getParam(2, 0.36f); // y position of the ball
		parameterMatrix[5] = params.getParam(3, 0.0f); // z position
		parameterMatrix[14] = 6.24f * params.getParam(4, 0.49f); // rotation around z
		parameterMatrix[15] = ftime * 0.2f + 6.24f * params.getParam(5, 0.84f); // Rotation around y
		//parameterMatrix[2] = 1.5f;
		//parameterMatrix[14] = -3.1f;
		//parameterMatrix[15] = 0.0f;
	}
	else if (ftime < 3.0f * dTime)
	{
		parameterMatrix[5] = params.getParam(3, 0.0f); // z position
		parameterMatrix[14] = 6.24f * params.getParam(4, 0.54f); // rotation around z
		parameterMatrix[15] = ftime * 0.2f + 6.24f * params.getParam(5, 0.5f); // Rotation around y
		parameterMatrix[2] = sqrt(ftime - 2.0f * dTime); // y position of the ball
		//parameterMatrix[14] = -2.9f; // Rotation around z
		//parameterMatrix[15] = 3.1f + ftime * 0.2f; // Rotation around y
		parameterMatrix[1] = 1.05f; // amount of lightstream
	}
	else if (ftime < 4.0f * dTime)
	{
		parameterMatrix[2] = 4.0f * params.getParam(2, 0.25f); // y position of the ball
		parameterMatrix[5] = params.getParam(3, 0.0f); // z position
		parameterMatrix[14] = 6.24f * params.getParam(4, 0.8f) - (ftime-3.0f*dTime)*0.2f; // rotation around z
		parameterMatrix[15] = 6.24f * params.getParam(5, 0.23f); // Rotation around y
		//parameterMatrix[2] = 1.0f;
		//parameterMatrix[14] = -1.5f - (ftime-3.0f*dTime)*0.2f;
		//parameterMatrix[15] = 1.5f;
		parameterMatrix[1] = 0.5f + (ftime - 3.0f*dTime) * 0.25f; // amount of lightstream
	}
	else if (ftime < 5.0f * dTime)
	{
		//parameterMatrix[2] = 4.0f * params.getParam(2, 0.36f); // y position of the ball
		parameterMatrix[5] = params.getParam(3, 0.35f); // z position
		parameterMatrix[14] = 6.24f * params.getParam(4, 0.65f); // rotation around z
		parameterMatrix[15] = sqrtf(ftime-4*dTime) + 6.24f * params.getParam(5, 0.84f); // Rotation around y
		parameterMatrix[2] = 3.0f - (ftime-4*dTime) * 1.2f; // y position of the ball
		//parameterMatrix[14] = -3.4f; // Rotation around z
		//parameterMatrix[15] = 4.6f + sqrtf(ftime-4*dTime) ; // Rotation around y
	}
	else if (ftime < 6.0f * dTime)
	{
		parameterMatrix[2] = 0.5f; // y position of the ball
		parameterMatrix[14] = -2.4f - (ftime-5*dTime)*0.1f; // Rotation around z
		parameterMatrix[15] = 5.6f + (ftime-5*dTime)*0.2f; // Rotation around y
	}
	else if (ftime < 7.0f * dTime)
	{
		parameterMatrix[2] = 1.2f; // y position of the ball
		parameterMatrix[13] = 1.0f; // local density of reflection
		parameterMatrix[14] = 1.5f; // Rotation around z
		parameterMatrix[15] = 2.6f + (ftime-6*dTime)*0.2f; // Rotation around y
	}
	else if (ftime < 8.0f * dTime)
	{
		//parameterMatrix[2] = 4.0f * params.getParam(2, 0.36f); // y position of the ball
		parameterMatrix[5] = params.getParam(3, 0.2f); // z position
		parameterMatrix[14] = 6.24f * params.getParam(4, 0.66f); // rotation around z
		parameterMatrix[15] = sqrtf(ftime-7*dTime) + 6.24f * params.getParam(5, 0.8f); // Rotation around y
		parameterMatrix[2] = sqrt(ftime-7*dTime) * 0.8f; // y position of the ball
		parameterMatrix[1] = 1.3f;
		//parameterMatrix[14] = -3.4f; // Rotation around z
		//parameterMatrix[15] = 4.6f + sqrtf(ftime-4*dTime) ; // Rotation around y
	}
	else if (ftime < 9*dTime)
	{
		parameterMatrix[2] = 2.0f; // y position of the ball	
		parameterMatrix[14] = -2.7f; // Roation around z
		parameterMatrix[15] = (ftime-8*dTime) * 0.2f; // Rotation around y
	}
	else if (ftime < 10*dTime)
	{
		parameterMatrix[2] = 1.5f;
		parameterMatrix[14] = -3.1f;
		parameterMatrix[15] = 0.0f;
	}
	else
	{
		parameterMatrix[2] = -1.0f;
		parameterMatrix[14] = -3.0f;
		parameterMatrix[15] = 0.0f;
		parameterMatrix[1] = 1.5f - (ftime-10*dTime)*0.075f;
	}
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[6]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}



void gras10Scene(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	parameterMatrix[0] = ftime; // time
	parameterMatrix[1] = 1.0f;
#if 1
	if (ftime < 1.0f)
	{
		parameterMatrix[1] = sqrtf(ftime);
	}
	else if (ftime > 6.5f) // if (ftime < 22.0f)
	{
		parameterMatrix[1] = 1.0f - (ftime-6.5f) * 0.25f * fabsf(sin(ftime*14.0f));
	}
#endif
	parameterMatrix[4] = params.getParam(2, 0.36f);
	parameterMatrix[5] = params.getParam(3, 0.39f);
	parameterMatrix[6] = params.getParam(4, 0.43f);
	parameterMatrix[7] = params.getParam(5, 0.01f);
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[4]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}



void gras122Scene(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	parameterMatrix[0] = ftime; // time
	parameterMatrix[1] = 1.0f; // global light
	parameterMatrix[15] = max(0.0f, 0.35f - 0.4f * (sin(ftime)*sin(ftime))); // god lights
#if 1
	if (ftime < 1.0f)
	{
		parameterMatrix[1] = sqrtf(ftime);
	}
	else if (ftime > 6.3f) // if (ftime < 22.0f)
	{
		parameterMatrix[1] = 1.0f - (ftime-6.3f) * 0.25f * fabsf(sin(ftime*14.0f));
	}
#endif
	// shape of the object
	//2:0.36(46) 3:0.39(50) 4:0.43(54) 5:0.01(1) 
	parameterMatrix[4] = params.getParam(2, 0.36f);
	parameterMatrix[5] = params.getParam(3, 0.39f);
	parameterMatrix[6] = params.getParam(4, 0.43f);
	parameterMatrix[7] = params.getParam(5, 0.01f);
	//parameterMatrix[0] = 5.0;
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[5]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}



void gras122AScene(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);

	parameterMatrix[0] = ftime; // time
	parameterMatrix[1] = 1.0f; // global light
	parameterMatrix[15] = 0.5f; // light rays
#if 1
	if (ftime < 1.0f)
	{
		parameterMatrix[1] = sqrtf(ftime);
	}
	else if (ftime > 6.3f) // if (ftime < 22.0f)
	{
		parameterMatrix[1] = 1.0f - (ftime-6.3f) * 0.25f * fabsf(sin(ftime*14.0f));
	}
#endif
	// shape of the object
	//2:0.20(26) 3:0.43(55) 4:0.24(30) 5:0.40(51) 6:0.37(47) 
	parameterMatrix[4] = params.getParam(2, 0.20f);
	parameterMatrix[5] = params.getParam(3, 0.43f);
	parameterMatrix[6] = params.getParam(4, 0.24f);
	parameterMatrix[7] = params.getParam(5, 0.40f);
	parameterMatrix[8] = params.getParam(6, 0.37f);
	//parameterMatrix[0] = 5.0;
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[5]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}




void gras20Scene(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	parameterMatrix[0] = ftime; // time
	parameterMatrix[1] = 1.0f; // totalColor multiplier
	parameterMatrix[4] = 1.0f;  // cameraZ
	parameterMatrix[15] = ftime * 0.695f * 1.05f; // additional lighter?
	while (parameterMatrix[15] > 2.5f*1.05f) parameterMatrix[15] -= 2.5f*1.05f;
#if 1
	if (ftime < 1.0f)
	{
		parameterMatrix[1] = 1.0f;
	}
	else if (ftime > 12.3f) // if (ftime < 22.0f)
	{
		parameterMatrix[1] = 1.0f - (ftime-12.3f) * 0.2f * (sin(ftime*14.0f));
		parameterMatrix[4] = 1.0f - (ftime-12.3f) * (ftime-12.3f) * 0.1f;  // cameraZ
	}
#endif
	parameterMatrix[2] = 4.0f / (ftime + 10.0f); // unknown noise parameter
	parameterMatrix[3] = 1.3f - ftime * 0.15f;   // camera rotation
	// human user stuff 2:0.00(0) 3:0.17(22) 4:0.80(101) 
	//2:0.31(39) 3:0.28(35) 4:0.28(36) 5:0.52(66) 
	parameterMatrix[5] = params.getParam(2, 0.31f);
	parameterMatrix[6] = params.getParam(3, 0.28f);
	parameterMatrix[7] = params.getParam(4, 0.28f);
	parameterMatrix[8] = params.getParam(5, 0.0f);
	parameterMatrix[9] = params.getParam(6, 0.0f);
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[3]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}


void nothingScene(float ftime)
{
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);

	parameterMatrix[0] = ftime; // time	
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[1]);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime - 0.15f;

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
	
#if 1
	if (ftime < 37.7f)
	{
		veryStartScene(ftime);
	}
	else if (ftime < 52.7f)
	{
		otoneScene(ftime - 37.7f);
	}
	else if (ftime < 60.0f)
	{
		gras122AScene(ftime - 52.7f);
	}
	else if (ftime < 67.5f)
	{
		gras10Scene(ftime - 60.0f);
	}
	else if (ftime < 74.8f)
	{
		gras122Scene(ftime - 67.5f);
	}
	else if (ftime < 89.5f)
	{
		gras20Scene(ftime - 75.2f);
	}
	else if (ftime < 101.f)
	{
		ball8Preview(ftime - 89.5f);
	}
	else if (ftime < 151.0f)
	{
		ball8Scene(ftime - 101.f);
	}
	else
	{
		nothingScene(ftime);
	}
#else
	float tt = ftime;
	//float dTime = 3.75f;
	float dTime = 7.3f;
	//float dTime = 14.7f;
	while (tt > dTime) tt -= dTime;
	gras10Scene(tt);
#endif
}

