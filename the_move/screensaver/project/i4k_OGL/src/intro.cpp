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
#define OTONE_SHADER 7
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
   gl_Position = gl_ProjectionMatrix * vec4(gl_Vertex.x, gl_Vertex.y, gl_Vertex.z, 1.0) ;\
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
//        ALL the stuff that I need to represent scenes
// -------------------------------------------------------------------
struct Line
{
	const char *texName;
	float start[3];  // Z is depth...
	float end[3];
	float width;
	float color[4];
	bool multipartLine;
	float hangThrough;
	float xTranspose;
};

struct Scene
{
	int outputNumLines; // What can come after this scene
	int numLines;
	const Line *lines;
};

// Input 0: Scene with 3 Lines at the top.
// I need some sort of perspective correction, no? And also aspect ratio...
const float CABLE_WIDTH = 0.01f;
const float MAST_WIDTH = 0.6f;
const float MAST_DISTANCE = 10.0f;

const Line scene_3_3_a[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.2f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.13f},
	{"mast1.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_3_3_b[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.22f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.1f, 0.13f},
	{"mast2.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_3_3_c[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, 10.1f}, 0.012f, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, 10.1f}, 0.012f, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.09f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, 10.1f}, 0.012f, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.21f, 0.13f},
	{"mast3.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, 0.6f, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_3_5_up[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.2f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.53f},
	{"mast4.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
	{"thin_line_small.tga", {-1.25f, 0.0f, -1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.3f},
	{"thin_line_small.tga", {-1.28f, 0.0f, -1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.21f, 0.38f},
};

const Line scene_3_5_leftUp[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, 0.53f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
	{"thin_line_small.tga", {-2.75f, 0.0f, -1.2f}, {-1.25f, 1.36f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.3f},
	{"thin_line_small.tga", {-2.78f, 0.0f, -1.2f}, {-1.25f, 1.36f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.38f},
};

const Line scene_3_3_crossAtMast[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.2f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.13f},
	{"thin_line_small.tga", {-11.26f, 1.65f, 0.0f}, {-1.25f, 1.65f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.24f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.65f, 0.0f}, {9.26f, 1.65f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, -0.01f},
	{"thin_line_small.tga", {-11.26f, 1.5f, 0.0f}, {-1.25f, 1.5f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.5f, 0.0f}, {9.26f, 1.5f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, -0.01f},
	{"mast2.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_3_3_crossAtMastLots[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-11.26f, 1.65f, 0.0f}, {-1.25f, 1.65f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.65f, 0.0f}, {9.26f, 1.65f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, -0.01f},
	{"thin_line_small.tga", {-11.26f, 1.5f, 0.0f}, {-1.25f, 1.5f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.11f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.5f, 0.0f}, {9.26f, 1.5f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, -0.01f},
	{"thin_line_small.tga", {-11.26f, 1.6f, 0.3f}, {11.25f, 1.6f, 0.3f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, -0.01f},
	{"thin_line_small.tga", {-11.26f, 1.5f, 0.3f}, {11.25f, 1.5f, 0.3f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, -0.01f},
	{"thin_line_small.tga", {-11.26f, 1.5f, 0.5f}, {11.25f, 1.5f, 0.5f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, -0.01f},
	{"mast1.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};


const Line scene_3_3_crossAbove[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.2f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.13f},
	{"thin_line_small.tga", {-11.26f, 1.9f, 0.7f*MAST_DISTANCE}, {11.25f, 1.9f, 0.7f*MAST_DISTANCE}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.32f, -0.01f},
	{"thin_line_small.tga", {-11.26f, 1.9f, 0.7f*MAST_DISTANCE+0.2f}, {11.25f, 1.9f, 0.7f*MAST_DISTANCE+0.2f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.31f, -0.01f},
	{"thin_line_small.tga", {-11.26f, 1.9f, 0.7f*MAST_DISTANCE+0.4f}, {11.25f, 1.9f, 0.7f*MAST_DISTANCE+0.4f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.31f, -0.01f},
	{"mast1.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_5_5_a[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.2f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.25f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.53f},
	{"mast4.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_5_5_b[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.11f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.11f, 0.53f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_5_3_a[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.11f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.13f},
	{"thin_line_small.tga", {-11.26f, 1.35f, 0.0f}, {-1.25f, 1.35f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, -0.01f},
	{"thin_line_small.tga", {-11.26f, 1.4f, 0.0f}, {-1.25f, 1.4f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, -0.01f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_5_3_b[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.21f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.65f, 0.0f}, {9.26f, 1.65f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.5f, 0.0f}, {9.26f, 1.5f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, -0.01f},
	{"mast4.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_5_5_crossAtMast[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.2f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.13f},
	{"thin_line_small.tga", {-11.26f, 1.65f, 0.0f}, {-1.25f, 1.65f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.65f, 0.0f}, {9.26f, 1.65f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.21f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.53f},
	{"thin_line_small.tga", {-11.26f, 1.5f, 0.0f}, {-1.25f, 1.5f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.5f, 0.0f}, {9.26f, 1.5f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, -0.01f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_5_5_crossUpMast[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.2f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.4f, 0.0f}, {9.26f, 1.4f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.21f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.53f},
	{"thin_line_small.tga", {-1.25f, 1.35f, 0.0f}, {9.26f, 1.35f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, -0.01f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
	{"thin_line_small.tga", {-1.25f, 0.0f, -1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.3f},
	{"thin_line_small.tga", {-1.28f, 0.0f, -1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.21f, 0.38f},
};

const Line scene_5_3_down[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.11f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 0.0f, 1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, 0.3f},
	{"thin_line_small.tga", {-1.28f, 0.0f, 1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.38f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_5_7_up[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.11f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.35f, -0.1f}, {-1.25f, 1.35f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.25f, -0.1f}, {-1.25f, 1.25f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, 0.53f},
	{"mast4.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
	{"thin_line_small.tga", {-1.25f, 0.0f, -1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.3f},
	{"thin_line_small.tga", {-1.28f, 0.0f, -1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.38f},
};

const Line scene_5_7_leftUp[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.35f, -0.1f}, {-1.25f, 1.35f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.25f, -0.1f}, {-1.25f, 1.25f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.53f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
	{"thin_line_small.tga", {-2.75f, 0.0f, -1.2f}, {-1.25f, 1.36f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.3f},
	{"thin_line_small.tga", {-2.78f, 0.0f, -1.2f}, {-1.25f, 1.36f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, 0.38f},
};




const Line scene_7_7_a[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.35f, -0.1f}, {-1.25f, 1.35f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.19f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.25f, -0.1f}, {-1.25f, 1.25f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.53f},
	{"mast4.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_7_7_b[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.35f, -0.1f}, {-1.25f, 1.35f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.25f, -0.1f}, {-1.25f, 1.25f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.53f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_7_5_a[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.53f},
	{"thin_line_small.tga", {-11.26f, 1.25f, 0.0f}, {-1.25f, 1.25f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, -0.01f},
	{"thin_line_small.tga", {-11.26f, 1.3f, 0.0f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, -0.01f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_7_7_crossAtMast[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.13f},
	{"thin_line_small.tga", {-11.26f, 1.65f, 0.0f}, {-1.25f, 1.65f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.17f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.65f, 0.0f}, {9.26f, 1.65f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.14f, 0.53f},
	{"thin_line_small.tga", {-11.26f, 1.5f, 0.0f}, {-1.25f, 1.5f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.5f, 0.0f}, {9.26f, 1.5f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.35f, -0.1f}, {-1.25f, 1.35f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.13f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.25f, -0.1f}, {-1.25f, 1.25f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.13f},
	{"mast4.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
};

const Line scene_7_7_crossUpMast[] =
{
	{"thin_line_small.tga", {-1.25f, 1.8f, -0.1f}, {-1.25f, 1.8f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.7f, -0.1f}, {-1.25f, 1.7f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.21f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.6f, -0.1f}, {-1.25f, 1.6f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.4f, 0.0f}, {9.26f, 1.4f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.11f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.27f, -0.1f}, {-1.25f, 1.27f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.6f},
	{"thin_line_small.tga", {-1.25f, 1.22f, -0.1f}, {-1.25f, 1.22f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.12f, 0.53f},
	{"thin_line_small.tga", {-1.25f, 1.35f, 0.0f}, {9.26f, 1.35f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, -0.01f},
	{"thin_line_small.tga", {-1.25f, 1.35f, -0.1f}, {-1.25f, 1.35f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.16f, 0.13f},
	{"thin_line_small.tga", {-1.25f, 1.25f, -0.1f}, {-1.25f, 1.25f, MAST_DISTANCE + 0.1f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.13f},
	{"mast5.tga", {-1.25f, 2.0f, 0.0f}, {-1.25f, -0.5f, 0.0f}, MAST_WIDTH, {1.0f, 1.0f, 1.0f, 1.0f}, false},
	{"thin_line_small.tga", {-1.25f, 0.0f, -1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.15f, 0.3f},
	{"thin_line_small.tga", {-1.28f, 0.0f, -1.7f}, {-1.25f, 1.3f, 0.0f}, CABLE_WIDTH, {0.1f, 0.1f, 0.1f, 1.0f}, true, 0.18f, 0.38f},
};




const int numScenesInNode[] = {0, 0, 0, 8, 0, 10, 0, 5};

const Scene sceneGraph[][20] = 
{
	{},
	{},
	{},
	{
		{3, sizeof(scene_3_3_a) / sizeof(Line), scene_3_3_a},
		{3, sizeof(scene_3_3_b) / sizeof(Line), scene_3_3_b},
		{3, sizeof(scene_3_3_c) / sizeof(Line), scene_3_3_c},
		{3, sizeof(scene_3_3_crossAtMast) / sizeof(Line), scene_3_3_crossAtMast},
		{3, sizeof(scene_3_3_crossAtMastLots) / sizeof(Line), scene_3_3_crossAtMastLots},
		{3, sizeof(scene_3_3_crossAbove) / sizeof(Line), scene_3_3_crossAbove},		
		{5, sizeof(scene_3_5_up) / sizeof(Line), scene_3_5_up},
		{5, sizeof(scene_3_5_leftUp) / sizeof(Line), scene_3_5_leftUp},
	},
	{},
	{
		{5, sizeof(scene_5_5_a) / sizeof(Line), scene_5_5_a},
		{5, sizeof(scene_5_5_b) / sizeof(Line), scene_5_5_b},
		{5, sizeof(scene_5_5_b) / sizeof(Line), scene_5_5_b}, // DOUBLE FOR LONGER
		{5, sizeof(scene_5_5_crossAtMast) / sizeof(Line), scene_5_5_crossAtMast},
		{5, sizeof(scene_5_5_crossAtMast) / sizeof(Line), scene_5_5_crossAtMast}, // Double for Longer
		{5, sizeof(scene_5_5_crossUpMast) / sizeof(Line), scene_5_5_crossUpMast},
		{3, sizeof(scene_5_3_a) / sizeof(Line), scene_5_3_down},
		{3, sizeof(scene_5_3_a) / sizeof(Line), scene_5_3_a},
		{3, sizeof(scene_5_3_b) / sizeof(Line), scene_5_3_b},
		{7, sizeof(scene_5_7_up) / sizeof(Line), scene_5_7_up},
		{7, sizeof(scene_5_7_leftUp) / sizeof(Line), scene_5_7_leftUp},
	},
	{},
	{
		{7, sizeof(scene_7_7_a) / sizeof(Line), scene_7_7_a},
		{7, sizeof(scene_7_7_b) / sizeof(Line), scene_7_7_b},
		{7, sizeof(scene_7_7_crossAtMast) / sizeof(Line), scene_7_7_crossAtMast},
		{7, sizeof(scene_7_7_crossUpMast) / sizeof(Line), scene_7_7_crossUpMast},
		{5, sizeof(scene_7_5_a) / sizeof(Line), scene_7_5_a},
	}
};

const int SCREENSAVER_NUM_SCENES = 1000;
const Scene *sceneList[SCREENSAVER_NUM_SCENES];

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

	// Create the path for the screensaver:
	int lastNumLines = 5;
	for (int i = 0; i < SCREENSAVER_NUM_SCENES; i++)
	{
		int numScenes = numScenesInNode[lastNumLines];
		int sceneID = (rand() >> 8) % numScenes;
		sceneList[i] = &(sceneGraph[lastNumLines][sceneID]);
		lastNumLines = sceneList[i]->outputNumLines;
	}

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

	// Set the texture locations in the relevant shaders:
	// Set texture locations
	glUseProgram(shaderPrograms[OTONE_SHADER]);
	int my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[OTONE_SHADER], "Texture0");
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(my_sampler_uniform_location, 0);
	my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[OTONE_SHADER], "Texture1");
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(my_sampler_uniform_location, 1);
	glActiveTexture(GL_TEXTURE0);
}


void desktopScene(float ftime)
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
		if (i == 0) // ALARM!
		{
			icon[i].drawAlarming(ftime);
		}
		else
		{
			icon[i].draw(ftime);
		}
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

// assumes that BLEND_mode is on standard.
// Program must be standard
// Ignores ASPECT RATIO... I have to think about it.
void drawLine(float start[3], float end[3],
	          float width, const char *texName,
			  const float color[4])
{
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH+1];
	if (textureManager.getTextureID(texName, &texID, errorString) < 0)
	{
		MessageBox(hWnd, errorString, "Texture load error", MB_OK);
		exit(-1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);

	float startX = start[0];
	float startY = start[1];
	float startZ = start[2];
	float endX = end[0];
	float endY = end[1];
	float endZ = end[2];

	// I have to set the color differently...
	glColor4f(color[0], color[1], color[2], color[3]);

	float lx = endX - startX;
	float ly = endY - startY;
	float invLen = 1.0f / sqrtf(lx * lx + ly * ly);
	lx *= invLen;
	ly *= invLen;
	float nx = -ly;
	float ny = lx;

	// I do not want to do that lx magic...
	lx = 0.0f;
	ly = 0.0f;

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(startX + (-lx + nx)*width, startY + (-ly + ny)*width, -startZ);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(endX + (lx + nx)*width, endY + (ly + ny)*width, -endZ);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(endX + (lx - nx)*width, endY + (ly - ny)*width, -endZ);
	glTexCoord2f(1.0, 1.0f);
	glVertex3f(startX + (-lx - nx)*width, startY + (-ly - ny)*width, -startZ);
	glEnd();
}

// The same as drawline, but uses multiple segments, for perspective correction
void drawMultiLine(float start[3], float end[3],
	               float width, const char *texName,
			       const float color[4], float hangThrough)
{
	const int numSegments = 20;

#if 1
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH+1];
	if (textureManager.getTextureID(texName, &texID, errorString) < 0)
	{
		MessageBox(hWnd, errorString, "Texture load error", MB_OK);
		exit(-1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);
#else
	glBindTexture(GL_TEXTURE_2D, 0);
#endif

	float dPos[3];
	for (int i = 0; i < 3; i++)
	{
		dPos[i] = (end[i] - start[i]) / (float)numSegments;
		end[i] = start[i] + dPos[i];
	}

	float hangers[numSegments+1];
	for (int i = 0; i < numSegments+1; i++)
	{
		// Stupid hanging:
		float hanger = (float)(i - numSegments/2) / (float)(numSegments/2);
		hanger *= hanger;
		hanger = (hanger - 1.0f) * hangThrough;
		hangers[i] = hanger;
	}

	for (int i = 0; i < numSegments; i++)
	{
		float startX = start[0];
		float startY = start[1] + hangers[i];
		float startZ = start[2];
		float endX = end[0];
		float endY = end[1] + hangers[i+1];
		float endZ = end[2];

		float lx = dPos[0];
		float ly = dPos[1];
		float invLen = 1.0f / sqrtf(lx * lx + ly * ly);
		lx *= invLen;
		ly *= invLen;
		float nx = -ly;
		float ny = lx;

		glColor4f(color[0], color[1], color[2], color[3]);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, i/(float)numSegments);
		glVertex3f(startX + nx*width, startY + ny*width, -startZ);
		glTexCoord2f(0.0f, (i+1)/(float)numSegments);
		glVertex3f(endX + nx*width, endY + ny*width, -endZ);
		glTexCoord2f(1.0f, (i+1)/(float)numSegments);
		glVertex3f(endX - nx*width, endY - ny*width, -endZ);
		glTexCoord2f(1.0, i/(float)numSegments);
		glVertex3f(startX - nx*width, startY - ny*width, -startZ);
		glEnd();

		for (int j = 0; j < 3; j++)
		{
			start[j] = end[j];
			end[j] += dPos[j];
		}
	}
}


void rotateX(float dest[3], const float source[3], float alpha)
{
	float ca = (float)cos(alpha);
	float sa = (float)sin(alpha);
	dest[0] = source[0];
	dest[1] = ca * source[1] - sa * source[2];
	dest[2] = sa * source[1] + ca * source[2];
}

void rotateY(float dest[3], const float source[3], float alpha)
{
	float ca = (float)cos(alpha);
	float sa = (float)sin(alpha);
	dest[0] = ca * source[0] - sa * source[2];
	dest[1] = source[1];
	dest[2] = sa * source[0] + ca * source[2];
}

void screensaverScene(float ftime)
{
	char errorString[MAX_ERROR_LENGTH+1];
	GLuint offscreenTexID;
	GLUquadric* quad = gluNewQuadric();

	// Draw the background image
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
    //desktopScene(ftime);

	// Draw the background
	glDisable(GL_BLEND);
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);
	textureManager.getTextureID("wolken1.tga", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	textureManager.drawQuad(-4.0f - 3.0f * cos(ftime*0.007f), -1.0f, 4.0f - 3.0f * cos(ftime*0.007f), 1.0f, 1.0f);

	// Draw a simple line
#if 0
	float color[4] = {0.8f, 0.7f, 0.6f, 1.0f};
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);
	drawLine(0.2f, 0.2f, 0.8f, 0.8f, 0.05f, 0.15f, "line_16x1.tga", color);
	glDisable(GL_BLEND);
#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);

	// Set perspective correction matrix
	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();
	gluPerspective(40, (float)XRES / (FLOAT)YRES, 0.1f, 1000.0f);

	// Calculate where we are in the world
	float distance = ftime * 5.0f;
	int sceneIndex = (int)(distance / MAST_DISTANCE);
	distance -= sceneIndex * MAST_DISTANCE;

	// Draw all the lines in the current scene
	int sceneID[3];
	sceneID[0] = sceneIndex % SCREENSAVER_NUM_SCENES;
	sceneID[1] = (sceneIndex+1) % SCREENSAVER_NUM_SCENES;
	sceneID[2] = (sceneIndex+2) % SCREENSAVER_NUM_SCENES;
	float tmpV[3];

	// Get the direction after rotation
	float up[3] = {0.0f, 1.0f, 0.0f};
	rotateY(tmpV, up, -0.3f);
	rotateX(up, tmpV, 0.5f);

	for (int drawings = 2; drawings >= 0; drawings--)
	{
		const Scene *scene = sceneList[sceneID[drawings]];

		for (int i = 0; i < scene->numLines; i++)
		{
			const Line *line = &(scene->lines[i]);
			float transStart[3];
			float transEnd[3];

			// Translate (and copy...)
			for (int i = 0; i < 3; i++)
			{
				transStart[i] = line->start[i];
				transEnd[i] = line->end[i];
			}
			transStart[2] -= distance - drawings * MAST_DISTANCE;
			transEnd[2] -= distance - drawings * MAST_DISTANCE;

			// Move the lines
			rotateY(tmpV, transStart, -0.3f);
			rotateX(transStart, tmpV, 0.5f);
			rotateY(tmpV, transEnd, -0.3f);
			rotateX(transEnd, tmpV, 0.5f);

			// transpose after rotate
			transStart[0] += line->xTranspose * up[1];
			transStart[1] += line->xTranspose * up[0];
			transEnd[0] += line->xTranspose * up[1];
			transEnd[1] += line->xTranspose * up[0];

			// draw
			if (line->multipartLine)
			{
				drawMultiLine(transStart, transEnd, line->width, line->texName, line->color, line->hangThrough);
			}
			else
			{
				drawLine(transStart, transEnd, line->width, line->texName, line->color);
			}
		}
	}

	// Reset the GL settings
	glDisable(GL_BLEND);
	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	//parameterMatrix[0] = sqrt(ftime) * 3.0f; // time
	parameterMatrix[0] = ftime; // time
	//parameterMatrix[3] = sqrtf(ftime * 0.1f); // time
	//if (parameterMatrix[3] > 1.0f) parameterMatrix[3] = 1.0f;
	parameterMatrix[3] = 1.0f;
	parameterMatrix[1] = 0.0f;
	parameterMatrix[6] = 0.0f;
	// translation
	float rotSpeed = 0.02f;//0.3f;
	float rotAmount = 0.5f;//1.6f;
	float moveSpeed = 0.02f;//0.25f;
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

#if 1
	// copy the video to texture for later rendering
	GLuint noiseTexID;
	glViewport(0, 0, realXRes, realYRes);
	textureManager.getTextureID("renderTarget", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture

	// draw offscreen painters effect
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[OTONE_SHADER]);
	// Set texture0 (noise texture)
	textureManager.getTextureID("noise2D", &noiseTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, noiseTexID);
	// Set texture1 (the original scene)
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glActiveTexture(GL_TEXTURE0);

	// Draw it
	textureManager.drawQuad(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f);

	// copy to front
	glViewport(0, 0, realXRes, realYRes);
	textureManager.getTextureID("renderTarget", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	textureManager.drawQuad(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f);
#endif
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
	
	//desktopScene(ftime);
	screensaverScene(ftime);
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