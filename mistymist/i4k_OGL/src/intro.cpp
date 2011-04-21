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
vec3 color(vec3 pos, vec3 noiseData)\n\
{\n\
  float fTime0_X = parameters[0][0];\n\
  vec3 innercolor = vec3(0.4, 0.1, 0.0);\n\
  vec3 outercolor = vec3(0.1, 0.35, 0.65);\n\
  float height = (pos.y + noiseData.x);\n\
  float colorpart = smoothstep(-0.5, 0.5, -length(pos) + noiseData.x*12.*(parameters[2][1]+1.) + 2.);\n\
  float border = smoothstep(-0.8, 0.8, abs(colorpart - 0.5));\n\
  return mix(innercolor, outercolor, colorpart) * border + clamp(height - 2.*parameters[1][3] - 1.,0.0,100.0);\n\
}\n\
\n\
vec2 rotate(vec2 pos, float angle)\n\
{\n\
	return pos * mat2(cos(angle),-sin(angle),sin(angle),cos(angle));\n\
}\n\
\n\
void main(void)\n\
{  \n\
   float fTime0_X = parameters[0][0];\n\
   vec3 rayDir = normalize(objectPosition * vec3(1.0, 0.6, 1.0));\n\
   vec3 camPos = vec3(0.0, 0.0, -8. + 8. * parameters[0][1]);\n\
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
   vec3 totalColorAdder = (vec3(0.016, 0.012, 0.009)) * (parameters[1][0]);\n\
   \n\
   while(length(rayPos)<sceneSize && totalDensity < 0.95)\n\
   {\n\
      // base head\n\
      vec3 tmpPos = rayPos;\n\
      float base1 = abs(tmpPos.y);\n\
	  float base2 = abs(length(rayPos.xz) - 2.0 + 2.0 * parameters[1][2]) * (0.5 + parameters[2][0]);\n\
	  float socket = length(rayPos + vec3(0., 9., 0.)) - 8.2 + 20.*parameters[2][1];  \n\
	  float base = (max(base1 - 1.1 - parameters[1][3]*20., base2 - 2.0*parameters[1][2] + 1.2));\n\
	  float implicitVal = min(socket, base);\n\
	  //float implicitVal = base;\n\
\n\
      vec3 noiseAdder = noise(rayPos * 0.03  - vec3(0.0, fTime0_X*0.02, 0.0), 2, 0.8) * 0.1 * parameters[1][1];\n\
      float noiseVal = noise(noiseAdder*0.3 + rayPos*0.04*vec3(1.0,0.05+0.95*parameters[1][1],1.0) - vec3(0.0, fTime0_X*0.03, 0.0), 7, 0.7).r * 0.6;\n\
      implicitVal -= noiseVal;\n\
      \n\
	  totalColor += totalColorAdder;\n\
      totalDensity += 0.005;\n\
      if (implicitVal < 0.0)\n\
      {\n\
	     float localDensity = 1. - exp(implicitVal);\n\
		 //float localDensity = 0.1;\n\
		 localDensity = (1.-totalDensity) * localDensity;\n\
         totalColor += color(rayPos, noiseVal) * localDensity;\n\
         totalDensity += localDensity ;\n\
      }\n\
      \n\
	  stepSize = (implicitVal) * 0.5;\n\
      stepSize = max(0.03, stepSize);\n\
      rayPos += rayDir * stepSize;\n\
   }\n\
   \n\
   float grad = normalize(rayPos).y;\n\
   totalColor += (1.-totalDensity) * (grad * vec3(0.0,0.0,0.1) + (1.-grad)*vec3(0.0,0.1,0.2));\n\
   \n\
   gl_FragColor = vec4(sqrt(smoothstep(0.3, 1.4, totalColor)), 1.0);\n\
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
   gl_FragColor = texture2D(Texture0, 0.5*objectPosition.xy + 0.5 + 0.001*noiseVal.xy) + noiseVal.x*0.02;\n\
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


	parameterMatrix[0] = ftime; // time	
	/* shader parameters */
	/* 15:1.00(127) 16:0.69(88) 17:1.00(127) 18:0.50(63) 19:1.00(127) */
	//parameterMatrix[1] = params.getParam(2, 1.0f);
	//parameterMatrix[2] = params.getParam(3, 0.52f);
	//parameterMatrix[3] = params.getParam(4, 0.38f);
	//parameterMatrix[4] = params.getParam(5, 1.0f);
	//parameterMatrix[5] = params.getParam(6, 1.0f);
	//2:0.00(0) 3:0.60(76) 4:0.69(87) 5:0.42(53) 6:0.00(0) 8:0.00(0) 
	//2:0.69(88) 3:0.87(110) 4:0.41(52) 5:0.28(35) 6:0.06(8) 8:0.46(58) 14:0.53(67) 15:1.00(127) 16:0.22(28) 17:1.00(127) 18:0.11(14) 
	//2:1.00(127) 3:0.51(65) 4:0.33(42) 5:0.00(0) 6:0.40(51) 8:0.40(51) 14:0.69(88) 15:1.00(127) 16:0.31(39) 17:1.00(127) 18:0.00(0) 
	//2:0.61(77) 3:0.46(59) 4:0.28(36) 5:0.28(35) 6:0.06(8) 8:0.46(58) 14:0.74(94) 15:1.00(127) 16:0.00(0) 17:1.00(127) 18:1.00(127) 
	//2:0.60(76) 3:0.46(58) 4:0.28(35) 8:0.99(126) 14:0.69(88) 15:0.00(0) 16:0.74(94) 17:0.00(0) 18:0.00(0) 19:1.00(127)  THIS IS SOME STATIC SOMETHING.
	//2:0.60(76) 3:0.83(106) 4:0.88(112) 5:0.00(0) 6:0.36(46) 8:0.64(81) 14:0.93(118) 15:0.17(22) 16:0.54(69) 17:0.75(95) 18:0.07(9) 19:0.60(76)  This is a flyby-in-light
	//2:0.28(35) 3:0.72(92) 4:1.00(127) 5:0.00(0) 6:0.35(44) 8:0.80(102) 14:0.70(89) 15:0.06(8) 16:0.81(103) 17:0.72(92) 18:0.31(39) FIRE UP
	//2:0.09(11) 3:0.54(69) 4:0.24(31) 5:0.00(0) 6:0.34(43) 8:0.49(62) 14:1.00(127) 15:1.00(127) 16:0.72(91) 17:1.00(127) 18:0.62(79) 19:0.00(0) // standard bar not fast
    //2:0.00(0) 3:0.18(23) 4:0.69(87) 6:0.39(49) 8:0.50(64) 14:0.79(100) 15:1.00(127) 16:0.47(60) 17:0.82(104) 18:0.50(63) //Auspuff
	//2:0.15(19) 3:0.49(62) 4:0.35(45) 6:0.39(49) 8:0.50(64) 14:0.77(98) 15:1.00(127) 16:0.24(31) 17:1.00(127) 18:0.51(65) //Auspuff2
	//2:0.80(102) 3:0.17(22) 4:0.66(84) 8:0.72(92) 14:0.77(98) 15:1.00(127) 16:0.46(58) 17:0.00(0) 18:1.00(127) // Plasma
	//2:0.52(66) 3:0.75(95) 4:0.80(101) 6:0.07(9) 8:0.64(81) 9:0.03(4) 14:1.00(127) 15:1.00(127) 16:0.91(115) 17:1.00(127) 18:1.00(127) 19:0.00(0) // Twister
	//2:0.87(111) 3:0.83(106) 4:0.67(85) 6:0.12(15) 8:0.84(107) 14:0.78(99) 15:1.00(127) 16:0.52(66) 17:0.00(0) 18:1.00(127) // Plasma (b)
	//2:0.00(0) 3:0.67(85) 4:0.21(27) 8:0.81(103) 9:0.08(10) 14:0.73(93) 15:1.00(127) 16:0.65(82) 17:1.00(127) 18:0.00(0) 25:0.00(0) // open stange

    // New version:
	// Plasma 2:0.91(115) 3:0.50(64) 4:0.33(42) 5:0.98(125) 6:0.51(65) 8:0.69(88) 9:1.00(127) 12:0.00(0) 13:0.00(0) 
	// Sidescroll 2:0.75(95) 3:0.83(105) 4:0.21(27) 5:0.83(106) 6:0.72(92) 8:0.79(100) 9:0.09(12) 12:0.16(20) 13:1.00(127) 
	// Startup (8) 2:0.52(66) 3:0.54(69) 4:0.39(50) 5:0.72(91) 6:0.45(57) 8:0.43(54) 9:1.00(127) 12:0.57(72) 13:1.00(127) 
	// Final 2:0.31(40) 3:0.70(89) 4:0.31(40) 5:0.87(110) 6:0.81(103) 8:0.74(94) 9:0.07(9) 12:0.00(0) 13:0.00(0) 
	// Fasto (8) 2:0.69(87) 3:0.54(68) 4:0.39(49) 5:0.75(95) 6:0.00(0) 8:0.72(92) 9:1.00(127) 12:1.00(127) 13:1.00(127) 
    // flythrough 2:0.99(126) 3:0.80(101) 4:0.69(88) 5:0.93(118) 6:0.72(92) 8:0.69(87) 9:1.00(127) 12:0.00(0) 13:1.00(127) 
	// Bar (3) 2:0.26(33) 3:0.00(0) 4:1.00(127) 5:1.00(127) 6:0.87(110) 8:0.88(112) 9:1.00(127) 12:0.65(83) 13:1.00(127) 
	// Cloud (5) 2:0.54(69) 3:0.20(25) 4:0.71(90) 5:0.76(97) 6:0.59(75) 8:0.90(114) 9:0.29(37) 12:0.06(7) 13:1.00(127)
	// Tunnelb 2:1.00(127) 3:0.54(69) 4:0.35(45) 5:0.72(92) 6:0.54(69) 8:0.71(90) 9:1.00(127) 12:0.00(0) 13:1.00(127) 
	// Baralong 2:0.72(92) 3:0.22(28) 4:0.73(93) 5:1.00(127) 6:0.78(99) 8:1.00(127) 9:1.00(127) 12:1.00(127) 13:1.00(127) 
	parameterMatrix[1] = params.getParam(2, 0.75f);
	parameterMatrix[2] = params.getParam(3, 0.83f);
	parameterMatrix[3] = params.getParam(4, 0.21f);
	parameterMatrix[4] = params.getParam(5, 0.83f);
	parameterMatrix[5] = params.getParam(6, 0.72f);
	parameterMatrix[6] = params.getParam(8, 0.79f);
	parameterMatrix[7] = params.getParam(9, 0.09f);
	parameterMatrix[8] = params.getParam(12, 0.1f);
	parameterMatrix[9] = params.getParam(13, 1.0f);
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

	fallingBall(ftime);
}

