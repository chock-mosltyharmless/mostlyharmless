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
#version 440 core\n\
in vec2 g2f_Position;\n\
in vec4 g2f_Color;\n\
out vec4 out_Color;\n\
\n\
void main(void)\n\
{\n\
	float dist = length(g2f_Position);\n\
	float intensity = smoothstep(1.0, 0.7, dist);\n\
	out_Color = vec4(vec3(intensity) * g2f_Color.rgb, 1.0);\n\
}";

const GLchar *geometryMainParticle="\
#version 440 compatibility\n\
layout(points) in;\n\
layout(triangle_strip, max_vertices=4) out;\n\
in vec4 v2g_Color[];\n\
out vec4 g2f_Color;\n\
out vec2 g2f_Position;\n\
layout (location=1) uniform mat4 parameterMatrix;\n\
\n\
void main()\n\
{\n\
	const float aspect = 9./16.;\n\
	const vec4 pos = gl_in[0].gl_Position;\n\
	\n\
	// Calculate size and color \n\
	float coreBrightness = 0.7;\n\
	float lenseFocus = 0.5; // Should be a uniform!\n\
	float lenseSize = 0.04;\n\
	float defocus = abs(pos.z - lenseFocus);\n\
	float coreParticleSize = 0.001;\n\
	float particleSize = coreParticleSize + defocus * lenseSize;\n\
	float relSize = coreParticleSize / particleSize;\n\
	particleSize = min(particleSize, 0.2 * pos.w);\n\
	// cheat for gamma: relSize^3\n\
	float brightness = coreBrightness * pow(relSize, 2.2);\n\
	\n\
	// Brightness of the DOFer?\n\
	float comparisonValue = pow(relSize, 1.7);\n\
	brightness *= smoothstep(comparisonValue, 0.5*comparisonValue, v2g_Color[0].a) /\n\
		comparisonValue;\n\
	\n\
	if (v2g_Color[0].a <= comparisonValue && brightness > 0.05)\n\
	{\n\
		\n\
		// Drop the thing\n\
		gl_Position = pos + vec4(-aspect*particleSize, particleSize, 0.0, 0.0);\n\
		g2f_Position = vec2(-1.0, 1.0);\n\
		g2f_Color = brightness * v2g_Color[0];\n\
		EmitVertex();\n\
		gl_Position = pos + vec4(aspect*particleSize, particleSize, 0.0, 0.0);\n\
		g2f_Position = vec2(1.0, 1.0);\n\
		g2f_Color = brightness * v2g_Color[0];\n\
		EmitVertex();\n\
		gl_Position = pos + vec4(-aspect*particleSize, -particleSize, 0.0, 0.0);\n\
		g2f_Position = vec2(-1.0, -1.0);\n\
		g2f_Color = brightness * v2g_Color[0];\n\
		EmitVertex();\n\
		gl_Position = pos + vec4(aspect*particleSize, -particleSize, 0.0, 0.0);\n\
		g2f_Position = vec2(1.0, -1.0);\n\
		g2f_Color = brightness * v2g_Color[0];\n\
		EmitVertex();\n\
	}\n\
	EndPrimitive();\n\
}";

const GLchar *vertexMainParticle="\
#version 440 core\n\
layout (location=0) in vec4 in_Position;\n\
layout (location=1) in vec4 in_Color;\n\
layout (location=0) uniform mat4 transformMatrix;\n\
out vec4 v2g_Color;\n\
void main(void)\
{\
	mat4 tMat = transformMatrix;\n\
	vec4 batchColor = transformMatrix[3];\n\
	batchColor.a = in_Color.a;\n\
	tMat[3][0] = 0.;\n\
	tMat[3][1] = 0.;\n\
	tMat[3][2] = 0.;\n\
	vec3 transformPos = (vec4(in_Position.xyz, 1.0) * tMat).xyz;\n\
	// Perspective projection\n\
	// Here I need to think about z-clip aswell. Maybe I do the w stuff?\n\
    gl_Position = vec4(transformPos, transformPos.z+0.0001);\n\
	v2g_Color = mix(in_Color, batchColor, 0.4);\n\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define FRACTAL_TREE_DEPTH 6
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
// 0 is for particle positions, 1 is for particle colors
unsigned int vboID[2];
// And the actual vertices
GLfloat vertices[FRACTAL_NUM_LEAVES * 3];

// -------------------------------------------------------------------
//                          Data for the fractal:
// -------------------------------------------------------------------

// The fractals are saved in a tree of 4x4 matrices
float fractalTree[FRACTAL_TREE_NUM_ENTRIES][4][4];
float fractalColorTree[FRACTAL_TREE_NUM_ENTRIES][4];
int firstTreeLeaf; // The first of the leaf entries

// The transformation matrices of the fractal
float transformMat[4][4][4];
//const float transformColor[4][3] =
//2:0.99(127) 3:0.96(123) 4:0.73(94) 5:0.99(127) 6:0.90(115) 8:0.99(127)
//    9:0.75(96) 12:0.87(111) 13:0.99(127) 14:0.99(127) 15:0.99(127) 16:0.99(127) 
float transformColor[4][3] =
{
	{1.0f, 0.9f, 0.66f},
	{1.0f, 0.35f, 0.4f},
	{0.2f, 0.6f, 1.0f},
	{0.95f, 0.9f, 0.9f},
};

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
void randomQuaternion(float quat[4], float changer)
{
	float invQuatLen = 0.0f;
	for (int qdim = 0; qdim < 4; qdim++)
	{
		// RANDOM!!!
		quat[qdim] = frand() - 0.5f + 0.25f * sin(0.125f*changer+qdim);
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
	glBufferData(GL_ARRAY_BUFFER, FRACTAL_NUM_LEAVES * 3 * sizeof(GLfloat),
		         NULL, GL_DYNAMIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
	glVertexAttribPointer(0, // attribute
						  3, // size
						  GL_FLOAT, // type
						  GL_FALSE, // normalized?
						  0, // stride
						  (void*)0); // array buffer offset
	
	// Vertex array color data
	// change to GL_STATIC_DRAW and single update for speed.
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vboID[1]); // Bind our Vertex Buffer Object  
	glBufferData(GL_ARRAY_BUFFER, FRACTAL_NUM_LEAVES * 4 * sizeof(GLfloat),
		         NULL, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
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

// The seed value of the random number generator is accessed here!
extern unsigned long seed;
void createTransforms(unsigned long startSeed, float changer)
{
	seed = startSeed;
	float quaternion[4];

	for (int transform = 0; transform < 4; transform++)
	{
		// Set quaternion values
		randomQuaternion(quaternion, changer);
		matrixFromQuaternion(quaternion, transformMat[transform]);

		// Multiply by scaling
		for (int dim = 0; dim < 3; dim++)
		{
			// RANDOM!!!
			//float scaling = frand() * 0.25f + 0.625f;
			float scaling = (0.875f - frand() * frand());
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
	fractalColorTree[0][0] = 1.0f;
	fractalColorTree[0][1] = 1.0f;
	fractalColorTree[0][2] = 1.0f;

	int startEntry = 0;
	int endEntry = 1;
	for (int depth = 1; depth < FRACTAL_TREE_DEPTH; depth++)
	{
		firstTreeLeaf = startEntry;

		// seed is stored...
		//unsigned int seedCopy = seed;
		//seed = 1;
		for (int entry = startEntry; entry < endEntry; entry++)
		{
			for (int transform = 0; transform < 4; transform++)
			{
				int destEntry = entry * 4 + 1 + transform;
				matrixMult(transformMat[transform], fractalTree[entry],
					       fractalTree[destEntry]);
				// Create the color:
				for (int col = 0; col < 3; col++)
				{
					fractalColorTree[destEntry][col] =
						0.625f * fractalColorTree[entry][col] +
						0.375f * transformColor[transform][col];
				}
				fractalColorTree[destEntry][3] = frand();
			}
		}
		//seed = seedCopy;

		startEntry = endEntry;
		endEntry += 1 << (2*depth);
	}
}

// Create the particle locations and move them to the GPU
void generateParticles(void)
{
	bool firstTime = true;

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
	glBindBuffer(GL_ARRAY_BUFFER, vboID[0]); // Bind our Vertex Buffer Object  
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		            FRACTAL_NUM_LEAVES * 3 * sizeof(GLfloat), vertices);  

	// Send the color data to the graphics card (only once for speed...)
	if (firstTime)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboID[1]); // Bind our Vertex Buffer Object  
		glBufferSubData(GL_ARRAY_BUFFER, 0,
						FRACTAL_NUM_LEAVES * 4 * sizeof(GLfloat),
						&(fractalColorTree[firstTreeLeaf][0]));
		firstTime = false;
	}
}

// This function first generates a transformation matrix for the camera
// Then it multiplies it with all the transforms in the fractal tree leaves...
void generateOGLTransforms(int sceneID, float ftime)
{
	seed = sceneID; // Change the seed to be equal for the scene
	float quaternion[2][4];
	float distance[2];
	float finalTransform[4][4];
	randomQuaternion(quaternion[0], 0.0f);
	randomQuaternion(quaternion[1], 0.0f);

	// linear interpolation of the two quaternions
	float invQuatSize = 0.0f;
	for (int dim = 0; dim < 4; dim++)
	{
		quaternion[0][dim] = ftime * quaternion[1][dim] + 
			(1.0f - ftime) * quaternion[0][dim];
		invQuatSize += quaternion[0][dim] * quaternion[0][dim];
	}
	invQuatSize = 1.0f / sqrtf(invQuatSize);
	for (int dim = 0; dim < 4; dim++)
	{
		quaternion[0][dim] *= invQuatSize;
	}

	matrixFromQuaternion(quaternion[0], finalTransform);

	distance[0] = 0.7f;//frand() - 0.2f;
	distance[1] = 0.7f;//frand() - 0.2f;
	finalTransform[2][3] = ftime * distance[0] + (1.0f - ftime) * distance[1];

	// multiply camera transform with leaf matrices
	for (int draw = 0; draw < FRACTAL_NUM_LEAVES; draw++)
	{
		float tmpMatrix[4][4];
		matrixMult(finalTransform, fractalTree[firstTreeLeaf+draw], tmpMatrix);
		for (int s = 0; s < 4; s++)
		{
			for (int t = 0; t < 4; t++)
			{
				fractalTree[firstTreeLeaf+draw][s][t] = tmpMatrix[s][t];
			}
		}

		// encode the color in the matrix
		fractalTree[firstTreeLeaf+draw][3][0] = fractalColorTree[firstTreeLeaf+draw][0];
		fractalTree[firstTreeLeaf+draw][3][1] = fractalColorTree[firstTreeLeaf+draw][1];
		fractalTree[firstTreeLeaf+draw][3][2] = fractalColorTree[firstTreeLeaf+draw][2];
		glUniformMatrix4fv(0, 1, GL_FALSE, &(fractalTree[firstTreeLeaf+draw][0][0]));
		glDrawArrays(GL_POINTS, 0, FRACTAL_NUM_LEAVES);
	}
}

// This function is finding the right thing to do based on the time that we are in
void doTheScripting(long itime)
{
	// The duration of one scene. Later on this should not be constant
	const int sceneDuration = 100000;
	static int sceneStartTime = 0; // The time that this scene started
	static int sceneNumber = 0; // The number of the scene that is currently running
	
	while (itime >= sceneStartTime + sceneDuration)
	{
		sceneStartTime += sceneDuration;
		sceneNumber++;
	}

	int sceneID = sceneNumber; // That's the easiest thing, really
	int sceneTime = itime - sceneStartTime;

	// Create the stuff based on the current timing thing
	createTransforms(sceneID, (float)sceneTime / 44100.0f);
	buildTree();
	generateParticles();
	generateOGLTransforms(sceneID, (float)sceneTime / (float)sceneDuration);
}

void intro_do( long itime )
{
	//2:0.35(45) 3:0.31(40) 4:0.00(0) 5:0.20(25) 6:0.16(21) 8:0.22(28)
	//9:0.00(0) 12:0.38(48) 13:0.54(69) 14:0.16(21) 15:0.00(0) 16:0.00(0) 
	transformColor[0][0] = params.getParam(2, 0.35f);
	transformColor[0][1] = params.getParam(3, 0.31f);
	transformColor[0][2] = params.getParam(4, 0.00f);
	
	transformColor[1][0] = params.getParam(5, 0.20f);
	transformColor[1][1] = params.getParam(6, 0.16f);
	transformColor[1][2] = params.getParam(8, 0.22f);
	
	transformColor[2][0] = params.getParam(9, 0.00f);
	transformColor[2][1] = params.getParam(12, 0.38f);
	transformColor[2][2] = params.getParam(13, 0.54f);
	
	transformColor[3][0] = params.getParam(14, 0.16f);
	transformColor[3][1] = params.getParam(15, 0.00f);
	transformColor[3][2] = params.getParam(16, 0.00f);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

    // render
    glDisable( GL_CULL_FACE );
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);

	doTheScripting(itime);

#if 0
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
}
