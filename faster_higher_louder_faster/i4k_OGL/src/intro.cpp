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

//#define SHADER_DEBUG

inline int ftoi_fast(float f)
{
    return _mm_cvtt_ss2si(_mm_load_ss(&f));     // SSE1 instructions for float->int
}

// -------------------------------------------------------------------
//                          SCRIPT
// -------------------------------------------------------------------
#define NUM_SCENES 14

// Additive, in 4*4096/44100 seconds?
#pragma data_seg(".script_durations")
static unsigned char scriptDurations[NUM_SCENES] =
{
	44,//14, // 0: From black to circles
	16,  // 1: From circles to 8-bit (almost instant)
	10,//40, // 2: 8-Bit stay
	10, // 3: Shatter of in 8-Bit land
	 0,  // 4: To pre-shatter in 8-Bit land
	12,  // 5: To fireball (instant?)
	51, // 6: Fireball to transparent
	 8, // 7: Exploded fireball
	30, // 8: To Shadow man
	45, // 9: To Light mystic
	 5, // 10: Hold Light mystic
	41, // 11: Blorange wool ball
	48, // 12: Fade out
	255, // END!
};

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
static unsigned char script[14][NUM_SCENES] =
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
	{  0,   0,   6,   6,   0, 127, 127, 127,  63,  38,  43,  43,  66,  66},
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
static const GLchar *fragmentMainBackground=
 "uniform sampler3D t;"
 "varying vec3 o;"
 "varying mat4 p;"
 "vec3 v(vec3 z,int v,float m)"
 "{"
   "float y=1.;"
   "vec3 x=vec3(0.);"
   "for(;v>0;v--)"
     "x+=texture3D(t,z*2.).xyz*y,y*=m,z*=1.93;"
   "return x;"
 "}"
 "vec2 v(vec2 z,float v)"
 "{"
   "return z*mat2(cos(v),-sin(v),sin(v),cos(v));"
 "}"
 "void main()"
 "{"
   "vec3 z=normalize(o*vec3(1.,.6,1.)),y=vec3(0.,0.,-8.),f=vec3(0.);"
   "float x=0.;"
   "z.xz=v(z.xz,p[3][2]);"
   "z.xy=v(z.xy,p[3][2]*.7);"
   "z.xz=v(z.xz,p[3][2]*.4);"
   "y.xz=v(y.xz,p[3][2]);"
   "y.xy=v(y.xy,p[3][2]*.7);"
   "y.xz=v(y.xz,p[3][2]*.4);"
   "for(int m=0;m<100&&length(y)<12.&&x<.95;m++)"
     "{"
       "vec3 s=v(y*.05*p[0][3]*p[0][3]+vec3(p[3][2]*.02)*p[3][0],3,.6),c=v(s*y*.1*p[0][0]*p[0][0]+floor(z*3.5*p[1][3])*.01*p[1][3]+s*2.*p[1][0]+vec3(p[3][2]*.01),5,p[0][1]);"
       "float i=length(y+p[0][2]*c*5.)-3.-2.*p[2][1]*p[3][3]-p[1][1]*s.y*15.,r=length(s)*8.+.1/(p[2][3]+.01);"
       "x+=(1.-x)*smoothstep(.1*p[2][2],-p[2][2]*10.,i)+.01;"
       "f+=mix(vec3(.01,.012,.014),vec3(.015,.013,.01),(c.x+.1)*20.*p[2][0])*(smoothstep(3.,.1,r)*50.*p[2][3]+1.)*p[3][1]*3.;"
       "y+=z*max(.03,min(r,abs(i))*p[1][2]);"
     "}"
   "gl_FragColor=vec4((x*.8+.2)*f*(1.+p[3][3])-.2*p[3][3],1.);"
 "}";

#pragma data_seg(".fragment_offscreen_copy")
static const GLchar *fragmentOffscreenCopy=
 "uniform sampler2D t;"
 "varying vec3 o;"
 "varying mat4 p;"
 "void main()"
 "{"
   "float v=smoothstep(6.,4.,p[3][2]);"
   "vec3 m=vec3(1.3);"
   "float s=o.x,a=o.y*.6,l=s+a,y=s-a;"
   "m=mix(m,vec3(.1),smoothstep(.005,0.,min(min(length(vec2(s,a))-.2,max(max(s+.1,-.2-s),max(a-.3,-a))),max(-l,min(min(max(l-.2-v*.4,max(y+.155,-.28-y)),max(l-.65,max(y+.01,-.135-y))),min(max(l-.25-v*.4,max(y-.135,.01-y)),max(l-.18-v*.4,max(y-.28,.155-y))))))));"
   "m=mix(m,vec3(1.3),smoothstep(.005,0.,max(max(abs(s)-.1,abs(a)-.1),abs(y)-.14)));"
   "m=mix(m,vec3(1.3,.1,.1),smoothstep(.005,0.,max(max(abs(s)-.08,abs(a)-.08),abs(y)-.11)));"
   "vec4 f=mix(texture2D(t,.5*o.xy+.5),vec4(m,1.),.7*v);"
   "for(int i=0;i<9;i++)"
     "{"
       "float x=fract(sin(p[3][2]+dot(o.xy,vec2(-12.9898+float(i),78.233)))*43758.5),r=fract(sin(p[3][2]+dot(o.xy,vec2(23.3453,23.4324-float(i))))*2038.23);"
       "vec4 e=texture2D(t,.5*o.xy+.5+vec2(x,r)*vec2(x,r)*.03);"
       "f=mix(f,e,.3*(.07/(length(f-e)+.07)))+.04*vec4(min(x,r));"
     "}"
   "gl_FragColor=1.1*f-vec4(.1);"
 "}";

#pragma data_seg(".vertex_main_object")
static const GLchar *vertexMainObject=
 "varying vec3 o;"
 "varying mat4 p;"
 "void main()"
 "{"
   "p=gl_ModelViewMatrix,o=vec3(gl_Vertex.xy,.9),gl_Position=vec4(o,1.);"
 "}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

#define MAX_GL_NAME_LENGTH 16
#pragma data_seg(".gl_names")
#ifdef SHADER_DEBUG
#define NUM_GL_NAMES 10
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
};
#else
#define NUM_GL_NAMES 8
const static char glnames[NUM_GL_NAMES][MAX_GL_NAME_LENGTH]={
	{"glCreateShader"}, {"glCreateProgram"}, {"glShaderSource"}, {"glCompileShader"}, 
	{"glAttachShader"}, {"glLinkProgram"}, {"glUseProgram"},
	{"glTexImage3D"}
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

#ifdef SHADER_DEBUG
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
	//glShaderSource(fMainBackground, 1, &fotzeShader, NULL);
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
#if 0
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
#endif
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
	float ftime = 0.001f * (float)itime;

	/* Set everything to beginning */
	for (int i = 0; i < 16; i++)
	{
		parameterMatrix[i] = 0.0f;
	}

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	


	parameterMatrix[14] = ftime; // time	
#if 1
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
#else
	int jumpIndex = (itime + 223) / 8 * MZK_RATE / MZK_BLOCK_SIZE / 1000;
	int iJumpTime = (itime + 223) - jumpIndex * MZK_BLOCK_SIZE * 1000 / MZK_RATE * 8;
	float jumpTime = (float)iJumpTime / 8.0f * (float)MZK_RATE / (float)MZK_BLOCK_SIZE / 1000.0f;
	jumpTime = jumpTime * jumpTime;
	// spike is between 0.0 and 1.0 depending on the position within whatever.
	//float spike = 0.5f * cosf(jumpTime * PI * 1.5f) + 0.5f;
	float spike = 1.0f - jumpTime * 3.f;
	if (spike < 0) spike = 0;
	if (ftime < MZK_BLOCK_SIZE * 60.0f / 44100.0f ||
		ftime > MZK_BLOCK_SIZE * 60.0f * 18.0f / 44100.0f) spike = 0;
#endif
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

