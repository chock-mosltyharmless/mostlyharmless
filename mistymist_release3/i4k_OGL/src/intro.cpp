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

// #define SHADER_DEBUG

extern double outwave[][2];

float frand();
int rand();

// -------------------------------------------------------------------
//                          INTRO SCRIPT:
// -------------------------------------------------------------------
#define NUM_SCENES 13
#define NUM_PARAMETERS 9
#define SYNC_MULTIPLIER 361
#define SYNC_ADJUSTER 80

// I need to come up with a good timerimer here...
static unsigned char sceneLength[NUM_SCENES] =
{
	///*
	20, // light ray
	4, // startup.2
	16, // Baralong
	20, // Sidescroll
	20, // Final
	40, // Fasto (8)
	20, // flythrough.a
	20, // flythrough
	40, // Tunnelb
	40, // Cloud (5)
	20, // Plasma
	//*/
	/*1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,//*/
	35, // Bar (3)
	60 // Bar.b (3)
};
static unsigned char sceneData[NUM_SCENES][NUM_PARAMETERS] =
{
	// Startup (8)
	{66, 69, 50, 91, 57, 54, 127, 72, 127},
	// Startup.2 (8)
	{66, 69, 50, 31, 97, 64, 127, 22, 127},
	// Baralong
	{92, 28, 93, 127, 99, 127, 127, 127, 127},
	// Sidescroll
	{95, 105, 27, 106, 92, 100, 12, 20, 127}, 
	// Final
	{40, 89, 10, 110, 103, 94, 9, 0, 0}, 
	// Fasto (8)
	{87, 68, 49, 75, 0, 92, 127, 127, 127},
    // flythrough.a
	{126, 101, 88, 124, 102, 94, 127, 0, 127},
	// flythrough
	{126, 101, 88, 124, 102, 94, 127, 0, 127},
	// Tunnelb
	{127, 69, 45, 92, 69, 90, 127, 0, 127},
	// Cloud (5)
	{69, 25, 90, 57, 75, 114, 37, 7, 127},
	// Plasma
	{115, 64, 42, 125, 65, 88, 127, 0, 0},
	// Bar (3)
	{50, 40, 127, 127, 110, 112, 127, 83, 127},
	// Safety
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
};
static char sceneDelta[NUM_SCENES][NUM_PARAMETERS] =
{
	// Startup (8)
	{0, 0, 0, -60, 40, 10, 0, -50, 0},
	// Startup.2 (8)
	{26, -41, 43, 96, 2, 63, 0, 105, 0},
	// Baralong
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	// Sidescroll
	{0, -5, 0, 0, 0, 0, 0, 0, 0},
	// Final
	{30, 0, 30, 60, 0, 0, 0, 0, 0},
	// Fasto (8)
	{0, 0, 0, 0, 0, 0, 0, -120, 0},
	// flythrough.a
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
    // flythrough
	//{1, 0, 0, -26, -23, 3, 0, 0, 0},
	{1, -32, -43, -32, -33, -4, 0, 0, 0},
	// Tunnelb
	//{-12, -5, -3, 33, -4, -2, 0, 0, -127},
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	// Cloud (5)
	{10, 4, -5, 0, 0, 0, 0, 0, 0},
	// Plasma
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	// Bar (3)
	{40, -20, -85, 0, 0, 0, 0, 0, 0},
	// Safety
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
};

// -------------------------------------------------------------------
//                          SHADER CODE:
// -------------------------------------------------------------------

const GLchar *fragmentMainBackground="\
uniform sampler3D t;\
varying vec3 o;\
varying mat4 p;\
vec3 e(vec3 s, int i)\
{\
float n=1.;\
s*=2.;\
vec3 r=vec3(0.);\
for(;i>0;i--)\
{\
r+=texture3D(t,s).xyz*n;\
n*=0.7;\
s*=1.93;\
}\
return r;\
}\
void main(void)\
{\
vec3 d=normalize(o*vec3(1.,.6,1.));\
vec3 r=vec3(0.,0.,8.*p[0][1]-8.);\
float s=sin(p[0][3]*9.42);\
float c=cos(p[0][3]*9.42);\
r.xz=r.xz*mat2(c,-s,s,c);\
d.xz=d.xz*mat2(c,-s,s,c);\
s=sin(p[0][2]*9.42);\
c=cos(p[0][2]*9.42);\
r.yz=r.yz*mat2(c,-s,s,c);\
d.yz=d.yz*mat2(c,-s,s,c);\
vec3 l=vec3(0.,0.,0.);\
float m=0.;\
vec3 w=vec3(.016,.012,.009)*p[1][0];\
while(length(r)<15.&&m<0.95)\
{\
float i=min(length(r+vec3(0.,9.,0.))-8.2+20.*p[2][1],max(abs(r.y)-1.1-p[1][3]*20.,abs(length(r.xz)-2.+2.*p[1][2])*(.5+p[2][0])-2.*p[1][2]+1.2));\
vec3 n=r;\
n.y-=p[0][0]*.66;\
vec3 c=e(n*0.03,2)*.04*p[1][1];\
n.y=n.y*(.05+p[1][1])-.5*p[0][0];\
float v=e(c+n*.08,6).r;\
i-=.5*v;\
l+=w;\
m+=.003;\
float t=(1.-m)*clamp(1.-exp(i),0.,1.);\
float f=smoothstep(-2.5,-1.5,-length(r)+v*6.*(p[2][1]+1.));\
l+=(mix(vec3(.7,.2,.0),vec3(.1,.45,.85),f)*clamp(abs(f-.5),-0.5,1.)+clamp(r.y+v.x-2.*p[1][3]-1.,.0,100.))*t;\
m+=t;\
r+=d*max(.02,(i)*.5);\
}\
l+=(1.-m)*mix(vec3(.0,.1,.2),vec3(.0,.0,.1),normalize(r).y);\
gl_FragColor=vec4(l-.3,1.);\
}";

const GLchar *fragmentOffscreenCopy="\
uniform sampler2D t;\
varying vec3 o;\
varying mat4 p;\
void main(void)\
{\
vec2 n=vec2(fract(sin(dot(o.xy+p[0][0],vec2(12.9898,78.233)))*43758.5453));\
gl_FragColor=texture2D(t,.5*o.xy+.5+.001*n)+n.x*.03;\
}";

const GLchar *vertexMainObject="\
varying vec3 o;\
varying mat4 p;\
void main(void)\
{\
p=gl_ModelViewMatrix;\
o=vec3(gl_Vertex.xy,.99);\
gl_Position=vec4(o,1.);\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

#ifdef SHADER_DEBUG
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
#ifdef SHADER_DEBUG
#define glGetShaderiv ((PFNGLGETSHADERIVPROC)glFP[8])
#define glGetShaderInfoLog ((PFNGLGETSHADERINFOLOGPROC)glFP[9])
#endif

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

typedef void (*GenFP)(void); // pointer to openGL functions
static GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderPrograms[2];
GLint viewport[4];

#ifdef SHADER_DEBUG
char err[4097];
#endif

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
}

void intro_do( long itime )
{
	//GLUquadric* quad = gluNewQuadric();

	itime += SYNC_ADJUSTER;
	float ftime = 0.001f*(float)itime;

    // render
    glEnable( GL_CULL_FACE );

	glMatrixMode(GL_MODELVIEW);
	parameterMatrix[0] = ftime; // time	
	/* shader parameters */
	int scene = 0;
	unsigned int sceneTime = (unsigned int)itime;
	while (sceneTime > (unsigned int)sceneLength[scene] * SYNC_MULTIPLIER)
	{
		sceneTime -= (unsigned int)sceneLength[scene]*SYNC_MULTIPLIER;
		scene++;		
	}
	float t = (float)(sceneTime) /
		      (float)(sceneLength[scene] * SYNC_MULTIPLIER);
	t = 0.5f - 0.5f * (float)cos(t*3.1415f);
	for (int k = 0; k < NUM_PARAMETERS; k++)
	{
		float a = (float)(sceneData[scene][k] * (1./128.)) +
			      t * (float)(sceneDelta[scene][k] * (1./128.));
		parameterMatrix[k+1] = a;
	}

	// get music information
	double loudness = 1.0;
	int musicPos = (((itime-SYNC_ADJUSTER)*441)/10);
	for (int k = 0; k < 4096; k++)
	{
		loudness += outwave[musicPos][k]*outwave[musicPos][k];
	}
#ifdef USEDSOUND
	parameterMatrix[4] += (float)log(loudness) * (1.f/32.f) - .75f; // This makes it silent?
	//parameterMatrix[4] = (float)log(loudness) * 0.1f - 0.5f;
#endif

	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glUseProgram(shaderPrograms[0]);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	//gluSphere(quad, 2.0f, 8, 8);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// copy to front
	glViewport(0, 0, viewport[2], viewport[3]);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderPrograms[1]);	
	//gluSphere(quad, 2.0f, 8, 8);
	glRectf(-1.0, -1.0, 1.0, 1.0);
}

