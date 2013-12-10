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

const GLchar *fragmentMainBackground="\
#version 330 core\n\
in vec3 pass_Position;\n\
out vec4 out_Color;\n\
\n\
void main(void)\n\
{\n\
   out_Color = vec4(pass_Position, 1.0);\n\
}";

const GLchar *vertexMainObject="\
#version 330 core\n\
in vec3 in_Position;\n\
out vec3 pass_Position;\n\
void main(void)\
{\
    gl_Position = vec4(in_Position, 1.);\n\
	pass_Position = in_Position;\n\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define FRACTAL_TREE_DEPTH 3
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
unsigned int vboID;
// And the actual vertices
GLfloat vertices[18];

// -------------------------------------------------------------------
//                          Data for the fractal:
// -------------------------------------------------------------------

// The fractals are saved in a tree of 4x4 matrices
float fractalTree[FRACTAL_TREE_NUM_ENTRIES][4][4];

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
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);
	shaderPrograms[0] = glCreateProgram();
	// compile sources:
	glShaderSource(vMainObject, 1, &vertexMainObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(fMainBackground, 1, &fragmentMainBackground, NULL);
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
	glAttachShader(shaderPrograms[0], vMainObject);
	glAttachShader(shaderPrograms[0], fMainBackground);
	glLinkProgram(shaderPrograms[0]);
	// The in_Position is at 0:
	glBindAttribLocation(shaderPrograms[0], 0, "in_Position");
	//glBinAttribLocation(shader_id, 1, "in_Color");

#ifdef SHADER_DEBUG
	int programStatus;
	glGetProgramiv(shaderPrograms[0], GL_LINK_STATUS, &programStatus);
	if (programStatus == GL_FALSE)
	{
		MessageBox(hWnd, "Could not link program", "Shader 0 error", MB_OK);
		return;
	}
#endif

	vertices[0] = -1.0f; vertices[1] = -1.0f; vertices[2] = 1.0f; // Bottom left corner  
	vertices[3] = -1.0f; vertices[4] = 1.0f; vertices[5] = 1.0f; // Top left corner  
	vertices[6] = 1.0f; vertices[7] = 1.0f; vertices[8] = 1.0f; // Top Right corner  
	vertices[9] = 1.0f; vertices[10] = -1.0f; vertices[11] = 1.0f; // Bottom right corner  
	vertices[12] = -1.0f; vertices[13] = -1.0f; vertices[14] = 1.0f; // Bottom left corner  
	vertices[15] = 1.0f; vertices[16] = 1.0f; vertices[17] = 1.0f; // Top Right corner  

	// Set up vertex buffer and stuff
	glGenVertexArrays(1, &vaoID); // Create our Vertex Array Object  
	glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  
  
	glGenBuffers(1, &vboID); // Generate our Vertex Buffer Object  
	glBindBuffer(GL_ARRAY_BUFFER, vboID); // Bind our Vertex Buffer Object  
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
  
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // Set up our vertex attributes pointer 
	//glBindVertexArray(0);

#ifdef SHADER_DEBUG
	// Get all the errors:
	GLenum errorValue = glGetError();
	if (errorValue != GL_NO_ERROR)
	{
		MessageBox(hWnd, "There was an error.", "Init error", MB_OK);
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

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

	// Create the transformation matrices from random values
	createTransforms(1);

	// Create the matrix tree
	// But first: I have to write the geometry shader stuffiskaya!

    // render
    glDisable( GL_CULL_FACE );

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
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glBindVertexArray(0);
}
