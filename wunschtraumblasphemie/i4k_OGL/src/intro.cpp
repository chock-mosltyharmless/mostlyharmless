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

// You can access the wave output for basic synchronization
extern double outwave[][2];

float frand();
int rand();

// -------------------------------------------------------------------
//                          INTRO SCRIPT:
// -------------------------------------------------------------------
// Would go here somehow

// -------------------------------------------------------------------
//                          SHADER CODE:
// -------------------------------------------------------------------

const GLchar *fragmentMainParticle="\
#version 330 core\n\
in vec2 g2fPosition;\n\
out vec4 out_Color;\n\
\n\
void main(void)\n\
{\n\
	float intensity = smoothstep(1.0, 0.6, length(g2fPosition));\n\
	out_Color = vec4(vec3(intensity) * vec3(0.3, 0.2, 0.1), 1.0);\n\
}";

const GLchar *geometryMainParticle="\
#version 330 compatibility\n\
layout(points) in;\n\
layout(triangle_strip, max_vertices=4) out;\n\
out vec2 g2fPosition;\n\
\n\
void main()\n\
{\n\
	gl_Position = gl_in[0].gl_Position + vec4(-0.01, 0.01, 0.0, 0.0);\n\
	g2fPosition = vec2(-1.0, 1.0);\n\
	EmitVertex();\n\
	gl_Position = gl_in[0].gl_Position + vec4(0.01, 0.01, 0.0, 0.0);\n\
	g2fPosition = vec2(1.0, 1.0);\n\
	EmitVertex();\n\
	gl_Position = gl_in[0].gl_Position + vec4(-0.01, -0.01, 0.0, 0.0);\n\
	g2fPosition = vec2(-1.0, -1.0);\n\
	EmitVertex();\n\
	gl_Position = gl_in[0].gl_Position + vec4(0.01, -0.01, 0.0, 0.0);\n\
	g2fPosition = vec2(1.0, -1.0);\n\
	EmitVertex();\n\
	EndPrimitive();\n\
}";

const GLchar *vertexMainParticle="\
#version 330 core\n\
layout (location=0) in vec3 in_Position;\n\
void main(void)\
{\
	vec3 transformPos = in_Position;\n\
    gl_Position = vec4(transformPos, 1.);\n\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define FRACTAL_TREE_DEPTH 4
#define FRACTAL_NUM_LEAVES (1 << (2 * (FRACTAL_TREE_DEPTH-1)))
// It's actually less than that:
#define FRACTAL_TREE_NUM_ENTRIES (FRACTAL_NUM_LEAVES * 2)

// This is only used if SHADER_DEBUG is on, but I will leave it for now.
HWND hWnd;
#ifdef SHADER_DEBUG
char err[4097];
#endif

static GLuint shaderPrograms[1];
// The vertex array and vertex buffer
unsigned int vaoID;
// 0 is for particle positions, 1 is for particle matrices
unsigned int vboID;
// And the actual vertices
GLfloat vertices[FRACTAL_NUM_LEAVES * 3];

// -------------------------------------------------------------------
//                          Data for the fractal:
// -------------------------------------------------------------------

// The fractals are saved in a tree of 4x4 matrices
float fractalTree[FRACTAL_TREE_NUM_ENTRIES][4][4];
int firstTreeLeaf; // The first of the leaf entries

// The transformation matrices of the fractal
float transformMat[4][4][4];

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
	mat[0][2] = xz - yw;
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

	glGenBuffers(1, &vboID); // Generate our Vertex Buffer Object  
	
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vboID); // Bind our Vertex Buffer Object  
	glBufferData(GL_ARRAY_BUFFER, FRACTAL_NUM_LEAVES * 3 * sizeof(GLfloat),
		         NULL, GL_DYNAMIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
	glVertexAttribPointer(0, // attribute
						  3, // size
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

// The seed value of the random number generator is accessed here!
extern unsigned long seed;
void createTransforms(unsigned long startSeed)
{
	seed = startSeed;
	float quaternion[4];

	for (int transform = 0; transform < 4; transform++)
	{
		// Set quaternion values
		float invQuatLen = 0.0f;
		for (int qdim = 0; qdim < 4; qdim++)
		{
			// RANDOM!!!
			quaternion[qdim] = frand() - 0.5f;
			invQuatLen += quaternion[qdim] * quaternion[qdim];
		}
		invQuatLen = 1.0f / (float)sqrt(invQuatLen);
		for (int qdim = 0; qdim < 4; qdim++)
		{
			quaternion[qdim] *= invQuatLen;
		}

		matrixFromQuaternion(quaternion, transformMat[transform]);

		// Multiply by scaling
		for (int dim = 0; dim < 3; dim++)
		{
			// RANDOM!!!
			float scaling = frand() * 0.25f + 0.625f;
			for (int i = 0; i < 4; i++)
			{
				transformMat[transform][dim][i] *= scaling;
			}
		}

		// Transform x-y
		for (int dim = 0; dim < 3; dim++)
		{
			// RANDOM!!!
			transformMat[transform][dim][3] = frand() - 0.5f;
		}
	}
}

void buildTree(void)
{
	// Set the first entry in the tree to unity (assuming all zeroes)
	fractalTree[0][0][0] = 1.0f;
	fractalTree[0][1][1] = 1.0f;
	fractalTree[0][2][2] = 1.0f;
	fractalTree[0][3][3] = 1.0f;

	int startEntry = 0;
	int endEntry = 1;
	for (int depth = 1; depth < FRACTAL_TREE_DEPTH; depth++)
	{
		firstTreeLeaf = startEntry;

		for (int entry = startEntry; entry < endEntry; entry++)
		{
			for (int transform = 0; transform < 4; transform++)
			{
				int destEntry = entry * 4 + 1 + transform;
				matrixMult(transformMat[transform], fractalTree[entry],
					       fractalTree[destEntry]);
			}
		}

		startEntry = endEntry;
		endEntry += 1 << (2*depth);
	}
}

// Create the particle locations and move them to the GPU
void generateParticles(void)
{
	// Copy the positions to the vertices:
	for (int entry = 0; entry < FRACTAL_NUM_LEAVES; entry++)
	{
		for (int dim = 0; dim < 3; dim++)
		{
			vertices[entry*3 + dim] =
				fractalTree[entry + firstTreeLeaf][dim][3];
		}
	}

	// Send the data to the graphics card
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		            FRACTAL_NUM_LEAVES * 3 * sizeof(GLfloat), vertices);  
}

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

	// Create the transformation matrices from random values
	createTransforms(itime / 3000);
	buildTree();
	generateParticles();

	// Create the matrix tree
	// But first: I have to write the geometry shader stuffiskaya!

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

    // render
    glDisable( GL_CULL_FACE );
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);

	//parameterMatrix[0] = ftime; // time	
	// get music information
#ifdef USEDSOUND
	double loudness = 1.0;
	int musicPos = (((itime)*441)/10);
	for (int k = 0; k < 4096; k++)
	{
		loudness += outwave[musicPos][k]*outwave[musicPos][k];
	}
	parameterMatrix[15] = (float)log(loudness) * (1.f/24.f); // This makes it silent?
#endif

	// set the viewport (not neccessary?)
	//glGetIntegerv(GL_VIEWPORT, viewport);
	//glViewport(0, 0, XRES, YRES);
	
	glUseProgram(shaderPrograms[0]);

	//glBindVertexArray(vaoID);
	glDrawArrays(GL_POINTS, 0, FRACTAL_NUM_LEAVES);
	//glBindVertexArray(0);
}
