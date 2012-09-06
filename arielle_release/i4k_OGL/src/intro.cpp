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

//#define SHADER_DEBUG

extern double outwave[][2];

float frand();
int rand();

// -------------------------------------------------------------------
//                          INTRO SCRIPT:
// -------------------------------------------------------------------
#define NUM_SCENES 10
#define NUM_PARAMETERS 7
//#define SYNC_MULTIPLIER 361
#define SYNC_MULTIPLIER 95
#define SYNC_ADJUSTER 80

// I need to come up with a good timerimer here...
static unsigned char sceneLength[NUM_SCENES] =
{
	///*
	72, // simple-1
	72, // simple-2
	76, // simple-3
	72, // simple-4
	72, // simple-5
	76, // foerever first part
	72, // simple-4
	72, // simple-5
	80, // foerever first part
	80, // safety
};

	// 2:0.46(59) 3:0.73(93) 4:0.18(23) 14:0.61(78) 15:0.17(22) 16:0.23(30) 17:0.31(40)
	// 2:0.61(78) 3:0.54(69) 4:0.00(0) 14:0.27(34) 15:0.01(1) 16:0.37(47) 17:0.76(97) fully morphable
	// 2:0.72(92) 3:0.59(75) 4:0.80(102) 14:0.47(60) 15:0.04(5) 16:0.31(40) 17:0.74(95) only rotate
	// 2:0.59(75) 3:0.69(88) 4:0.04(5) 14:0.59(76) 15:0.06(8) 16:0.20(26) 17:0.74(95) slow morpher
	// 2:0.70(90) 3:0.44(56) 4:0.98(126) 14:0.52(66) 15:0.41(53) 16:0.14(18) 17:0.80(103) cool yellow
	// 2:0.71(91) 3:0.41(53) 4:0.27(35) 14:0.52(66) 15:0.41(53) 16:0.14(18) 17:0.00(0) decent pointing finger 
	// 2:0.63(80) 3:0.64(82) 4:0.65(83) 14:0.52(66) 15:0.41(53) 16:0.18(23) 17:0.04(5) complex morpher 

static char sceneData[NUM_SCENES][NUM_PARAMETERS] =
{
	// cam distance, cam rotation, morph, seed[4]
	{80,    69-50, 0,    34,   1,  47,  97}, // simple-1
	{80,    69-50, 102,  60,   5,  40,  95}, // simple-2
	{58+50, 7-33, 23,   78,  22,  30,  40}, // foerever
	{90,    69-50, 5,    76,   8,  26,  95}, // simple-3
	{80,    69-50, 126,  66,  53,  18, 103}, // simple-4
	{90,    69-50, 35,   66,  53,  18,   0}, // simple-5
	{80,    82-50, 83,   66,  53,  23,   5}, // simple-5
	{80,    69, 0,    34,   1,  47,  97}, // simple-1
	//{80,    69, 102,  60,   5,  40,  95}, // simple-2
	{58+50, 7-33, 23,   78,  22,  110,  40}, // foerever
	// Safety
	{0, 0, 0, 0, 0, 0, 0},
};
static char sceneDelta[NUM_SCENES][NUM_PARAMETERS] =
{
	{0, 33, 0, 0, 0, 0, 0}, // simple-1
	{0, 33, 0, 0, 0, 0, 0}, // simple-2
	{-50, 33, 0, 0, 0, 0, 0}, // forever
	{0, 33, 40, 0, 0, 0, 0}, // simple-3
	{0, 33, 40, 0, 0, 0, 0}, // simple-4
	{0, 33, 40, 0, 0, 0, 0}, // simple-5
	{0, 33, 120, 0, 0, 0, 0}, // simple-3
	{0, 33, 120, 0, 0, 0, 0}, // simple-4
	//{0, 33, 120, 0, 0, 0, 0}, // simple-5
	{-50, 33, 0, 0, 0, 0, 0}, // forever
	// Safety
	{0, 0, 0, 0, 0, 0, 0},
};

// -------------------------------------------------------------------
//                          SHADER CODE:
// -------------------------------------------------------------------

#if 0
const GLchar *fragmentMainBackground="\
varying vec3 v;\n\
varying mat4 m;\n\
\n\
vec4 randomIteration(vec4 seed)\n\
{\n\
   return fract((seed.zxwy + vec4(0.735, 0.369, 0.438, 0.921)) * vec4(9437.4, 7213.5, 5935.72, 4951.6));\n\
}\n\
\n\
vec4 quatMult( vec4 q1, vec4 q2 ) {\n\
   return vec4(q1.x * q2.x - dot( q1.yzw, q2.yzw ), q1.x * q2.yzw + q2.x * q1.yzw + cross( q1.yzw, q2.yzw ));\n\
}\n\
\n\
mat3 quaternionToMatrix(vec4 q)\n\
{\n\
   vec4 qx = q.x * q;\n\
   vec4 qy = q.y * q;\n\
   vec4 qz = q.z * q;\n\
   return mat3(1.0) - 2. * mat3(qy.y+qz.z,  qz.w-qx.y,  -qx.z-qy.w,\n\
								-qx.y-qz.w, qx.x+qz.z,  qx.w-qy.z,\n\
								qy.w-qx.z,  -qy.z-qx.w, qx.x+qy.y);\n\
}\n\
\n\
float heart(vec3 z)\n\
{\n\
	z = z.xzy*5.0;\n\
	vec3 z2=z*z;\n\
	float lp = z2.x + 2.25*z2.y + z2.z - 1.0;\n\
	float rp = z2.x*z2.z*z.z + 0.08888*z2.y*z2.z*z.z;\n\
	return lp*lp*lp-rp;\n\
}\n\
\n\
void main(void)\n\
{  \n\
   vec4 coreSeed = m[1];\n\
\n\
   vec3 rayDir = normalize(v * vec3(1.0, 0.6, 1.0));\n\
   vec3 camPos = vec3(0.0, 0.0, -m[0][1]);\n\
   \n\
   // rotate camera around y axis\n\
   float alpha = m[0][2] * 4.5;\n\
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
      // This stuff is the transformation information from previous stuff\n\
	  vec4 prevQuaternion = normalize(vec4(cos(m[0][3]), sin(m[0][3]), sin(m[0][3] * 1.3), sin(m[0][3] * 2.7)));\n\
      float prevLength = 1.0;\n\
      vec3 prevMover = vec3(0.0);\n\
      vec3 prevColor = vec3(1.0, 0.4, 0.2);\n\
	  \n\
	  if (m[1][2] < 0.5) {\n\
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
				vec3 lengthes = prevLength * (seed.xyz * seed.xyz * seed.xyz * seed.xyz * vec3(0.2) + vec3(0.05));\n\
				implicitVal = min(implicitVal, length(max(abs((rayPos - ((0.5*seed.wzx - vec3(0.25)) * prevRotationMatrix * prevLength) - prevMover) * quaternionToMatrix(quatMult(normalize(seed - vec4(0.5)), prevQuaternion))) - lengthes, 0.0)) - length(lengthes) * 0.3);\n\
			 }\n\
			 \n\
			 // Non-solid:\n\
			 float nonSolidDist = 1.0e10;\n\
			 for (int k = 0; k < 2; k++)\n\
			 {\n\
				seed = randomIteration(seed);\n\
				vec4 quaternion = quatMult(normalize(seed - vec4(0.5)), prevQuaternion);\n\
				float lengthes = prevLength * (seed.x * 0.3 + 0.25);\n\
				vec3 mover = ((0.5*seed.wzx - vec3(0.25)) * prevRotationMatrix * prevLength) + prevMover;\n\
				float curImplicitVal = length(rayPos - mover) - lengthes;\n\
				if (curImplicitVal < nonSolidDist)\n\
				{\n\
				   nonSolidDist = curImplicitVal;\n\
				   newQuaternion = quaternion;\n\
				   newLength = lengthes;\n\
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
	  }else{\n\
		float Eps = 0.01;\n\
		float vs=heart(rayPos);\n\
		vec3 v=1./Eps*(vec3(heart(rayPos+vec3(Eps,0.,0.)),heart(rayPos+vec3(0.,Eps,0.)),heart(rayPos+vec3(0. ,0.,Eps)) )-vec3(vs));\n\
		implicitVal = vs/(length(v)+Eps);\n\
		prevColor = vec3(1.0, 0.2, 0.2);\n\
	  }\n\
      // I need to do this distance related to for the DOF!      \n\
      totalColor += vec3(1./50., 1./70., 1./90.) *\n\
				3.06 / exp(abs(implicitVal*5.)) * m[3][3];\n\
      totalDensity += 1./ 15. / exp(abs(implicitVal*10.0) + 0.5);\n\
      //if (implicitVal < 0.0)\n\
      stepDepth += abs(implicitVal) * 0.99;      \n\
	  if (implicitVal < 0.0) {\n\
         totalColor = totalColor + (1.-totalDensity) * prevColor;\n\
         totalDensity = 1.0f;\n\
      }\n\
      \n\
      stepSize = max(0.005 * stepDepth, abs(implicitVal) * 0.99);\n\
      rayPos += rayDir * stepSize;\n\
   }\n\
   \n\
   float grad = normalize(rayDir).y;\n\
   totalColor += (1.-totalDensity) * (grad * vec3(0.0,-0.4,-0.3) + (1.-grad)*vec3(0.0,0.4,0.6));\n\
   \n\
   gl_FragColor = vec4(totalColor-vec3(0.0), 1.0);\n\
}";
#else
const GLchar *fragmentMainBackground = ""
 "varying vec3 z;"
 "varying mat4 v;"
 "vec4 n(vec4 v)"
 "{"
   "return fract((v.zxwy+vec4(.735,.369,.438,.921))*vec4(9437.4,7213.5,5935.72,4951.6));"
 "}"
 "vec4 n(vec4 v,vec4 s)"
 "{"
   "return vec4(v.x*s.x-dot(v.yzw,s.yzw),v.x*s.yzw+s.x*v.yzw+cross(v.yzw,s.yzw));"
 "}"
 "mat3 s(vec4 v)"
 "{"
   "vec4 s=v.x*v,c=v.y*v,z=v.z*v;"
   "return mat3(1.)-2.*mat3(c.y+z.z,z.w-s.y,-s.z-c.w,-s.y-z.w,s.x+z.z,s.w-c.z,c.w-s.z,-c.z-s.w,s.x+c.y);"
 "}"
 "float m(vec3 v)"
 "{"
   "v=v.xzy*5.;"
   "vec3 s=v*v;"
   "float z=s.x+2.25*s.y+s.z-1.,c=s.x*s.z*v.z+.08888*s.y*s.z*v.z;"
   "return z*z*z-c;"
 "}"
 "void main()"
 "{"
   "vec4 c=v[1];"
   "vec3 f=normalize(z*vec3(1.,.6,1.)),l=vec3(0.,0.,-v[0][1]);"
   "float i=v[0][2]*4.5;"
   "l.xz=vec2(cos(i)*l.x-sin(i)*l.z,sin(i)*l.x+cos(i)*l.z);"
   "f.xz=vec2(cos(i)*f.x-sin(i)*f.z,sin(i)*f.x+cos(i)*f.z);"
   "vec3 x=l;"
   "float r=8.;"
   "vec3 w=vec3(0.);"
   "float y,e=0.,a=0.;"
   "for(int g=0;length(x)<r&&e<.9&&g<50;g++)"
     "{"
       "float b;"
       "vec4 t=normalize(vec4(cos(v[0][3]),sin(v[0][3]),sin(v[0][3]*1.3),sin(v[0][3]*2.7)));"
       "float o=1.;"
       "vec3 d=vec3(0.),h=vec3(1.,.4,.2);"
       "if(v[1][2]<.5)"
         "{"
           "b=1e+10;"
           "for(int u=0;u<12;u++)"
             "{"
               "vec4 k;"
               "float p;"
               "vec3 F,C;"
               "mat3 Z=s(t);"
               "vec4 Y=c;"
               "for(int X=0;X<4;X++)"
                 "{"
                   "Y=n(Y);"
                   "vec3 W=o*(Y.xyz*Y.xyz*Y.xyz*Y.xyz*vec3(.2)+vec3(.05));"
                   "b=min(b,length(max(abs((x-(.5*Y.wzx-vec3(.25))*Z*o-d)*s(n(normalize(Y-vec4(.5)),t)))-W,0.))-length(W)*.3);"
                 "}"
               "float W=1e+10;"
               "for(int X=0;X<2;X++)"
                 "{"
                   "Y=n(Y);"
                   "vec4 V=n(normalize(Y-vec4(.5)),t);"
                   "float U=o*(Y.x*.3+.25);"
                   "vec3 T=(.5*Y.wzx-vec3(.25))*Z*o+d;"
                   "float S=length(x-T)-U;"
                   "if(S<W)"
                     "W=S,k=V,p=U,F=T,C=Y.xyz;"
                 "}"
               "if(W>b)"
                 "{"
                   "break;"
                 "}"
               "else"
                 " t=k,o=p,d=F,h=.5*h+.5*C;"
             "}"
         "}"
       "else"
         "{"
           "float Y=.01,W=m(x);"
           "vec3 X=1./Y*(vec3(m(x+vec3(Y,0.,0.)),m(x+vec3(0.,Y,0.)),m(x+vec3(0.,0.,Y)))-vec3(W));"
           "b=W/(length(X)+Y);"
           "h=vec3(1.,.2,.2);"
         "}"
       "w+=vec3(.02,1./70.,1./90.)*3.06/exp(abs(b*5.))*v[3][3];"
       "e+=1./15./exp(abs(b*10.)+.5);"
       "a+=abs(b)*.99;"
       "if(b<0.)"
         "w=w+(1.-e)*h,e=1.f;"
       "y=max(.005*a,abs(b)*.99);"
       "x+=f*y;"
     "}"
   "float Y=normalize(f).y;"
   "w+=(1.-e)*(Y*vec3(0.,-.4,-.3)+(1.-Y)*vec3(0.,.4,.6));"
   "gl_FragColor=vec4(w-vec3(0.),1.);"
 "}";
#endif

const GLchar *fragmentOffscreenCopy="\
uniform sampler2D t;\
varying vec3 z;\
varying mat4 v;\
void main(void)\
{\
vec2 n=vec2(fract(sin(dot(z.xy+v[0][0],vec2(12.9898,78.233)))*43758.5453));\
gl_FragColor=texture2D(t,.5*z.xy+.5+.0007*n)+n.x*.02;\
}";

const GLchar *vertexMainObject="\
varying vec3 z;\
varying mat4 v;\
void main(void)\
{\
v=gl_ModelViewMatrix;\
z=vec3(gl_Vertex.xy,.99);\
gl_Position=vec4(z,1.);\
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
#ifdef USEDSOUND
	double loudness = 1.0;
	int musicPos = (((itime-SYNC_ADJUSTER)*441)/10);
	for (int k = 0; k < 4096; k++)
	{
		loudness += outwave[musicPos][k]*outwave[musicPos][k];
	}
	parameterMatrix[15] = (float)log(loudness) * (1.f/24.f); // This makes it silent?
#endif

	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glUseProgram(shaderPrograms[0]);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	//// copy to front
	glViewport(0, 0, viewport[2], viewport[3]);
	glBindTexture(GL_TEXTURE_3D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderPrograms[1]);	
	glRectf(-1.0, -1.0, 1.0, 1.0);
}
