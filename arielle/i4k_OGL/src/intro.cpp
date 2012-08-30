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
   #if 0\n\
   //mat3 result = mat3(vec3(1.-2.*q.y*q.y-2.*q.z*q.z, 2.*q.x*q.y-2.*q.z*q.w,    2.*q.x*q.z+2.*q.y*q.w),\n\
   //                   vec3(2.*q.x*q.y+2.*q.z*q.w,    1.-2.*q.x*q.x-2.*q.z*q.z, 2.*q.y*q.z-2.*q.x*q.w),\n\
   //                   vec3(2.*q.x*q.z-2.*q.y*q.w,    2.*q.y*q.z+2.*q.x*q.w,    1.-2.*q.x*q.x-2.*q.y*q.y));\n\
   mat3 products = mat3(vec3(q.y*q.y+q.z*q.z, q.z*q.w-q.x*q.y, -q.x*q.z-q.y*q.w),\n\
                        vec3(-q.x*q.y-q.z*q.w, q.x*q.x+q.z*q.z, q.x*q.w-q.y*q.z),\n\
                        vec3(q.y*q.w-q.x*q.z, -q.y*q.z-q.x*q.w, q.x*q.x+q.y*q.y));\n\
   mat3 result = mat3(1.0) - 2.*products;\n\
   #else                      \n\
   vec4 qx = q.x * q;\n\
   vec4 qy = q.y * q;\n\
   vec4 qz = q.z * q;\n\
   mat3 products = mat3(vec3(qy.y+qz.z,  qz.w-qx.y,  -qx.z-qy.w),\n\
                        vec3(-qx.y-qz.w, qx.x+qz.z,  qx.w-qy.z),\n\
                        vec3(qy.w-qx.z,  -qy.z-qx.w, qx.x+qy.y));\n\
   mat3 result = mat3(1.0) - 2. * products;\n\
   //mat3 result = mat3(\n\
   //   vec3(1.-2.*(qy.y+qz.z), 2.*(qx.y-qz.w), 2.*(qx.z+qy.w)),\n\
   //   vec3(2.*(qx.y+qz.w), 1.-2.*(qx.x+qz.z), 2.*(qy.z-qx.w)),\n\
   //   vec3(2.*(qx.z-qy.w), 2.*(qy.z+qx.w), 1.-2.*(qx.x+qy.y)));\n\
   #endif\n\
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
   //vec3 mover = transformation[3].xyz;\n\
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
   //vec3 rayDir = normalize(objectPosition);\n\
   vec3 camPos = vec3(0.0, 0.0, -0.5 + 0.05 * sin(fTime0_X*0.5));\n\
   \n\
   // rotate camera around y axis\n\
   #if 1\n\
   float alpha = fTime0_X * 0.3;\n\
   camPos.xz = vec2(cos(alpha)*camPos.x - sin(alpha)*camPos.z,\n\
                    sin(alpha)*camPos.x + cos(alpha)*camPos.z);\n\
   rayDir.xz = vec2(cos(alpha)*rayDir.x - sin(alpha)*rayDir.z,\n\
                    sin(alpha)*rayDir.x + cos(alpha)*rayDir.z);\n\
   #endif                    \n\
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
      vec4 prevQuaternion = vec4(cos(fTime0_X), sin(fTime0_X), sin(fTime0_X * 0.3), sin(fTime0_X * 0.7));\n\
      prevQuaternion = normalize(prevQuaternion);\n\
      float prevLength = 1.0;\n\
      vec3 prevMover = vec3(0.0);\n\
      vec3 prevColor = vec3(1.0, 0.4, 0.2);\n\
      \n\
      // Multiple boxes\n\
      implicitVal = 1.0e10;\n\
      \n\
      for (int loop = 0; loop < 6; loop++)\n\
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
         //float localDensity = implicitVal < 0.0 ? (-implicitVal*100./stepDepth) : 0.0;\n\
         //localDensity = min(localDensity, 1.0);\n\
         totalColor = totalColor + (1.-totalDensity) * prevColor * localDensity;\n\
         totalDensity = totalDensity + (1.05-totalDensity) * localDensity;\n\
      }\n\
      \n\
      //stepSize = abs(implicitVal + 0.001 * stepDepth) * 0.95;\n\
      stepSize = abs(implicitVal) * 0.99;\n\
      stepSize = max(0.005 * stepDepth, stepSize);\n\
      rayPos += rayDir * stepSize;\n\
   }\n\
   \n\
   float grad = normalize(rayDir).y;\n\
   //totalColor *= totalDensity;\n\
   totalColor += (1.-totalDensity) * (grad * vec3(0.0,-0.4,-0.3) + (1.-grad)*vec3(0.0,0.4,0.6));\n\
   //totalColor += (1.-totalDensity) * (vec3(0.3,0.6,1.0));\n\
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
	// 2:0.61(78) 3:0.17(22) 4:0.23(30) 5:0.31(40) 
	// 2:0.54(69) 3:0.17(22) 4:0.23(30) 5:0.31(40)
	// 2:0.63(80) 3:0.17(22) 4:0.23(30) 5:0.31(40)
	// 2:0.70(89) 3:0.17(22) 4:0.23(30) 5:0.31(40)
	// 2:0.88(112) 3:0.17(22) 4:0.23(30) 5:0.31(40)
	parameterMatrix[4] = params.getParam(2, 78/128.0f);
	parameterMatrix[5] = params.getParam(3, 22/128.0f);
	parameterMatrix[6] = params.getParam(4, 30/128.0f);
	parameterMatrix[7] = params.getParam(5, 40/128.0f);
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

