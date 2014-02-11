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
#include <stdlib.h>

#include "config.h"
#include "intro.h"
#include "Parameter.h"
#include "GLNames.h"
#include "TextureManager.h"
#include "FlowIcon.h"

float frand();

extern int realXRes;
extern int realYRes;

/* Number of names of the used shaders */
#define NUM_SHADERS 10
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
	"shaders/simpleTex.shader",
	"shaders/fragmentOffscreenCopy.shader"
};
#define SIMPLE_TEX_SHADER 8
/* Location where the loaded shader is stored */
#define MAX_SHADER_LENGTH 200000
GLchar fragmentMainBackground[MAX_SHADER_LENGTH];

// Constant shader code that is used for all the effects.
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

TextureManager textureManager;

const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
	 "glDeleteProgram", "glDeleteShader",
	 "glActiveTexture", "glGetUniformLocation", "glUniform1i", "glUniform1f",
	 "glMultiTexCoord2f"
};

// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];
static int noiseTmp[4];

typedef void (*GenFP)(void); // pointer to openGL functions
GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderPrograms[NUM_SHADERS];
//static GLuint shaderCopyProgram;
#define shaderCopyProgram (shaderPrograms[9])

// mouse stuff
bool isSubMenu = false;
int subMenuIndex;
float subMenuAlpha = 0.0f;
float subMenuSize = 0.0f;

// The Icons globally here
#define NUM_ICONS 12
FlowIcon icon[NUM_ICONS];
char iconFileName[NUM_ICONS][1024] = 
{
	"alarm_icon.tga",
	"chair_icon.tga",
	"fernseher_icon.tga",
	"fridge_icon.tga",
	"garbage_icon.tga",
	"garderobe_icon.tga",
	"komode_icon.tga",
	"lampe_icon.tga",
	"music_icon.tga",
	"plant_icon.tga",
	"sofa_icon.tga",
	"teppich_icon.tga"
};

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
	glShaderSource(vMainObject, 1, &vertexMainObject, NULL);
	glCompileShader(vMainObject);
	// Check programs
	glGetShaderiv(vMainObject, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(vMainObject, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "vMainObject shader error", MB_OK);
		return;
	}

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
}

void intro_init( void )
{
	char errorString[MAX_ERROR_LENGTH + 1];

	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// Create and initialize everything needed for texture Management
	if (textureManager.init(errorString))
	{
		MessageBox(hWnd, errorString, "Texture Manager Load", MB_OK);
		return;
	}

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop..
	compileShaders();

	// RLY?
	//glEnable(GL_CULL_FACE);

	// Create the icons
	int index = 0;
	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			icon[index].init(iconFileName[index], x + 1, y + 1);
			index++;
		}
	}
}


void veryStartScene(float ftime)
{
	static float lastTime = 0.0f;
	float deltaTime = ftime - lastTime;
	lastTime = ftime;
	GLuint texID;

	char errorString[MAX_ERROR_LENGTH+1];

	glDisable(GL_BLEND);

	if (isSubMenu)
	{
		subMenuSize += deltaTime * 5.0f * (1.1f - subMenuSize);
		subMenuAlpha += deltaTime * 5.0f * (1.1f - subMenuAlpha);
		if (subMenuSize > 1.0f) subMenuSize = 1.0f;
		if (subMenuAlpha > 1.0f) subMenuAlpha = 1.0f;
	}
	else
	{
		float expTime = exp(-deltaTime*3.0f);
		subMenuAlpha *= expTime;
	}

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

#if 0
	GLuint noiseTexID;
	GLuint offscreenTexID;
	GLUquadric* quad = gluNewQuadric();

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
	glUseProgram(shaderPrograms[0]);
	//glBindTexture(GL_TEXTURE_2D, noiseTexture);
	textureManager.getTextureID("noise2D", &noiseTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, noiseTexID);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	textureManager.getTextureID("renderTarget", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
#else
	if (textureManager.getTextureID("sun-flower.tga", &texID, errorString))
	{
		MessageBox(hWnd, errorString, "texture not found", MB_OK);
		exit(1);
	}
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texID);
	textureManager.drawQuad(-1.2f, -1.3f, 1.6f, 1.0f, 1.0f);
#endif

	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

	// Draw the icons
	for (int i = 0; i < NUM_ICONS; i++)
	{
		icon[i].draw(ftime);
	}

	// Draw the menu if present
	if (subMenuAlpha > 0.01f)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
		if (textureManager.getTextureID("blue.tga", &texID, errorString))
		{
			MessageBox(hWnd, errorString, "texture not found", MB_OK);
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		float xp = icon[subMenuIndex].getGLX();
		float yp = icon[subMenuIndex].getGLY();
		textureManager.drawQuad(xp, yp,
			xp+0.9f*subMenuSize, yp-subMenuSize*0.6f*ASPECT_RATIO, 0.9f * subMenuAlpha);

		icon[subMenuIndex].draw(ftime);
	}
	
	glDisable(GL_BLEND);
}



void otoneScene(float ftime)
{
	char errorString[MAX_ERROR_LENGTH+1];
	GLuint noiseTexID;
	GLuint offscreenTexID;
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
	textureManager.getTextureID("noise2D", &noiseTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, noiseTexID);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	textureManager.getTextureID("renderTarget", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}




void nothingScene(float ftime)
{
	char errorString[MAX_ERROR_LENGTH+1];
	GLuint offscreenTexID;
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
	//glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	textureManager.getTextureID("renderTarget", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}

void intro_do( long itime )
{
	static int oldTimes[5] = {0, 0, 0, 0, 0};
	//static float ftime = -0.15f;
	static float ftime = -0.2f;
	ftime += 0.001f*(float)(itime-oldTimes[0]) / 5.0f;
	for (int i = 0; i < 4; i++)
	{
		oldTimes[i] = oldTimes[i+1];
	}
	oldTimes[4] = itime;	

    // render
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_BLEND);
    glDisable( GL_CULL_FACE );
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
	
	veryStartScene(ftime);
}

void intro_cursor(float xpos, float ypos)
{
	for (int i = 0; i < NUM_ICONS; i++)
	{
		icon[i].setMousePosition(xpos, ypos);
	}
}

void intro_click(float xpos, float ypos)
{
	if (!isSubMenu)
	{
		for (int i = 0; i < NUM_ICONS; i++)
		{
			icon[i].setMousePosition(xpos, ypos);
			if (icon[i].clickMouse())
			{
				isSubMenu = true;
				subMenuAlpha = 0.0f;
				subMenuSize = 0.0f;
				subMenuIndex = i;
			}
		}
	}
	else
	{
		isSubMenu = false;
	}
}