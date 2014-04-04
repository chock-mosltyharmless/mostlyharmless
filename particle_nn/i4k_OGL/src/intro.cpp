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

// You can access the wave output for basic synchronization
extern double outwave[][2];

// -------------------------------------------------------------------
//                          INTRO SCRIPT:
// -------------------------------------------------------------------
// Would go here somehow

// -------------------------------------------------------------------
//                          SHADER CODE:
// -------------------------------------------------------------------

const GLchar *fragmentMainParticle="\
#version 330 core\n\
in vec2 n;\
in vec4 w;\
out vec4 t;\
\
void main(void)\
{\
t=vec4(vec3(smoothstep(1.,.7,length(n)))*w.xyz,1.);\
}";

const GLchar *geometryMainParticle="\
#version 330 core\n\
layout(points) in;\
layout(triangle_strip, max_vertices=4) out;\
in vec4 o[];\
out vec4 w;\
out vec2 n;\
uniform mat4 r;\
void main()\
{\
vec4 p=gl_in[0].gl_Position;\
float s=.001+abs(p.z-.5)*.08;\
float l=.001/s;\
s=min(s,.2*p.w);\
float b=(.15+.5*smoothstep(.003,.0,abs(sin(r[0][0]*.02)-sin(o[0].r*100.))))*pow(l,2.);\
float c=pow(l,1.5);\
b*=smoothstep(c,.5*c,o[0].a)/c;\
if (o[0].a<=c&&b>.02)\
{\
gl_Position=p+vec4(-.56*s,s,.0,.0);\
n=vec2(-1.,1.);\
w=b*o[0];\
EmitVertex();\
gl_Position=p+vec4(.56*s,s,.0,.0);\
n=vec2(1.,1.);\
w=b*o[0];\
EmitVertex();\
gl_Position=p+vec4(-.56*s,-s,.0,.0);\
n=vec2(-1.,-1.);\
w=b*o[0];\
EmitVertex();\
gl_Position=p+vec4(.56*s,-s,.0,.0);\
n=vec2(1.,-1.);\
w=b*o[0];\
EmitVertex();\
}\
EndPrimitive();\
}";

const GLchar *vertexMainParticle="\
#version 330 core\n\
layout (location=0) in vec4 p;\
layout (location=1) in vec4 c;\
uniform mat4 t;\
out vec4 o;\
void main(void)\
{\
vec4 b=t[3];\
b.a=c.a;\
mat4 m=t;\
m[3].xyz=vec3(0.);\
vec3 f=(vec4(p.xyz, 1.0) * m).xyz;\
gl_Position=vec4(f,f.z+.0001);\
o=mix(c,b,0.4);\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define NUM_INNER_PARTICLES 2048
float innerParticlePos[NUM_INNER_PARTICLES][3];
float innerParticleColor[NUM_INNER_PARTICLES][4];

#define NUM_PARTICLES 1024
float particlePos[NUM_PARTICLES][3];
float particleMovement[NUM_PARTICLES][3];
float particleColor[NUM_PARTICLES][4];
// This one is probably constant...
float particleOrientation[NUM_PARTICLES][4][4];

// This is only used if SHADER_DEBUG is on, but I will leave it for now.
HWND hWnd;
#ifdef SHADER_DEBUG
char err[4097];
#endif

static GLuint shaderPrograms[1];
// The vertex array and vertex buffer
unsigned int vaoID;
// 0 is for particle positions, 1 is for particle colors
unsigned int vboID[2];

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

// Multiplices two 4x4 matrices
void matrixMult(float src1[4][4], float src2[4][4], float dest[4][4])
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			dest[i][j] = 0.0f;
			for (int k = 0; k < 4; k++)
			{
				dest[i][j] += src1[i][k] * src2[k][j];
			}
		}
	}
}

// Creates a pseudorandom quaternion using the current seed.
// You can also specify some value that does some modification to the quat.
void randomQuaternion(float quat[4], unsigned int *seed)
{
	float invQuatLen = 0.0f;
	for (int qdim = 0; qdim < 4; qdim++)
	{
		// RANDOM!!!
		quat[qdim] = frand(seed) - 0.5f;
		invQuatLen += quat[qdim] * quat[qdim];
	}
	invQuatLen = 1.0f / (float)sqrt(invQuatLen);
	for (int qdim = 0; qdim < 4; qdim++)
	{
		quat[qdim] *= invQuatLen;
	}
}

// Creates a 4x4 rotation matrix from a quaternion?
void matrixFromQuaternion(float quat[4], float mat[4][4])
{
	float xsquare = 2 * quat[0] * quat[0];
	float ysquare = 2 * quat[1] * quat[1];
	float zsquare = 2 * quat[2] * quat[2];
	float xy = 2 * quat[0] * quat[1];
	float zw = 2 * quat[2] * quat[3];
	float xz = 2 * quat[0] * quat[2];
	float yw = 2 * quat[1] * quat[3];
	float yz = 2 * quat[1] * quat[2];
	float xw = 2 * quat[0] * quat[3];
	
	mat[0][0] = 1 - ysquare - zsquare;
	mat[0][1] = xy - zw;
	mat[0][2] = xz + yw;
	mat[0][3] = 0.0f;
	mat[1][0] = xy + zw;
	mat[1][1] = 1 - xsquare - zsquare;
	mat[1][2] = yz - xw;
	mat[1][3] = 0.0f;
	mat[2][0] = xz - yw;
	mat[2][1] = yz + xw;
	mat[2][2] = 1 - xsquare - ysquare;
	mat[2][3] = 0.0f;
	mat[3][0] = 0.0f;
	mat[3][1] = 0.0f;
	mat[3][2] = 0.0f;
	mat[3][3] = 1.0f;
}

void intro_init( void )
{
	// Create the initial pariticle positions
	unsigned int frandSeed = timeGetTime();
	for (int i = 0; i < NUM_INNER_PARTICLES; i++)
	{
		for (int dim = 0; dim < 3; dim++)
		{
			innerParticlePos[i][dim] = frand(&frandSeed) - 0.5f;
			innerParticleColor[i][dim] = 0.1f * frand(&frandSeed) + 0.9f;
		}

		innerParticleColor[i][3] = frand(&frandSeed);
	}

	// Create the particle start data
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		for (int dim = 0; dim < 3; dim++)
		{
			particlePos[i][dim] = frand(&frandSeed) - 0.5f;
			particleMovement[i][dim] = 0.00001f * (frand(&frandSeed) - 0.5f);
		}

		float quaternion[4];
		randomQuaternion(quaternion, &frandSeed);
		matrixFromQuaternion(quaternion, particleOrientation[i]);

		for (int col = 0; col < 4; col++)
		{
			particleColor[i][col] = frand(&frandSeed);
		}
	}

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop...
	
	// init objects:
	GLuint vMainParticle = glCreateShader(GL_VERTEX_SHADER);
	GLuint gMainParticle = glCreateShader(GL_GEOMETRY_SHADER_EXT);
	GLuint fMainParticle = glCreateShader(GL_FRAGMENT_SHADER);
	shaderPrograms[0] = glCreateProgram();
	// compile sources:
	glShaderSource(vMainParticle, 1, &vertexMainParticle, NULL);
	glCompileShader(vMainParticle);
	glShaderSource(gMainParticle, 1, &geometryMainParticle, NULL);
	glCompileShader(gMainParticle);
	glShaderSource(fMainParticle, 1, &fragmentMainParticle, NULL);
	glCompileShader(fMainParticle);

#ifdef SHADER_DEBUG
	// Check programs
	int tmp, tmp2;
	glGetShaderiv(vMainParticle, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(vMainParticle, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "vMainParticle shader error", MB_OK);
		return;
	}
	glGetShaderiv(gMainParticle, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(gMainParticle, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "gMainParticle shader error", MB_OK);
		return;
	}
	glGetShaderiv(fMainParticle, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(fMainParticle, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "fMainParticle shader error", MB_OK);
		return;
	}
#endif

	// link shaders:
	glAttachShader(shaderPrograms[0], vMainParticle);
	glAttachShader(shaderPrograms[0], gMainParticle);
	glAttachShader(shaderPrograms[0], fMainParticle);
	glLinkProgram(shaderPrograms[0]);

#ifdef SHADER_DEBUG
	int programStatus;
	glGetProgramiv(shaderPrograms[0], GL_LINK_STATUS, &programStatus);
	if (programStatus == GL_FALSE)
	{
		MessageBox(hWnd, "Could not link program", "Shader 0 error", MB_OK);
		return;
	}
#endif

	// Set up vertex buffer and stuff
	glGenVertexArrays(1, &vaoID); // Create our Vertex Array Object  
	glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  
  
	int maxAttrt;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrt);

	glGenBuffers(2, vboID); // Generate our Vertex Buffer Object  
	
	// Vertex array position data
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vboID[0]); // Bind our Vertex Buffer Object  
	glBufferData(GL_ARRAY_BUFFER, NUM_INNER_PARTICLES * 3 * sizeof(GLfloat),
		         NULL, GL_DYNAMIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
	glVertexAttribPointer(0, // attribute
						  3, // size
						  GL_FLOAT, // type
						  GL_FALSE, // normalized?
						  0, // stride
						  (void*)0); // array buffer offset
	
	// Vertex array color data
	// change to GL_STATIC_DRAW and single update for speed.
	// Back to Dynamic...
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vboID[1]); // Bind our Vertex Buffer Object  
	glBufferData(GL_ARRAY_BUFFER, NUM_INNER_PARTICLES * 4 * sizeof(GLfloat),
		         NULL, GL_DYNAMIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
	glVertexAttribPointer(1, // attribute
						  4, // size
						  GL_FLOAT, // type
						  GL_FALSE, // normalized?
						  0, // stride
						  (void*)0); // array buffer offset

#ifdef SHADER_DEBUG
	// Get all the errors:
	GLenum errorValue = glGetError();
	if (errorValue != GL_NO_ERROR)
	{
		char *errorString = (char *)gluErrorString(errorValue);
		MessageBox(hWnd, errorString, "Init error", MB_OK);
		return;
	}
#endif
}

// Create the locations and move them to the GPU
void generateParticles(void)
{
	bool firstTime = true;

	// Send the data to the graphics card
	glBindBuffer(GL_ARRAY_BUFFER, vboID[0]); // Bind our Vertex Buffer Object  
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		            NUM_INNER_PARTICLES * 3 * sizeof(GLfloat), innerParticlePos);

	// Send the color data to the graphics card (only once for speed...)
#if 0
	if (firstTime)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboID[1]); // Bind our Vertex Buffer Object  
		glBufferSubData(GL_ARRAY_BUFFER, 0,
						FRACTAL_NUM_LEAVES * 4 * sizeof(GLfloat),
						&(fractalColorTree[firstTreeLeaf][0]));
		firstTime = false;
	}
#else
	glBindBuffer(GL_ARRAY_BUFFER, vboID[1]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 
					NUM_INNER_PARTICLES * 4 * sizeof(GLfloat),
					innerParticleColor);
#endif
}

// This function first generates a transformation matrix for the camera
// Then it multiplies it with all the transforms in the fractal tree leaves...
void generateOGLTransforms(float ftime)
{
	float finalTransform[4][4] = {{.1f,0,0,0},{0,.1f,0,0},{0,0,.1f,0},{0,0,0,1.0f}};

#if 0
	float quaternion[2][4];
	float distance[2];

	randomQuaternion(quaternion[0], 0.0f);
	randomQuaternion(quaternion[1], 0.0f);

	// linear interpolation of the two quaternions
	float invQuatSize = 0.0f;
	for (int dim = 0; dim < 4; dim++)
	{
		quaternion[0][dim] = 0.5f*ftime * quaternion[1][dim] + 
			(1.0f - 0.5f*ftime) * quaternion[0][dim];
		invQuatSize += quaternion[0][dim] * quaternion[0][dim];
	}
	invQuatSize = 1.0f / sqrtf(invQuatSize);
	for (int dim = 0; dim < 4; dim++)
	{
		quaternion[0][dim] *= invQuatSize;
	}

	matrixFromQuaternion(quaternion[0], finalTransform);

	distance[0] = frand(&transformationSeed) - 0.2f;
	distance[1] = frand(&transformationSeed) + 0.2f;
	finalTransform[2][3] = ftime * distance[0] + (1.0f - ftime) * distance[1];
#endif

	finalTransform[2][3] = 0.5f; // TODO: not needed?

	// multiply camera transform with leaf matrices
	for (int draw = 0; draw < NUM_PARTICLES; draw++)
	{
		float tmpMatrix[4][4];
		matrixMult(finalTransform, particleOrientation[draw], tmpMatrix);

		// encode the color in the matrix
		tmpMatrix[3][0] = particleColor[draw][0];
		tmpMatrix[3][1] = particleColor[draw][1];
		tmpMatrix[3][2] = particleColor[draw][2];

		tmpMatrix[0][3] += particlePos[draw][0];
		tmpMatrix[1][3] += particlePos[draw][1];
		tmpMatrix[2][3] += particlePos[draw][2];
		
		int location = glGetUniformLocation(shaderPrograms[0], "t");
		glUniformMatrix4fv(location, 1, GL_FALSE, &(tmpMatrix[0][0]));
		glDrawArrays(GL_POINTS, 0, NUM_INNER_PARTICLES);
	}
}

// This function moves the particles
void moveParticles(long itime)
{
	static long lastTime = 0;
	static bool firstCall = true;
	if (firstCall)
	{
		lastTime = itime;
		firstCall = false;
	}

	// Go in 5 ms steps
	while (lastTime < itime)
	{
		// move particles along their direction
		for (int i = 0; i < NUM_PARTICLES; i++)
		{
			for (int dim = 0; dim < 3; dim++)
			{
				particlePos[i][dim] += particleMovement[i][dim];
			}
		}

		// Slow down particles
		for (int i = 0; i < NUM_PARTICLES; i++)
		{
			for (int dim = 0; dim < 3; dim++)
			{
				particleMovement[i][dim] *= 0.99998f;
			}
		}

		// Gravitate to the center
		for (int i = 0; i < NUM_PARTICLES; i++)
		{
			for (int dim = 0; dim < 3; dim++)
			{
				particleMovement[i][dim] -= particlePos[i][dim] * 0.000000001f;
			}
		}

		lastTime += 5; // 5 ms steps
	}
}

// This function is finding the right thing to do based on the time that we are in
void doTheScripting(long itime)
{	
	// Update of positions and stuff
	moveParticles(itime);

	// Create the stuff based on the current timing thing
	generateParticles();
	generateOGLTransforms(0.0f);
}

void intro_do( long itime )
{
	static int lastTime = 0;
	static int timeDiff = 0;

	// smooth the time
	timeDiff = (100 * timeDiff + (itime - lastTime) * 28) / 128;
	itime = lastTime + timeDiff;
	lastTime = itime;

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Set the render matrix
	float parameterMatrix[4][4];
	parameterMatrix[0][0] = itime / 44100.0f;
	int location = glGetUniformLocation(shaderPrograms[0], "r");
	glUniformMatrix4fv(location, 1, GL_FALSE, &(parameterMatrix[0][0]));
    // render
    glDisable( GL_CULL_FACE );
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);

	doTheScripting(itime);

	// set the viewport (not neccessary?)
	//glGetIntegerv(GL_VIEWPORT, viewport);
	//glViewport(0, 0, XRES, YRES);
	
	glUseProgram(shaderPrograms[0]);
}
