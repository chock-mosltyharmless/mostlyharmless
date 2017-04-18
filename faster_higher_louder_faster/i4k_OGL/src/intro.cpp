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

#define SHADER_DEBUG

inline int ftoi_fast(float f)
{
    return _mm_cvtt_ss2si(_mm_load_ss(&f));     // SSE1 instructions for float->int
}

extern double accumulated_drum_volume;
extern bool has_ended;

// -------------------------------------------------------------------
//                          SHADER CODE:
// -------------------------------------------------------------------

#pragma data_seg(".fragment_main_background")
static const GLchar *fragmentMainBackground=
 "uniform sampler3D t;"
 "varying vec3 o;"
 "varying mat4 p;"
 "void main()"
 "{"
    "vec2 rot_pos = o.xy * vec2(1.6, 0.9);"
    "vec2 rot_pos2 = rot_pos * mat2(p[0][1],-p[0][0],p[0][0],p[0][1]);"
    "vec2 rot_pos3 = rot_pos * mat2(p[0][1],p[0][0],-p[0][0],p[0][1]);"
    "rot_pos3 = rot_pos;"
    "float is_center = smoothstep(p[0][2]+ 0.04, p[0][2], abs(rot_pos2.x)) * 2.0;"
    "float noisy = 0.0;"
    // I have to fade-in topmost stuff somehow...
    "float new_time = p[3][3] - 0.5;"
    "if (p[3][3] < 1.0) {new_time = p[3][3] * p[3][3] * 0.5;}"
    "float zoom_out = fract(pow(0.25 * new_time, 1.5));"
    "const float zoom_step = 0.65;"
    "float zoom = 9.0 * pow(1.0 / zoom_step, zoom_out);"
    "for (int overtones = 1; overtones < 16; overtones++) {"
        "float amount = sin((float(overtones) - zoom_out) / 16.0 * 3.1);"
        "noisy += texture3D(t, vec3(zoom * rot_pos3, (float(overtones) - zoom_out) * 0.05)).r * amount;"
        "zoom *= zoom_step;"
    "}"
    //"gl_FragColor = vec4(vec3(1.0, 1.2, 1.5) * (is_center + noisy), 1.0);"
    "vec3 color = mix(vec3(1.0, 1.0, 1.0), vec3(0.47, 0.25, 0.15), smoothstep(2.0, 1.0, is_center + noisy));"
    "color = mix(color, vec3(0.22, 0.20, 0.05), smoothstep(0.8, 0.0, is_center + noisy));"
    "color = mix(color, vec3(0., 0., 0.0), smoothstep(0.6, -0.6, is_center + noisy));"
    "float brightness = 1.0 - p[3][3] * 0.005;"
    "gl_FragColor = vec4(color * brightness, 1.0);"
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
#define NOISE_TEXTURE_SIZE 32 // try smaller?
static GLuint noiseTexture;
#ifdef FLOAT_TEXTURE
static float noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];
#else
static unsigned char noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];
#endif
static int noiseTmp[4];

typedef void (*GenFP)(void); // pointer to openGL functions
static GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderProgram;

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
	shaderProgram = glCreateProgram();
	// compile sources:
	glShaderSource(vMainObject, 1, &vertexMainObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(fMainBackground, 1, &fragmentMainBackground, NULL);
	//glShaderSource(fMainBackground, 1, &fotzeShader, NULL);
	glCompileShader(fMainBackground);

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
#endif

	// link shaders:
	glAttachShader(shaderProgram, vMainObject);
	glAttachShader(shaderProgram, fMainBackground);
	glLinkProgram(shaderProgram);

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


    parameterMatrix[0] = (float)sin(accumulated_drum_volume*0.125);
    parameterMatrix[1] = (float)cos(accumulated_drum_volume*0.125);
    parameterMatrix[2] = 0.07f;
	parameterMatrix[15] = ftime; // time	
    if (has_ended) {
        parameterMatrix[2] = 2.0f;
        parameterMatrix[15] = ftime * 0.05f;
    }
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glUseProgram(shaderProgram);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	glRectf(-1., -1., 1., 1.);
}

