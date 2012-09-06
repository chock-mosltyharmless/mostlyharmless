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
varying vec3 objectPosition;\n\
varying vec3 objectNormal;\n\
\n\
varying mat4 parameters;\n\
\n\
vec4 randomIteration(vec4 seed)\n\
{\n\
   vec4 adder = vec4(0.735, 0.369, 0.438, 0.921);\n\
   vec4 mult = vec4(9437.4, 7213.5, 5935.72, 4951.6);\n\
   return fract((seed.zxwy + adder) * mult);\n\
}\n\
\n\
vec4 quatMult( vec4 q1, vec4 q2 ) {\n\
   vec4 r;\n\
\n\
   r.x = q1.x * q2.x - dot( q1.yzw, q2.yzw );\n\
   r.yzw = q1.x * q2.yzw + q2.x * q1.yzw + cross( q1.yzw, q2.yzw );\n\
\n\
   return r;\n\
}\n\
\n\
mat3 quaternionToMatrix(vec4 q)\n\
{\n\
   vec4 qx = q.x * q;\n\
   vec4 qy = q.y * q;\n\
   vec4 qz = q.z * q;\n\
   mat3 products = mat3(vec3(qy.y+qz.z,  qz.w-qx.y,  -qx.z-qy.w),\n\
                        vec3(-qx.y-qz.w, qx.x+qz.z,  qx.w-qy.z),\n\
                        vec3(qy.w-qx.z,  -qy.z-qx.w, qx.x+qy.y));\n\
   mat3 result = mat3(1.0) - 2. * products;\n\
   \n\
   return result;\n\
}\n\
\n\
float getSphereDistance(float len, vec3 mover, vec3 position)\n\
{\n\
   vec3 relPos = position - mover;\n\
\n\
   return length(relPos) - len;   \n\
}\n\
\n\
float getDistance(mat3 rotation, vec3 lengthes, vec3 mover, vec3 position)\n\
{\n\
   vec3 relPos = position - mover;\n\
   \n\
   vec3 dister = abs(relPos * rotation) - lengthes;\n\
   vec3 mdister = max(dister, 0.0);\n\
   float lendister = length(mdister);\n\
   //if (lendister < 0.01) lendister = max(dister.x, max(dister.y, dister.z));\n\
   \n\
   return lendister - length(lengthes) * 0.3;\n\
}\n\
\n\
void main(void)\n\
{  \n\
   float fTime0_X = parameters[0][0];\n\
   vec4 coreSeed = parameters[1];\n\
\n\
   vec3 rayDir = normalize(objectPosition * vec3(1.0, 0.6, 1.0));\n\
   vec3 camPos = vec3(0.0, 0.0, -parameters[0][1]);\n\
   \n\
   // rotate camera around y axis\n\
   float alpha = parameters[0][2] * 4.5f;\n\
   camPos.xz = vec2(cos(alpha)*camPos.x - sin(alpha)*camPos.z,\n\
                    sin(alpha)*camPos.x + cos(alpha)*camPos.z);\n\
   rayDir.xz = vec2(cos(alpha)*rayDir.x - sin(alpha)*rayDir.z,\n\
                    sin(alpha)*rayDir.x + cos(alpha)*rayDir.z);\n\
   \n\
   vec3 rayPos = camPos;\n\
   float sceneSize = 8.0;\n\
   vec3 totalColor = vec3(0.);\n\
   float stepSize;\n\
   float totalDensity = 0.0;\n\
   float stepDepth = 0.0; // how far I went already.\n\
   \n\
   for(int step = 0; length(rayPos)<sceneSize && totalDensity < 0.9 && step < 50; step++)\n\
   {      \n\
      float implicitVal;\n\
      \n\
      // This stuff is the transformation information from previous stuff\n\
	  float transer = parameters[0][3];\n\
      vec4 prevQuaternion = vec4(cos(transer), sin(transer), sin(transer * 1.3), sin(transer * 2.7));\n\
      prevQuaternion = normalize(prevQuaternion);\n\
      float prevLength = 1.0;\n\
      vec3 prevMover = vec3(0.0);\n\
      vec3 prevColor = vec3(1.0, 0.4, 0.2);\n\
      \n\
      // Multiple boxes\n\
      implicitVal = 1.0e10;\n\
      \n\
      for (int loop = 0; loop < 12; loop++)\n\
      {\n\
         vec4 newQuaternion;\n\
         float newLength;\n\
         vec3 newMover;\n\
         vec3 newColor;\n\
         \n\
         mat3 prevRotationMatrix = quaternionToMatrix(prevQuaternion);\n\
\n\
         // Loop for solid stuff\n\
         vec4 seed = coreSeed;\n\
         for (int k = 0; k < 4; k++)\n\
         {\n\
            seed = randomIteration(seed);\n\
            vec4 quaternion = normalize(seed - vec4(0.5));\n\
            mat3 rotationMatrix = quaternionToMatrix(quatMult(quaternion, prevQuaternion));\n\
            vec3 lengthes = seed.xyz * seed.xyz * seed.xyz * seed.xyz * vec3(0.2) + vec3(0.05);\n\
            lengthes *= prevLength;\n\
            vec3 mover = 0.5*seed.wzx - vec3(0.25);\n\
            mover = (mover * prevRotationMatrix * prevLength) + prevMover;\n\
            float curImplicitVal = getDistance(rotationMatrix, lengthes, mover, rayPos);\n\
            implicitVal = min(implicitVal, curImplicitVal);\n\
         }\n\
         \n\
         // Non-solid:\n\
         float nonSolidDist = 1.0e10;\n\
         for (int k = 0; k < 2; k++)\n\
         {\n\
            seed = randomIteration(seed);\n\
            vec4 quaternion = normalize(seed - vec4(0.5));\n\
            quaternion = quatMult(quaternion, prevQuaternion);\n\
            vec3 lengthes = seed.xyz * vec3(0.3) + vec3(0.25);\n\
            lengthes *= prevLength;\n\
            vec3 mover = 0.5*seed.wzx - vec3(0.25);\n\
            mover = (mover * prevRotationMatrix * prevLength) + prevMover;\n\
            float curImplicitVal = getSphereDistance(lengthes.x, mover, rayPos);\n\
            if (curImplicitVal < nonSolidDist)\n\
            {\n\
               nonSolidDist = curImplicitVal;\n\
               newQuaternion = quaternion;\n\
               newLength = lengthes.x;\n\
               newMover = mover;\n\
               newColor = seed.xyz;\n\
            }\n\
         }\n\
         \n\
         if (nonSolidDist > implicitVal)\n\
         {\n\
            // I will not get closer than where I am now.\n\
            break;\n\
         }\n\
         else\n\
         {\n\
            prevQuaternion = newQuaternion;\n\
            prevLength = newLength;\n\
            prevMover = newMover;\n\
            prevColor = 0.5 * prevColor + 0.5 * newColor;      \n\
         }\n\
      }\n\
      \n\
      // I need to do this distance related to for the DOF!      \n\
      totalColor += vec3(1./50., 1./70., 1./90.) *\n\
                    1.7 / exp(abs(implicitVal*5.) + 0.0) * 1.8;\n\
      totalDensity += 1./ 15. / exp(abs(implicitVal*10.0) + 0.5);\n\
                      // *(1.0 - cos(fTime0_X*3.)*0.5);\n\
      //if (implicitVal < 0.0)\n\
      stepDepth += abs(implicitVal) * 0.95;      \n\
      {\n\
         // TODO: I could make this distance-related, with offset to get the size right?\n\
         float localDensity = implicitVal < 0.0 ? 1.0 : 0.0;\n\
         totalColor = totalColor + (1.-totalDensity) * prevColor * localDensity;\n\
         totalDensity = totalDensity + (1.05-totalDensity) * localDensity;\n\
      }\n\
      \n\
      stepSize = abs(implicitVal) * 0.99;\n\
      stepSize = max(0.005 * stepDepth, stepSize);\n\
      rayPos += rayDir * stepSize;\n\
   }\n\
   \n\
   float grad = normalize(rayDir).y;\n\
   totalColor += (1.-totalDensity) * (grad * vec3(0.0,-0.4,-0.3) + (1.-grad)*vec3(0.0,0.4,0.6));\n\
   \n\
   gl_FragColor = vec4(totalColor-vec3(0.0), 1.0);\n\
}\n\
";

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

	// Create and link shader and stuff:	
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
}

void fallingBall(float ftime)
{
	glMatrixMode(GL_MODELVIEW);	

	parameterMatrix[0] = ftime; // time	
	/* shader parameters */
	// lower row: 2, 3, 4, 5, 6, 8, 9, 12, 13
	// upper row: 14, 15, 16, 17
	// Old stuff, transformations only
		// 2:0.61(78) 3:0.17(22) 4:0.23(30) 5:0.31(40) 
		// 2:0.54(69) 3:0.17(22) 4:0.23(30) 5:0.31(40)
		// 2:0.63(80) 3:0.17(22) 4:0.23(30) 5:0.31(40)
		// 2:0.70(89) 3:0.17(22) 4:0.23(30) 5:0.31(40)
		// 2:0.88(112) 3:0.17(22) 4:0.23(30) 5:0.31(40)
	// 2:0.46(59) 3:0.73(93) 4:0.18(23) 14:0.61(78) 15:0.17(22) 16:0.23(30) 17:0.31(40)
	// 2:0.61(78) 3:0.54(69) 4:0.00(0) 14:0.27(34) 15:0.01(1) 16:0.37(47) 17:0.76(97) fully morphable
	// 2:0.72(92) 3:0.59(75) 4:0.80(102) 14:0.47(60) 15:0.04(5) 16:0.31(40) 17:0.74(95) only rotate
	// 2:0.59(75) 3:0.69(88) 4:0.04(5) 14:0.59(76) 15:0.06(8) 16:0.20(26) 17:0.74(95) slow morpher
	// 2:0.70(90) 3:0.44(56) 4:0.98(126) 14:0.52(66) 15:0.41(53) 16:0.14(18) 17:0.80(103) cool yellow
	// 2:0.71(91) 3:0.41(53) 4:0.27(35) 14:0.52(66) 15:0.41(53) 16:0.14(18) 17:0.00(0) decent pointing finger 
	// 2:0.63(80) 3:0.64(82) 4:0.65(83) 14:0.52(66) 15:0.41(53) 16:0.18(23) 17:0.04(5) complex morpher 
	parameterMatrix[1] = params.getParam(2, 91/128.0f); // camera distance
	parameterMatrix[2] = params.getParam(3, 53/128.0f); // camera rotation
	parameterMatrix[3] = params.getParam(4, 35/128.0f); // stuff transformation
	parameterMatrix[4] = params.getParam(14, 66/128.0f); // seed 1
	parameterMatrix[5] = params.getParam(15, 53/128.0f); // seed 2
	parameterMatrix[6] = params.getParam(16, 18/128.0f); // seed 3
	parameterMatrix[7] = params.getParam(17, 0/128.0f); // seed 4
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glUseProgram(shaderPrograms[0]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// copy to front
	glViewport(0, 0, XRES, YRES);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderPrograms[1]);	
	glRectf(-1.0, -1.0, 1.0, 1.0);
}

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

    // render
	/* Set everything to beginning */
	for (int i = 0; i < 16; i++)
	{
		parameterMatrix[i] = 0.0f;
	}

	fallingBall(ftime);
}

