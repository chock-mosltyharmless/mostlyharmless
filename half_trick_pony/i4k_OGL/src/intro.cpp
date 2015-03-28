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

float frand();
int rand();

#ifndef PI
#define PI 3.1415f
#endif

// -------------------------------------------------------------------
//                          SCRIPT
// -------------------------------------------------------------------
#define NUM_SCENES 14

// Additive, in 4*4096/44100 seconds?
#if 0
unsigned char scriptDurations[NUM_SCENES] =
{
	5, // 0: From black to circles
	2,  // 1: From circles to 8-bit (almost instant)
	10, // 2: 8-Bit stay
	0,  // 3: To fireball (instant?)
	10, // 4: Shatter off
	20, // 5: Fireball to transparent
	 3, // 6: Exploded fireball
	20, // 7: To Shadow man
	20, // 8: To Light mystic
	 5, // 9: Hold Light mystic
	12, // 10: Blorange wool ball
	13, // 11: Fade out
	255, // END!
};
#else
unsigned char scriptDurations[NUM_SCENES] =
{
	14, // 0: From black to circles
	6,  // 1: From circles to 8-bit (almost instant)
	40, // 2: 8-Bit stay
	21, // 3: Shatter of in 8-Bit land
	 0,  // 4: To pre-shatter in 8-Bit land
	11,  // 5: To fireball (instant?)
	51, // 6: Fireball to transparent
	 8, // 7: Exploded fireball
	30, // 8: To Shadow man
	50, // 9: To Light mystic
	15, // 10: Hold Light mystic
	36, // 11: Blorange wool ball
	38, // 12: Fade out
	255, // END!
};
#endif

// NOTE: scene is first...
// End effet|               3:0.57(72) 5:0.30(38) 6:0.24(31) 8:1.00(127) 9:0.43(54) 14:0.03(4) 15:0.05(6) 16:0.49(62) 17:0.66(84) 18:1.00(127) 19:1.00(127) 20:0.02(2) 
// Single circles|          2:0.00(0) 3:0.52(66) 4:1.00(127) 5:0.00(0) 6:0.00(0) 8:0.00(0) 9:0.41(52) 14:0.00(0) 15:0.57(73) 16:0.00(0) 17:0.00(0) 18:0.50(63) 19:0.00(0) 20:0.35(45) 21:0.01(1) 
// Multiple circles|        2:0.00(0) 3:0.48(61) 4:1.00(127) 5:0.00(0) 6:0.00(0) 8:0.00(0) 9:0.31(39) 14:1.00(127) 15:0.52(66) 16:0.00(0) 17:0.00(0) 18:0.48(61) 19:0.00(0) 20:0.34(43) 
// 8-Bit effect|            2:0.07(9) 3:0.54(68) 4:1.00(127) 5:0.06(8) 6:1.00(127) 8:0.00(0) 9:0.32(41) 14:1.00(127) 15:0.52(66) 17:0.00(0) 18:0.00(0) 19:0.00(0) 20:0.31(39) 
// Fireball 1 with shatter| 2:0.09(12) 3:0.63(80) 4:1.00(127) 5:1.00(127) 6:0.00(0) 8:0.01(1) 9:0.30(38) 14:1.00(127) 15:0.61(77) 16:0.00(0) 17:0.00(0) 18:0.00(0) 19:0.00(0) 20:0.49(62) 
// Transparent fireball|    2:0.23(29) 3:0.61(78) 4:1.00(127) 5:1.00(127) 6:0.00(0) 8:0.02(3) 9:0.69(87) 14:0.00(0) 15:0.57(72) 16:0.00(0) 17:1.00(127) 18:0.01(1) 19:0.00(0) 20:0.57(72) 
// Exploded fireball|       2:0.02(2) 3:0.13(16) 4:0.29(37) 5:0.50(63) 6:0.11(14) 8:0.38(48) 9:0.76(97) 15:0.20(26) 16:0.00(0) 17:1.00(127) 18:0.77(98) 19:0.09(11) 20:0.05(6) 
// Light mystic|            2:0.15(19) 3:0.41(52) 4:0.35(45) 5:0.34(43) 6:0.24(31) 8:0.70(89) 9:0.34(43) 14:0.00(0) 15:0.05(6) 16:0.00(0) 17:0.01(1) 18:0.46(59) 19:1.00(127) 20:0.13(17) 
// Blorange wool ball|      2:0.27(34) 3:0.09(12) 4:0.43(54) 5:0.52(66) 6:0.19(24) 8:0.00(0) 9:0.26(33) 14:0.19(24) 15:0.39(50) 16:0.00(0) 17:0.03(4) 18:0.24(30) 19:1.00(127) 20:0.29(37) 
unsigned char script[14][NUM_SCENES] =
{
	// 0: From black to circles
	// 1: From circles to 8-bit (almost instant)
	// 2: 8-Bit stay
	// 3: Shatter of in 8-Bit land
	// 4: To pre-shatter in 8-Bit land
	// 5: To fireball (instant?)
	// 6: Fireball to transparent
	// 7: Exploded Fireball
	// 8: to Shadow man
	// 9: To Light Mystic
	// 10: Hold Light mystic
	// 11: Blorange wool ball
	// 12: Fade out

	// 0    1    2    3    4    5    6    7    8    9   10   11   12
	// Slider 1 (2: base Noise frequency)
	{  9,   9,   9,   9,   9,   9,  12,  29,   2,  64,  19,  19,  34,  34},
	// Slider 2 (3: base Noise modulation)
	{ 68,  68,  68,  68,  68,  68,  68,  78,  16,  60,  52,  52,  12,  12},
	// Slider 3 (4: base Noise amount)
	{127, 127, 127, 127, 127, 127, 127, 127,  37,  64,  45,  45,  54,  54},
	// Slider 4 (5: delta Noise frequecny)
	{  0,   0,   8,   8,   0, 127, 127, 127,  63,  38,  43,  43,  66,  66},
	// Slider 5 (6: delta Noise amount)
	{ 80,  80,  80,  80,  80,   0,   0,   0,  14,  31,  31,  31,  12,   0},
	// Slider 6 (8: delta Noise implicit add amount)
	{  0,   0,   0,   0,   0,   0,   0,   3,  48, 127,  89,  89,   0,   0},
	// Slider 7 (9: ray movement speed)
	{ 44,  44,  24,  32,  32,  32,  39,  87,  97,  54,  43,  35,  30,  30},
	// Knob 1 (14: Mirror shattering amount)
	{  0, 127, 127, 127,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
	// Knob 2 (15: Saturation)
	{ 66,  66,  66,  66,  66,  66,  72,  72,  26,   6,   6,   6,  50,   0},
	// Knob 3 (16: Spike size modulation)
	//{127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127},
	{ 60,  60,  60,  60,  60,   60,  40,   0,   0,   40,  40,  20,  0,   0},
	// Knob 4 (17: Solid to transparent)
	{  0,   0,   0,   0,   0,   0,   0, 127, 127,  84,   3,   3,   4,   4},
	// Knob 5 (18: Delta noise brightness adder)
	{  0,   0,   0,   0,   0,   0,   0,   0,  98, 127,  59,  59,  30,  30},
	// Knob 6 (19: Delta noise movement speed)
	{  0,   0,   0,   0,   0,   0,   0,   0,  35, 127, 127, 127, 127, 127},
	// Knob 7 (20: Brightness)
	//{  0,  40,  60,  80,  64,  64,  72,  72,   3,   3,  14,  14,  37,   0}
	{  0,  30,  45,  50,  40,  40,  50,  50,   2,   2,  8,  8,  23,   0}
};

// -------------------------------------------------------------------
//                          SHADER CODE:
// -------------------------------------------------------------------

const GLchar *fragmentMainBackground="\
uniform sampler3D Texture0;\n\
varying vec3 objectPosition;\n\
varying mat4 parameters;\n\
\n\
vec3 vnoise(vec3 pos, int iterations, float reduction)\n\
{\n\
   pos *= 2.; /*adjust texture size*/\n\
   float intensity = 1.;\n\
   float size = 1.;\n\
   vec3 result = vec3(0.);\n\
\n\
   for (int k = 0; k < iterations; k++)\n\
   {\n\
      vec3 pos2 = floor(pos*size*16.) + smoothstep(0.,1., (pos*size*16.) - floor(pos*size*16.)) - 0.5;\n\
      vec4 inp = texture3D(Texture0, pos2/16.);\n\
      result += inp.xyz * intensity;\n\
      intensity = intensity * reduction;\n\
      size = size * 1.93;\n\
   }\n\
   \n\
   return result;\n\
}\n\
\n\
vec2 rotate(vec2 pos, float angle)\n\
{\n\
	return pos * mat2(cos(angle),-sin(angle),sin(angle),cos(angle));\n\
}\n\
\n\
void main(void)\n\
{  \n\
   float time = parameters[3][2];\n\
   float spike = parameters[3][3];\n\
   float slider1 = parameters[0][0];\n\
   float slider2 = parameters[0][1];\n\
   float slider3 = parameters[0][2];\n\
   float slider4 = parameters[0][3];\n\
   float slider5 = parameters[1][0];\n\
   float slider6 = parameters[1][1];\n\
   float slider7 = parameters[1][2];\n\
   float knob1 = parameters[1][3];\n\
   float knob2 = parameters[2][0];\n\
   float knob3 = parameters[2][1];\n\
   float knob4 = parameters[2][2];\n\
   float knob5 = parameters[2][3];\n\
   float knob6 = parameters[3][0];\n\
   float knob7 = parameters[3][1];\n\
   vec3 rayDir = normalize(objectPosition * vec3(1.0, 0.6, 1.0));\n\
   \n\
   vec3 color = vec3(0.);\n\
   vec3 rayPos = vec3(0., 0., 0. - 8.);\n\
   //vec3 rayDir = normalize(vec3(ppos, 2.));\n\
   vec3 totalColor = vec3(0.);\n\
   float totalDensity = 0.;\n\
   \n\
   rayDir.xz = rotate(rayDir.xz, time);\n\
   rayDir.xy = rotate(rayDir.xy, time*0.7);\n\
   rayDir.xz = rotate(rayDir.xz, time*0.4);\n\
   rayPos.xz = rotate(rayPos.xz, time);\n\
   rayPos.xy = rotate(rayPos.xy, time*0.7);\n\
   rayPos.xz = rotate(rayPos.xz, time*0.4);\n\
   \n\
   for (int i = 0; i < 100 && length(rayPos) < 12. && totalDensity < 0.95; i++) {\n\
      \n\
	  vec3 dval = vnoise((rayPos * 0.05) * slider4 * slider4 + vec3(time*0.02) * knob6, 3, 0.6).rgb;\n\
	  vec3 nval = vnoise(dval*rayPos*0.1*slider1*slider1 + floor(rayDir*3.5*knob1)*0.01*knob1 + dval*2.*slider5 + vec3(time*0.01), 5, slider2).rgb;\n\
	  float implicit = length(rayPos + slider3*nval*5.) - 3. - 2.*knob3*spike;\n\
	  implicit -= slider6 * dval.g * 15.;\n\
      \n\
	  float maxMove = (length(dval))*8. + 0.1/(knob5+0.01);\n\
	  float colAdd = smoothstep(3., 0.1, maxMove)*50.*knob5+1.;\n\
      \n\
      float density = smoothstep(0.1*knob4, -knob4*10., implicit);\n\
      totalDensity += (1. - totalDensity) * density;\n\
      totalDensity += 0.01;\n\
	  totalColor += mix(vec3(0.01, 0.012, 0.014), vec3(0.015, 0.013, 0.01), (nval.r+0.1)*20.*knob2) * colAdd * knob7 * 3.;\n\
      \n\
      rayPos += rayDir * max(0.03, min(maxMove,abs(implicit)) * slider7);\n\
   }\n\
   \n\
   color = mix(color, totalColor, totalDensity) * (1.0 + spike) - 0.2*spike;\n\
   \n\
   gl_FragColor = vec4(color, 1.0);\n\
}";

#if 0
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
   noiseVal.x = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43758.5453);\n\
   noiseVal.y = fract(sin(dot(noisePos.xy, vec2(12.9898, 78.233))) * 43753.5453);\n\
   gl_FragColor = texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.02;\n\
\n\
}";
#else
const GLchar *fragmentOffscreenCopy="\
uniform sampler2D Texture0;\n\
varying vec3 objectPosition;\n\
varying mat4 parameters;\n\
\n\
void main(void) {\n\
	float time = parameters[3][2];\n\
	vec4 col = texture2D(Texture0, 0.5*objectPosition.xy+0.5);\n\
	for (int i = 0; i < 9; i++)\n\
	{\n\
		float n1=fract(sin(time + dot(objectPosition.xy,vec2(-12.9898+float(i),78.233)))*43758.5453);\n\
		float n2=fract(sin(time + dot(objectPosition.xy,vec2(23.34534,23.4324-float(i))))*2038.23482);\n\
		vec4 col2 = texture2D(Texture0, 0.5*objectPosition.xy+0.5 + vec2(n1,n2)*vec2(n1,n2)*0.05);\n\
		float similarity = 0.07 / (length(col - col2) + 0.07);\n\
		//col = mix(col, col2, 0.5 * similarity);\n\
		//if (similarity > .5) {col = mix(col,col2,0.5);}\n\
		if (similarity > 0.) {col = mix(col,col2,.3*similarity);}\n\
		col += 0.04 * vec4(min(n1,n2));\n\
	}\n\
	gl_FragColor = 1.1 * col - vec4(0.1);\n\
}";
#endif

const GLchar *vertexMainObject="\
#version 120\n\
varying vec3 objectPosition;\
varying mat4 parameters;\
\
void main(void)\
{\
   parameters = gl_ModelViewMatrix;\
   objectPosition = vec3(gl_Vertex.x, gl_Vertex.y, 1.0);\
   gl_Position = vec4(gl_Vertex.x, gl_Vertex.y, 0.5, 1.0);\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

#define NUM_GL_NAMES 10
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
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

// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];

static GLuint offscreenTexture;
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

typedef void (*GenFP)(void); // pointer to openGL functions
static GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderPrograms[2];

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

void intro_init( void )
{
	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

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
	
	// init objects:	
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);	
	GLuint fOffscreenCopy = glCreateShader(GL_FRAGMENT_SHADER);
	shaderPrograms[0] = glCreateProgram();
	shaderPrograms[1] = glCreateProgram();
	// compile sources:
	glShaderSource(vMainObject, 1, &vertexMainObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(fMainBackground, 1, &fragmentMainBackground, NULL);
	glCompileShader(fMainBackground);
	glShaderSource(fOffscreenCopy, 1, &fragmentOffscreenCopy, NULL);
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

	// RLY?
	//glEnable(GL_CULL_FACE);
}

void fallingBall(float ftime)
{
	ftime += EFFECT_START_TIME;

	GLUquadric* quad = gluNewQuadric();

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


	parameterMatrix[14] = ftime; // time	
	// BPM => spike calculation
	float BPS = (float)MZK_RATE / (float)MZK_BLOCK_SIZE;
	float jumpsPerSecond = BPS / 8.0f; // Jump on every fourth beat.
	float phase = 0.3f;
	float jumpTime = (ftime * jumpsPerSecond) + phase;
	jumpTime -= floor(jumpTime);
	jumpTime = jumpTime * jumpTime;
	// spike is between 0.0 and 1.0 depending on the position within whatever.
	//float spike = 0.5f * cosf(jumpTime * PI * 1.5f) + 0.5f;
	float spike = 1.0f - jumpTime * 3.f;
	if (spike < 0) spike = 0;
	if (ftime < MZK_BLOCK_SIZE * 60.0f / 44100.0f ||
		ftime > MZK_BLOCK_SIZE * 60.0f * 18.0f / 44100.0f) spike = 0;
	parameterMatrix[15] = spike; // spike
	/* shader parameters */
	//3:0.57(72) 5:0.30(38) 6:0.24(31) 8:1.00(127) 9:0.43(54) 14:0.03(4) 15:0.05(6) 16:0.49(62) 17:0.66(84) 18:1.00(127) 19:1.00(127) 20:0.02(2) 
#ifdef EDIT_PARAMETERS
	parameterMatrix[0] = params.getParam(2, 0.5f); //slider1
	parameterMatrix[1] = params.getParam(3, 0.57f);
	parameterMatrix[2] = params.getParam(4, 0.5f);
	parameterMatrix[3] = params.getParam(5, 0.3f);
	parameterMatrix[4] = params.getParam(6, 0.24f);
	parameterMatrix[5] = params.getParam(8, 1.f);
	parameterMatrix[6] = params.getParam(9, 0.43f);
	parameterMatrix[7] = params.getParam(14, 0.03f); // knob1
	parameterMatrix[8] = params.getParam(15, 0.05f);
	parameterMatrix[9] = params.getParam(16, 0.49f);
	parameterMatrix[10] = params.getParam(17, 0.66f);
	parameterMatrix[11] = params.getParam(18, 1.f);
	parameterMatrix[12] = params.getParam(19, 1.f);
	parameterMatrix[13] = params.getParam(20, 0.02f);
#else
	for (int i = 0; i < 14; i++)
	{
		int timePoint = 0;
		float lastEndTime = 0.0f;
		float nextEndTime = scriptDurations[0] * 4 * 4096.0f / 44100.0f;
		while (nextEndTime < ftime)
		{
			lastEndTime = nextEndTime;
			timePoint++;
			nextEndTime += scriptDurations[timePoint] * 4 * 4096.0f / 44100.0f;
		}
		
		float firstVal = script[i][timePoint] * (1.0f / 127.0f);
		float lastVal = script[i][timePoint + 1] * (1.0f / 127.0f);
		float t = (ftime - lastEndTime) / (nextEndTime - lastEndTime);
		t = 0.5f - 0.5f * (float)cos(t * 3.1415f);
		parameterMatrix[i] = (1.0f - t) * firstVal + t * lastVal;
	}

	parameterMatrix[0] = params.getParam(2, parameterMatrix[0]); //slider1
	parameterMatrix[1] = params.getParam(3, parameterMatrix[1]);
	parameterMatrix[2] = params.getParam(4, parameterMatrix[2]);
	parameterMatrix[3] = params.getParam(5, parameterMatrix[3]);
	parameterMatrix[4] = params.getParam(6, parameterMatrix[4]);
	parameterMatrix[5] = params.getParam(8, parameterMatrix[5]);
	parameterMatrix[6] = params.getParam(9, parameterMatrix[6]);
	parameterMatrix[7] = params.getParam(14, parameterMatrix[7]); // knob1
	parameterMatrix[8] = params.getParam(15, parameterMatrix[8]);
	parameterMatrix[9] = params.getParam(16, parameterMatrix[9]);
	parameterMatrix[10] = params.getParam(17, parameterMatrix[10]);
	parameterMatrix[11] = params.getParam(18, parameterMatrix[11]);
	parameterMatrix[12] = params.getParam(19, parameterMatrix[12]);
	parameterMatrix[13] = params.getParam(20, parameterMatrix[13]);
#endif


	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glUseProgram(shaderPrograms[0]);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, XRES, YRES);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderPrograms[1]);	
	gluSphere(quad, 2.0f, 16, 16);
}

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

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

	fallingBall(ftime);
}

