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
#include <emmintrin.h>

#include "config.h"
#include "intro.h"
#include "mzk.h"

float frand();
int rand();

#ifndef PI
#define PI 3.1415f
#endif

#define DEBUG_SHADER

inline int ftoi_fast(float f)
{
    return _mm_cvtt_ss2si(_mm_load_ss(&f));     // SSE1 instructions for float->int
}

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
	45, // 9: To Light mystic
	 5, // 10: Hold Light mystic
	41, // 11: Blorange wool ball
	48, // 12: Fade out
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
#pragma data_seg(".script")
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
	{ 44,  44,  24,  32,  32,  32,  39,  87,  97,  54,  43,  35,  35,  35},
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

#pragma data_seg(".fragment_main_background")
const GLchar *fragmentMainBackground="\
uniform sampler3D t;\n\
varying vec3 o;\n\
varying mat4 p;\n\
\n\
vec3 v(vec3 s,int i,float r)\
{\
float n=1.;\
vec3 u=vec3(0.);\
for (;i>0;i--)\
{\
u += texture3D(t, s*2.).xyz*n;\
n*=r;\
s*=1.93;\
}\
return u;\
}\
\
vec2 q(vec2 p,float i)\
{\
return p*mat2(cos(i),-sin(i),sin(i),cos(i));\
}\
\
void main(void)\n\
{  \n\
   vec3 rayDir = normalize(o * vec3(1.0, 0.6, 1.0));\n\
   \n\
   vec3 rayPos = vec3(0., 0., 0. - 8.);\n\
   //vec3 rayDir = normalize(vec3(ppos, 2.));\n\
   vec3 totalColor = vec3(0.);\n\
   float totalDensity = 0.;\n\
   \n\
   rayDir.xz = q(rayDir.xz, p[3][2]);\n\
   rayDir.xy = q(rayDir.xy, p[3][2]*0.7);\n\
   rayDir.xz = q(rayDir.xz, p[3][2]*0.4);\n\
   rayPos.xz = q(rayPos.xz, p[3][2]);\n\
   rayPos.xy = q(rayPos.xy, p[3][2]*0.7);\n\
   rayPos.xz = q(rayPos.xz, p[3][2]*0.4);\n\
   \n\
   for (int i = 0; i < 100 && length(rayPos) < 12. && totalDensity < 0.95; i++) {\n\
      \n\
	  vec3 dval = v((rayPos * 0.05) * p[0][3] * p[0][3] + vec3(p[3][2]*0.02) * p[3][0], 3, 0.6).rgb;\n\
	  vec3 nval = v(dval*rayPos*0.1*p[0][0]*p[0][0] + floor(rayDir*3.5*p[1][3])*0.01*p[1][3] + dval*2.*p[1][0] + vec3(p[3][2]*0.01), 5, p[0][1]).rgb;\n\
	  float implicit = length(rayPos + p[0][2]*nval*5.) - 3. - 2.*p[2][1]*p[3][3];\n\
	  implicit -= p[1][1] * dval.g * 15.;\n\
      \n\
	  float maxMove = (length(dval))*8. + 0.1/(p[2][3]+0.01);\n\
	  float colAdd = smoothstep(3., 0.1, maxMove)*50.*p[2][3]+1.;\n\
      \n\
      float density = smoothstep(0.1*p[2][2], -p[2][2]*10., implicit);\n\
      totalDensity += (1. - totalDensity) * density;\n\
      totalDensity += 0.01;\n\
	  totalColor += mix(vec3(0.01, 0.012, 0.014), vec3(0.015, 0.013, 0.01), (nval.r+0.1)*20.*p[2][0]) * colAdd * p[3][1] * 3.;\n\
      \n\
      rayPos += rayDir * max(0.03, min(maxMove,abs(implicit)) * p[1][2]);\n\
   }\n\
   \n\
   gl_FragColor = vec4((totalDensity*.8+.2) * totalColor * (1.0 + p[3][3]) - 0.2*p[3][3], 1.0);\n\
}";

#pragma data_seg(".fragment_offscreen_copy")
const GLchar *fragmentOffscreenCopy="\
uniform sampler2D t;\n\
varying vec3 o;\n\
varying mat4 p;\n\
\n\
void main(void) {\n\
	float time = p[3][2];\n\
	vec4 col = texture2D(t, 0.5*o.xy+0.5);\n\
	for (int i = 0; i < 9; i++)\n\
	{\n\
		float n1=fract(sin(time + dot(o.xy,vec2(-12.9898+float(i),78.233)))*43758.5453);\n\
		float n2=fract(sin(time + dot(o.xy,vec2(23.34534,23.4324-float(i))))*2038.23482);\n\
		vec4 col2 = texture2D(t, 0.5*o.xy+0.5 + vec2(n1,n2)*vec2(n1,n2)*0.03);\n\
		float similarity = 0.07 / (length(col - col2) + 0.07);\n\
		//col = mix(col, col2, 0.5 * similarity);\n\
		//if (similarity > .5) {col = mix(col,col2,0.5);}\n\
		if (similarity > 0.) {col = mix(col,col2,.3*similarity);}\n\
		col += 0.04 * vec4(min(n1,n2));\n\
	}\n\
	gl_FragColor = 1.1 * col - vec4(0.1);\n\
}";

#pragma data_seg(".vertex_main_object")
const GLchar *vertexMainObject="\
#version 120\n\
varying vec3 o;\
varying mat4 p;\
\
void main(void)\
{\
   p = gl_ModelViewMatrix;\
   o = vec3(gl_Vertex.x, gl_Vertex.y, 1.0);\
   gl_Position = vec4(gl_Vertex.x, gl_Vertex.y, 0.5, 1.0);\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

#pragma data_seg(".gl_names")
#ifdef DEBUG_SHADER
#define NUM_GL_NAMES 10
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
};
#else
#define NUM_GL_NAMES 8
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D"
};
#endif

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

#ifdef DEBUG_SHADER
static char err[4097];
#endif
#pragma code_seg(".intro_init")
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

#ifdef SHADER_DEBUG
	// Check programs
	int tmp, tmp2;
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
#endif

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

static GLint viewport[4];

#pragma code_seg(".intro_do")
void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

	/* Set everything to beginning */
	for (int i = 0; i < 16; i++)
	{
		parameterMatrix[i] = 0.0f;
	}

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	


	parameterMatrix[14] = ftime; // time	
	// BPM => spike calculation
	float BPS = (float)MZK_RATE / (float)MZK_BLOCK_SIZE;
	float jumpsPerSecond = BPS / 8.0f; // Jump on every fourth beat.
	float phase = 0.3f;
	float jumpTime = (ftime * jumpsPerSecond) + phase;
	//jumpTime -= (int)(jumpTime);
	jumpTime -= ftoi_fast(jumpTime);
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

	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glUseProgram(shaderPrograms[0]);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	glRectf(-1., -1., 1., 1.);

	// copy to front
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderPrograms[1]);	
	glRectf(-1., -1., 1., 1.);
}

