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

float frand();
int rand();

// -------------------------------------------------------------------
//                          INTRO SCRIPT:
// -------------------------------------------------------------------
#define NUM_SCENES 10
#define NUM_PARAMETERS 12
#define SYNC_MULTIPLIER 128

// I need to come up with a good timerimer here...
static unsigned int sceneStart[NUM_SCENES+1] =
{
	0, 50, // light ray
	100, 200, 300, // cloud flight
	400, 500, // bluebar
	600, // auspuff
	700, 800, // fireplace
	900, // The end.
};
static unsigned char sceneData[NUM_SCENES+2][NUM_PARAMETERS] =
{
	{56, 69, 53, 127, 75, 49, 26, 127, 58, 127, 36, 127},
	//Light ray (beginning)
	{56, 69, 53, 127, 75, 0, 26, 127, 58, 127, 36, 127},
	//Light ray (beginning2)
	{56, 69, 53, 127, 75, 55, 26, 127, 58, 127, 36, 127},
	// Cloud flight start
	{127, 64, 49, 127, 127, 38, 65, 127, 51, 127, 0, 127},
	// Cloud flight middlepart
	{127, 64, 49, 127, 127, 38, 85, 127, 31, 127, 63, 127},
	// Cloud flight turbo
	{127, 64, 49, 127, 127, 78, 87, 111, 0, 127, 127, 127},
	// Bluebar start
	{60, 69, 50, 127, 127, 61, 85, 110, 0, 127, 127, 127},
	// Bluebar end
	{59, 68, 49, 127, 127, 42, 127, 91, 0, 127, 7, 127},
	// Auspuff
	{45, 67, 48, 0, 0, 78, 127, 5, 127, 0, 27, 127},
	// Fireplace 1
	{0, 88, 41, 0, 0, 71, 127, 6, 127, 0, 30, 127},
	// Fireplace 2
	{0, 88, 39, 0, 0, 76, 127, 9, 127, 0, 30, 7},
	{0, 88, 39, 0, 0, 76, 127, 9, 127, 0, 30, 7},
};

// -------------------------------------------------------------------
//                          SHADER CODE:
// -------------------------------------------------------------------

const GLchar *fragmentMainBackground="\
uniform sampler3D Texture0;\n\
varying vec3 objectPosition;\n\
varying mat4 parameters;\n\
\n\
vec3 noise(vec3 pos, int iterations, float reduction)\n\
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
/*\n\
vec3 color(vec3 pos, float sphereSize)\n\
{\n\
   float fTime0_X = parameters[0][0];\n\
   float transform = parameters[1][0];\n\
   vec3 relpos = (pos - vec3(0.0, fTime0_X*0.6, 0.0)) * 0.5;\n\
   \n\
   relpos += noise(relpos*0.03, 3, 0.6) * 1. + fTime0_X * 0.03;\n\
   float brightness, color;\n\
   brightness = noise(relpos*0.2, 5, 0.8).r + dot(pos, vec3(0., 0.5, 0.)) + 0.1 * length(pos);\n\
   \n\
   color = brightness*2. * (1.0 - 0.8*transform);\n\
   return clamp(color * vec3(0.3, .6, 1.), -0.2, 1.5);\n\
}*/\n\
vec3 color(vec3 pos, float sphereSize)\n\
{\n\
  vec3 relpos = pos * 0.5;\n\
  float fTime0_X = parameters[0][0];\n\
   \n\
   relpos *= vec3(1.0,0.05+0.95*parameters[2][1],1.0);\n\
   relpos -= vec3(0.0, fTime0_X*0.3, 0.0);\n\
   relpos += noise(relpos * 0.1, 2, 0.6) * 0.2 + fTime0_X * 0.03;\n\
   float brightness = -1.0, color, whiteness;\n\
      \n\
	  brightness = noise(relpos, 5, 0.8).r * 0.2 + (0.9*dot(pos, vec3(0., 1., 0.)) - 0.75) * (1.-1.1*parameters[1][0]);\n\
   \n\
   color = abs(brightness * 4.0);\n\
   whiteness = brightness * 0.6;\n\
   \n\
   vec3 bumpNormal = +2.5*pos + 1.0;\n\
   \n\
   float hemi = 0.5 + 0.5 * 1./*bumpNormal.y*/;   \n\
   hemi = 0.6 * smoothstep(-0.5, 0.5, hemi) - 0.1;\n\
   //float hemiSpec = clamp(1.0 * whiteness, -0.1, 0.5);\n\
   float hemiSpec = 0.;\n\
\n\
return clamp(hemiSpec + hemi * (whiteness + color * vec3(0.3, 0.6, 1.0) / (length(pos)*(1.-0.8*parameters[1][0])+parameters[1][0])), -0.2, 1.5); \n\
   }\n\
\n\
vec2 rotate(vec2 pos, float angle)\n\
{\n\
	return vec2(cos(angle)*pos.x - sin(angle)*pos.y,\n\
				sin(angle)*pos.x + cos(angle)*pos.y);\n\
}\n\
\n\
void main(void)\n\
{  \n\
   float fTime0_X = parameters[0][0];\n\
   float whitecolor = parameters[1][1];\n\
   vec3 rayDir = normalize(objectPosition * vec3(1.0, 0.6, 1.0));\n\
   vec3 camPos = vec3(0.0, 0.0, -5.7 + 5.7 * parameters[0][1]);\n\
   \n\
   // rotate camera around y axis\n\
   float alpha;\n\
   alpha = parameters[0][3]*9.42;\n\
   camPos.xz = rotate(camPos.xz, alpha);\n\
   rayDir.xz = rotate(rayDir.xz, alpha);\n\
   alpha = parameters[0][2]*9.42;\n\
   camPos.yz = rotate(camPos.yz, alpha);\n\
   rayDir.yz = rotate(rayDir.yz, alpha);\n\
   \n\
   vec3 rayPos = camPos;\n\
   float sceneSize = 16.0;\n\
   vec3 totalColor = vec3(0.,0.,0.);\n\
   float stepSize;\n\
   float totalDensity = 0.0;\n\
   vec3 totalColorAdder = (vec3(0.02, 0.014, 0.009) + whitecolor*vec3(0.,0.004,0.008)) * (parameters[1][2]);\n\
   \n\
   while(length(rayPos)<sceneSize && totalDensity < 0.9)\n\
   {\n\
      // base head\n\
      vec3 tmpPos = rayPos;\n\
      float base1 = abs(tmpPos.y);\n\
	  float base2 = abs(length(rayPos.xz) - 1.5 + 1.5 * parameters[1][3]) * (0.5 + parameters[2][3]);\n\
	  float socket = length(rayPos + vec3(0., 9., 0.)) - 8.2 + 20.*parameters[2][2];  \n\
	  float base = (max(base1 - 1.1 - parameters[2][0]*20., base2 - 1.5*parameters[1][3] + 0.8));\n\
	  float implicitVal = min(socket * (0.2 + parameters[3][0]), base);\n\
\n\
vec3 noiseAdder = noise(rayPos * 0.003  - vec3(0.0, fTime0_X*0.006, 0.0), 3, 0.) * 0.1 * parameters[2][1];\n\
      float noiseVal = noise(noiseAdder + rayPos*0.04*vec3(1.0,0.05+0.95*parameters[2][1],1.0) - vec3(0.0, fTime0_X*0.03, 0.0), 7, 0.6).r * 0.6;\n\
      implicitVal -= noiseVal;\n\
      \n\
	  totalColor += totalColorAdder;\n\
      totalDensity += 0.003;\n\
      if (implicitVal < 0.05)\n\
      {\n\
	     float localDensity = min(1.0, 0.05 - implicitVal);\n\
		 localDensity = (1.-totalDensity) * localDensity;\n\
         totalColor += color(rayPos, 0.) * localDensity;\n\
         totalDensity += localDensity ;\n\
      }\n\
      \n\
      stepSize = (implicitVal) * 0.3;\n\
      stepSize = max(0.005, stepSize);\n\
      rayPos += rayDir * stepSize;\n\
   }\n\
   \n\
   float grad = normalize(rayPos).y;\n\
   totalColor += (1.-totalDensity) * (grad * vec3(0.0,0.0,0.1) + (1.-grad)*vec3(0.0,0.1,0.2));\n\
   \n\
   gl_FragColor = vec4(totalColor-vec3(0.2), 1.0);\n\
\n\
}";

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
   gl_FragColor = texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.0007*noiseVal.xy) + noiseVal.x*0.02;\n\
\n\
}";

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
GLint viewport[4];
char err[4097];

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

void fallingBall(float ftime)
{

}

void intro_do( long itime )
{
	GLUquadric* quad = gluNewQuadric();

	float ftime = 0.001f*(float)itime;

    // render
    glEnable( GL_CULL_FACE );

	glMatrixMode(GL_MODELVIEW);
	parameterMatrix[0] = ftime; // time	
	/* shader parameters */
	int scene = 7;
	while ((unsigned int)itime > sceneStart[scene+1] * SYNC_MULTIPLIER && scene < NUM_SCENES - 1)
	{
		scene++;
	}
	for (int k = 0; k < NUM_PARAMETERS; k++)
	{
		float mu = (float)((unsigned int)itime - sceneStart[scene]*SYNC_MULTIPLIER) / (float)((sceneStart[scene+1]-sceneStart[scene])*SYNC_MULTIPLIER);
		if (mu > 1.) mu = 1.;
		if (mu < 0.) mu = 0.;
		float y0 = (float)(sceneData[scene][k] * (1./128.));
		float y1 = (float)(sceneData[scene+1][k] * (1./128.));
		float y2 = (float)(sceneData[scene+2][k] * (1./128.));
		float y3 = (float)(sceneData[scene+3][k] * (1./128.));
		//a0 = y3 - y2 - y0 + y1;
		//a1 = y0 - y1 - a0;
		//a2 = y2 - y0;
		//a3 = y1;
		float a0 = -0.5f*y0 + 1.5f*y1 - 1.5f*y2 + 0.5f*y3;
		float a1 = y0 - 2.5f*y1 + 2.0f*y2 - 0.5f * y3;
		float a2 = -0.5f*y0 + 0.5f * y2;
		float a3 = y1;
		parameterMatrix[k+1] = (((mu*a0)+a1)*mu+a2)*mu+a3;
	}
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glUseProgram(shaderPrograms[0]);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	gluSphere(quad, 2.0f, 8, 8);

	// copy to front
	glViewport(0, 0, viewport[2], viewport[3]);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderPrograms[1]);	
	gluSphere(quad, 2.0f, 8, 8);

	fallingBall(ftime);
}

