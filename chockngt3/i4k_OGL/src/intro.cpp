//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include "glext.h"

#include "config.h"
#include "intro.h"
#include "mzk.h"
#include "Parameter.h"

#include "mathhelpers.h"
#include "Swarm.h"

#include "bass.h"


// You can access the wave output for basic synchronization
extern double outwave[][2];

static GLuint creditsTexture;
static GLuint bunnyTexture;
static int *creditsTexData[1024*1024];

struct TGAHeader
{
	unsigned char identSize;
	unsigned char colourmapType;
	unsigned char imageType;

	// This is a stupid hack to fool the compiler.
	// I do not know what happens if I compile it
	// under release conditions.
	unsigned char colourmapStart1;
	unsigned char colourmapStart2;
	unsigned char colourmapLength1;
	unsigned char colourmapLength2;
	unsigned char colourmapBits;

	short xStart;
	short yStart;
	short width;
	short height;
	unsigned char bits;
	unsigned char descriptor;
};


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

const GLchar *fragmentPic="\
#version 330 core\n\
	in vec4 o;\
	in vec2 texCoord;\
	out vec4 t;\
	uniform sampler2D Texture0;\n\
	\
	void main(void)\
	{\
		vec4 col = texture(Texture0, texCoord);\n\
		t = col * o;\n\
}";

#if 1
const GLchar *vertexPic="\
#version 330 core\n\
in vec4 p;\n\
in vec4 c;\n\
in vec2 texC;\
out vec4 o;\n\
out vec2 texCoord;\
	void main(void)\n\
	{\
	gl_Position = vec4(p.xyz, 1.0);\n\
	o = c;\n\
texCoord = texC;\
	}";
#endif

const GLchar *vertexMainParticle="\
#version 330 core\n\
in vec4 p;\
in vec4 c;\
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
o=mix(c,b,0.6);\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define FRACTAL_TREE_DEPTH 6
#define FRACTAL_NUM_LEAVES (1 << (2 * (FRACTAL_TREE_DEPTH-1)))
#define FRACTAL_NUM_PRE_LEAVES (1 << (2 * (FRACTAL_TREE_DEPTH-2)))
// It's actually less than that:
#define FRACTAL_TREE_NUM_ENTRIES (FRACTAL_NUM_LEAVES * 2)

// This is only used if SHADER_DEBUG is on, but I will leave it for now.
HWND hWnd;
#ifdef SHADER_DEBUG
char err[4097];
#endif

static GLuint shaderPrograms[2];
// The vertex array and vertex buffer
unsigned int vaoID;
// 0 is for particle positions, 1 is for particle colors
unsigned int vboID[3];
// And the actual vertices
GLfloat vertices[FRACTAL_NUM_LEAVES * 3];

// seed values for random values
unsigned int transformationSeed;

#define NUM_RANDOM_VALUES 65536
unsigned int someRandomValues[NUM_RANDOM_VALUES];

// scene stuff
// The duration of one scene. Later on this should not be constant
const int sceneDuration = 300000;
static int sceneStartTime = 0; // The time that this scene started
static int sceneNumber = 0; // The number of the scene that is currently running

// -------------------------------------------------------------------
//                          Data for the fractal:
// -------------------------------------------------------------------

// The fractals are saved in a tree of 4x4 matrices
float fractalTree[FRACTAL_TREE_NUM_ENTRIES][4][4];
float fractalColorTree[FRACTAL_TREE_NUM_ENTRIES][4];
int firstTreeLeaf; // The first of the leaf entries
int firstPreLeaf; // vertex before tree leaves

// The transformation matrices of the fractal
float transformMat[4][4][4];
//const float transformColor[4][3] =
//2:0.99(127) 3:0.96(123) 4:0.73(94) 5:0.99(127) 6:0.90(115) 8:0.99(127)
//    9:0.75(96) 12:0.87(111) 13:0.99(127) 14:0.99(127) 15:0.99(127) 16:0.99(127) 
float transformColor[4][3] =
{
	{0.35f, 0.31f, 0.00f},
	{0.2f, 0.16f, 0.22f},
	{0.0f, 0.38f, 0.54f},
	{0.16f, 0.f, 0.f},
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
		quat[qdim] = frand(&transformationSeed) - 0.5f + 0.25f * sinf(0.125f*changer+qdim);
		invQuatLen += quat[qdim] * quat[qdim];
	}
	invQuatLen = 1.0f / (float)sqrt(invQuatLen);
	for (int qdim = 0; qdim < 4; qdim++)
	{
		quat[qdim] *= invQuatLen;
	}
}

void randomQuaternion2(float quat[4], float pos)
{
	while (pos > NUM_RANDOM_VALUES - 1) pos -= NUM_RANDOM_VALUES-1;

	float quat1[4];
	float quat2[4];

	transformationSeed = someRandomValues[(int)pos];
	randomQuaternion(quat1, 0.5f);
	transformationSeed = someRandomValues[(int)pos + 1];
	randomQuaternion(quat2, 0.5f);

	float transer = pos - (int)pos;

	float invQuatSize = 0.0f;
	for (int dim = 0; dim < 4; dim++)
	{
		quat[dim] = transer * quat2[dim] + 
			(1.0f - transer) * quat1[dim];
		invQuatSize += quat[dim] * quat[dim];
	}
	invQuatSize = 1.0f / sqrtf(invQuatSize);
	for (int dim = 0; dim < 4; dim++)
	{
		quat[dim] *= invQuatSize;
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
	// Start scene based on time
	sceneNumber = timeGetTime();
	
	// generate random numbers
	for (int i = 0; i < NUM_RANDOM_VALUES; i++)
	{
		someRandomValues[i] = irand(&transformationSeed);
	}

	initSwarm();

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop...
	
	// init objects:
	GLuint vMainParticle = glCreateShader(GL_VERTEX_SHADER);
	GLuint gMainParticle = glCreateShader(GL_GEOMETRY_SHADER_EXT);
	GLuint fMainParticle = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint vPic = glCreateShader(GL_VERTEX_SHADER);
	GLuint fPic = glCreateShader(GL_FRAGMENT_SHADER);
	shaderPrograms[0] = glCreateProgram();
	shaderPrograms[1] = glCreateProgram();
	// compile sources:
	glShaderSource(vPic, 1, &vertexPic, NULL);
	glCompileShader(vPic);
	glShaderSource(vMainParticle, 1, &vertexMainParticle, NULL);
	glCompileShader(vMainParticle);

	glShaderSource(fPic, 1, &fragmentPic, NULL);
	glCompileShader(fPic);

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
#if 1
	glGetShaderiv(fPic, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(fPic, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "fPic shader error", MB_OK);
		return;
	}
	glGetShaderiv(vPic, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(vPic, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "vPic shader error", MB_OK);
		return;
	}
#endif
#endif

	// link shaders:
		glBindAttribLocation(shaderPrograms[0], 0, "p");
		glBindAttribLocation(shaderPrograms[0], 1, "c");
		glBindAttribLocation(shaderPrograms[1], 0, "p");
		glBindAttribLocation(shaderPrograms[1], 1, "c");
		glBindAttribLocation(shaderPrograms[1], 2, "texC");


	glAttachShader(shaderPrograms[0], vMainParticle);
	glAttachShader(shaderPrograms[0], gMainParticle);
	glAttachShader(shaderPrograms[0], fMainParticle);
	glLinkProgram(shaderPrograms[0]);
	glAttachShader(shaderPrograms[1], vPic);
	glAttachShader(shaderPrograms[1], fPic);
	glLinkProgram(shaderPrograms[1]);

#ifdef SHADER_DEBUG
	int programStatus;
	glGetProgramiv(shaderPrograms[0], GL_LINK_STATUS, &programStatus);
	if (programStatus == GL_FALSE)
	{
		MessageBox(hWnd, "Could not link program", "Shader 0 error", MB_OK);
		return;
	}
#if 1
	glGetProgramiv(shaderPrograms[1], GL_LINK_STATUS, &programStatus);
	if (programStatus == GL_FALSE)
	{
		MessageBox(hWnd, "Could not link program", "Shader 1 error", MB_OK);
		return;
	}
#endif
#endif

	// Set up vertex buffer and stuff
	glGenVertexArrays(1, &vaoID); // Create our Vertex Array Object  
	glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  
  
	int maxAttrt;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrt);

	glGenBuffers(3, vboID); // Generate our Vertex Buffer Object  
	
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
		         NULL, GL_DYNAMIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
	glVertexAttribPointer(1, // attribute
						  4, // size
						  GL_FLOAT, // type
						  GL_FALSE, // normalized?
						  0, // stride
						  (void*)0); // array buffer offset

	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, vboID[2]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat),
		         NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(2,
						  2,
						  GL_FLOAT,
						  GL_FALSE,
						  0,
						  (void*)0);
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


#if 1
	// Load credits texture
	glGenTextures(1, &creditsTexture);
	glBindTexture(GL_TEXTURE_2D, creditsTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	FILE *cfpid = fopen("TEX.tga", "rb");
	if (cfpid == 0) return;
	// load header
	TGAHeader tgaHeader;
	fread(&tgaHeader, 1, sizeof(tgaHeader), cfpid);		
	// load image data
	int textureSize = 1024 * 1024 * 4;
	fread(creditsTexData, 1, textureSize, cfpid);	
	// TODO: Mip Mapping!!!!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, 1024, 1024,
					  GL_BGRA, GL_UNSIGNED_BYTE, creditsTexData);
	glDisable(GL_TEXTURE_2D);						// Enable Texture Mapping
	fclose(cfpid);

	// Load bunny texture
	glGenTextures(1, &bunnyTexture);
	glBindTexture(GL_TEXTURE_2D, bunnyTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	cfpid = fopen("bunny.tga", "rb");
	if (cfpid == 0) return;
	// load header
	fread(&tgaHeader, 1, sizeof(tgaHeader), cfpid);		
	// load image data
	textureSize = 512 * 512 * 4;
	fread(creditsTexData, 1, textureSize, cfpid);	
	// TODO: Mip Mapping!!!!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, 512, 512,
					  GL_BGRA, GL_UNSIGNED_BYTE, creditsTexData);
	glDisable(GL_TEXTURE_2D);						// Enable Texture Mapping
	fclose(cfpid);
#endif
}

// The seed value of the random number generator is accessed here!
void createTransforms(float changer)
{
	float quaternion[4];
	float amounter = changer * 2.7f * 4.0f;
	while (amounter > NUM_RANDOM_VALUES-100) amounter -= NUM_RANDOM_VALUES-100;

	for (int transform = 0; transform < 4; transform++)
	{
		// Set quaternion values
		//randomQuaternion(quaternion, changer);
		randomQuaternion2(quaternion, amounter + transform*3.1f);
		matrixFromQuaternion(quaternion, transformMat[transform]);

		// Multiply by scaling
		for (int dim = 0; dim < 3; dim++)
		{
			// RANDOM!!!
			//float scaling = frand() * 0.25f + 0.625f;
			//float scaling = (0.875f - frand(&transformationSeed) * frand(&transformationSeed));
			transformationSeed = someRandomValues[(int)amounter + dim*3];
			float scaling1 = (0.875f - frand(&transformationSeed) * frand(&transformationSeed));
			transformationSeed = someRandomValues[(int)amounter+1 + dim*3];
			float scaling2 = (0.875f - frand(&transformationSeed) * frand(&transformationSeed));
			float p = amounter - (int)amounter;
			float scaling = p * scaling2 + (1.0f - p) * scaling1;
			for (int i = 0; i < 4; i++)
			{
				transformMat[transform][dim][i] *= scaling;
			}
		}

		// Transform x-y
		for (int dim = 0; dim < 3; dim++)
		{
			// RANDOM!!!
			transformationSeed = someRandomValues[(int)amounter + dim*2];
			float v1 = frand(&transformationSeed);
			transformationSeed = someRandomValues[(int)amounter+1 + dim*2];
			float v2 = frand(&transformationSeed);
			float p = amounter - (int)amounter;
			float scaling = p*v2 + (1.0f - p) * v1;

			transformMat[transform][dim][3] = scaling - 0.5f;
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
		firstPreLeaf = firstTreeLeaf;
		firstTreeLeaf = startEntry;

		// seed is stored...
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
				fractalColorTree[destEntry][3] = frand(&transformationSeed);
			}
		}

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
	if (true || firstTime)
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
void generateOGLTransforms(float ftime)
{
	float quaternion[2][4];
	float distance[2];
	float finalTransform[4][4];
	//randomQuaternion(quaternion[0], 0.0f);
	//randomQuaternion(quaternion[1], 0.0f);
	randomQuaternion2(quaternion[0], ftime);

#if 0
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
#endif

	matrixFromQuaternion(quaternion[0], finalTransform);

	distance[0] = frand(&transformationSeed) - 0.2f;
	distance[1] = frand(&transformationSeed) + 0.2f;
	finalTransform[2][3] = ftime * distance[0] + (1.0f - ftime) * distance[1];

	// multiply camera transform with leaf matrices
	for (int draw = 0; draw < FRACTAL_NUM_PRE_LEAVES; draw++)
	{
		float tmpMatrix[4][4];
		matrixMult(finalTransform, fractalTree[firstPreLeaf+draw], tmpMatrix);
		for (int s = 0; s < 4; s++)
		{
			for (int t = 0; t < 4; t++)
			{
				fractalTree[firstPreLeaf+draw][s][t] = tmpMatrix[s][t];
			}
		}

		// encode the color in the matrix
		fractalTree[firstPreLeaf+draw][3][0] = fractalColorTree[firstPreLeaf+draw][0];
		fractalTree[firstPreLeaf+draw][3][1] = fractalColorTree[firstPreLeaf+draw][1];
		fractalTree[firstPreLeaf+draw][3][2] = fractalColorTree[firstPreLeaf+draw][2];
		int location = glGetUniformLocation(shaderPrograms[0], "t");
		glUniformMatrix4fv(location, 1, GL_FALSE, &(fractalTree[firstPreLeaf+draw][0][0]));
		glDrawArrays(GL_POINTS, 0, FRACTAL_NUM_LEAVES);
	}
}

void drawQuad(float startX, float endX, float startLY, float endLY, float startRY, float endRY, float startV, float endV, float alpha)
{
		// set up matrices
		glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);


#if 0
	glColor4f(1.0f, 1.0f, 1.0f, alpha);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, endV);
	glVertex3f(startX, endY, 0.99f);
	glTexCoord2f(1.0f, endV);
	glVertex3f(endX, endY, 0.99f);
	glTexCoord2f(1.0f, startV);
	glVertex3f(endX, startY, 0.99f);
	glTexCoord2f(0.0, startV);
	glVertex3f(startX, startY, 0.99f);
	glEnd();
#endif
	
	float pos[4][3] =
	{
		{startX, startLY, 0.5f},
		{startX, endLY, 0.5f},
		{endX, startRY, 0.5f},
		{endX, endRY, 0.5f}
	};
	float col[4][4] = 
	{
		{1.0f, 1.0f, 1.0f, alpha},
		{1.0f, 1.0f, 1.0f, alpha},
		{1.0f, 1.0f, 1.0f, alpha},
		{1.0f, 1.0f, 1.0f, alpha},
	};
	float texC[4][2] = 
	{
		{0.0f, startV},
		{0.0f, endV},
		{1.0f, startV},
		{1.0f, endV},
	};

	glUseProgram(shaderPrograms[1]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[0]); // Bind our Vertex Buffer Object  
	glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * 3 * sizeof(GLfloat), pos);
	glBindBuffer(GL_ARRAY_BUFFER, vboID[1]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * 4 * sizeof(GLfloat), col);
	glBindBuffer(GL_ARRAY_BUFFER, vboID[2]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * 4 * sizeof(GLfloat), texC);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram(shaderPrograms[0]);
	
	glDisable(GL_TEXTURE_2D);
}


void doTheMainPart(long itime)
{	
	static int lastTime = 0;
	float fDeltaTime = (itime - lastTime) / 44100.0f;
	lastTime  = itime;

	float fCurTime = (float)itime / 44100.0f;

	// Distance to the center object
	// TODO: Hitchcock-effect by means of gluPerspective!
	float cameraDist = 20.0f;
	float cameraComeTime = 150.0f;
	if (fCurTime > cameraComeTime)
	{
		cameraDist = 3.0f + 17.0f / (1.0f + 0.1f * ((fCurTime - cameraComeTime) * (fCurTime - cameraComeTime)));
	}

	while (itime >= sceneStartTime + sceneDuration)
	{
		sceneStartTime += sceneDuration;
		sceneNumber++;
	}

	// Create the swarm positions
	float metaAmount = 1.0f;
	int signedDistanceID = 1; // sphere
	if (fCurTime > 61.0f) signedDistanceID = 2; // metaballs
	//signedDistanceID = 0; // nothing

	int pathID = 0; // 8 moveTo points
	if (fCurTime > 80.0f)
	{
		pathID = 1; // 4 lines
		signedDistanceID = 0; // Nothing

		if (fCurTime > 111.0f)
		{
			pathID = 0; // 4 move to points
			signedDistanceID = 2; // metaballs
		}
	}

	// In reversed field
	if (fCurTime > 170.0f)
	{
		pathID = 0;
		signedDistanceID = 2;
		if (fCurTime > 190.0f)
		{
			pathID = 1;
			signedDistanceID = 0;
		}
		if (fCurTime > 210.0f)
		{
			pathID = 1;
			signedDistanceID = 2;
		}
		if (fCurTime > 217.0f)
		{
			pathID = 0;
			signedDistanceID = 1;
		}
	}

	// generate destiations
	float overshoot = 0.0f;
	if (fCurTime > 19.f)
	{
		float relTime = fCurTime - 19.f;
		overshoot = 2.0f * sinf(relTime*0.17f) * (-cosf(relTime * 0.75f) + 1.0f);
	}
	if (fCurTime > 190.0f)
	{
		float relTime = (fCurTime - 190.0f);
		overshoot /= relTime * relTime + 1.0f;
	}
	updateSwarmDestinations(pathID, fDeltaTime, overshoot);

	// update direction of the Triangles
	updateSwarmWithSignedDistance(signedDistanceID, fDeltaTime, metaAmount);

	// move all triangles
	moveSwarm(fDeltaTime);

	int numTrisRender2;
	int sttt = 501000;
	if (itime < sttt)
	{
		numTrisRender2 = (itime - 3300) / 150;
		numTrisRender2 = numTrisRender2 > NUM_TRIANGLES/4 ? NUM_TRIANGLES/4 : numTrisRender2;
		numTrisRender2 = numTrisRender2 < 0 ? 0 : numTrisRender2;
	}
	else
	{
		numTrisRender2 = NUM_TRIANGLES/4 + (itime-sttt)/10;
		numTrisRender2 = numTrisRender2 > NUM_TRIANGLES ? NUM_TRIANGLES : numTrisRender2;
		numTrisRender2 = numTrisRender2 < 0 ? 0 : numTrisRender2;
	}
	//numTrisRender2 = 1000;
	float triBrightness = fCurTime / 30.0f + 0.2f;
	if (triBrightness > 1.0f) triBrightness = 1.0f;

	float beating = 1.0f - 0.25f * fabsf((float)sin(fCurTime*4.652f));

	int sceneID = sceneNumber; // That's the easiest thing, really
	int sceneTime = itime - sceneStartTime;

	// Create the stuff based on the current timing thing
	transformationSeed = sceneID;
	//createTransforms((float)sceneTime / 44100.0f);
	createTransforms(fCurTime* 0.3f);
	buildTree();
	generateParticles();
	transformationSeed = sceneID;

	float pretransform[4][4] = {0};
	pretransform[0][0] = 1.0f;
	pretransform[1][1] = 16.0f / 9.0f;
	pretransform[2][2] = 1.0f;
	pretransform[3][3] = 1.0f;

	float quaternion[2][4];
	float tmpMat[4][4];
	float finalTransform[4][4];
	randomQuaternion2(quaternion[0], fCurTime*0.3f);
	matrixFromQuaternion(quaternion[0], tmpMat);
	matrixMult(pretransform, tmpMat, finalTransform);

	//finalTransform[2][3] = transformAmount * distance[0] + (1.0f - transformAmount) * distance[1];
	finalTransform[2][3] = cameraDist * 0.03f;
	finalTransform[0][3] = 0.12f;
	finalTransform[1][3] = -0.12f;
	//finalTransform[2][3] = 1.5f;

	float brrr = 0.0f;

	float wobStartTime = 6.5f;
	if (fCurTime > wobStartTime && fCurTime < 120.0f)
	{
		// 0.7215f is too fast.
		// 0.721f is too fast.
		// 0.7205f is too fast.
		float wob = (fCurTime - wobStartTime) * 0.7195f;
		wob -= (int)wob;
		if (wob < 0.2f)
		{
			wob *= 5.0f;
			brrr += sinf(wob * 3.1415f) / (fCurTime + 5.0f) * 4.0f;
		}
	}

	// 140 is off... hmm 140.5f is better?
#if 0
	wobStartTime = 140.5f;
	if (fCurTime > wobStartTime && fCurTime < 220.0f)
	{
		float wob = (fCurTime - wobStartTime) * 0.7195f;
		wob -= (int)wob;
		if (wob < 0.2f)
		{
			wob *= 5.0f;
			brrr += sin(wob * 3.1415f) / (wobStartTime - fCurTime + 220.0f) * 24.0f;
		}
	}
#endif


	if (fCurTime > 191.0f)
	{
		brrr += fCurTime - 191.0f;
	}

	float burnStartTime = 10.0f;
	float colorburn = 0.0f;
	if (fCurTime > burnStartTime)
	{
		float burni = (fCurTime - burnStartTime) * 0.17f;

		if (burni < 1.0f)
		{
			colorburn = sinf(fCurTime*25.0f) * sinf(burni * 3.1415f) * 0.5f;
		}
	}

	burnStartTime = 21.5f;
	if (fCurTime > burnStartTime)
	{
		float burnTime = fCurTime - burnStartTime;
		float burni = (sqrtf(sqrtf(burnTime * 0.3f)));

		if (burni < 1.0f)
		{
			colorburn = sinf(fCurTime*25.0f) * sinf(burni * 3.1415f) * sinf(burni * 3.1415f) * 1.5f;
		}
	}

	burnStartTime = 32.6f;
	if (fCurTime > burnStartTime)
	{
		float burnTime = fCurTime - burnStartTime;
		float burni = (sqrtf(sqrtf(burnTime * 0.25f)));

		if (burni < 1.0f)
		{
			colorburn = sinf(fCurTime*25.0f) * sinf(burni * 3.1415f) * sinf(burni * 3.1415f) * 0.75f;
		}
	}

	burnStartTime = 77.2f;
	if (fCurTime > burnStartTime)
	{
		float burnTime = fCurTime - burnStartTime;
		float burni = (sqrtf(sqrtf(burnTime * 0.25f)));

		if (burni < 1.0f)
		{
			colorburn = sinf(fCurTime*25.0f) * sinf(burni * 3.1415f) * sinf(burni * 3.1415f) * 0.75f;
		}
	}

	burnStartTime = 111.5f;
	if (fCurTime > burnStartTime)
	{
		float burnTime = fCurTime - burnStartTime;
		float burni = (sqrtf(sqrtf(burnTime * 0.85f)));

		if (burni < 1.0f)
		{
			colorburn = sinf(fCurTime*8.0f) * sinf(burni * 3.1415f) * sinf(burni * 3.1415f) * 2.75f;
		}
	}

	// multiply camera transform with leaf matrices
	for (int draw = 0; draw < numTrisRender2; draw++)
	{
		float tmpMatrix[4][4];
		float transmat[4][4];
		float size = 1.5f * brrr + 0.1f + 0.15f * colorburn;
		transmat[0][2] = size * tris.direction[draw][0];
		transmat[1][2] = size * tris.direction[draw][1];
		transmat[2][2] = size * tris.direction[draw][2];
		transmat[3][2] = 0.0f;
		transmat[0][1] = size * tris.normal[draw][0];
		transmat[1][1] = size * tris.normal[draw][1];
		transmat[2][1] = size * tris.normal[draw][2];
		transmat[3][1] = 0.0f;
		transmat[0][0] = size * (tris.direction[draw][1] * tris.normal[draw][2] - tris.direction[draw][2] * tris.normal[draw][1]);
		transmat[1][0] = size * (tris.direction[draw][2] * tris.normal[draw][0] - tris.direction[draw][0] * tris.normal[draw][2]);
		transmat[2][0] = size * (tris.direction[draw][0] * tris.normal[draw][1] - tris.direction[draw][1] * tris.normal[draw][0]);
		transmat[3][0] = 0.0f;
		size = 0.4f * brrr + 0.1f;
		transmat[0][3] = size * tris.position[draw][0];
		transmat[1][3] = size * tris.position[draw][1];
		transmat[2][3] = size * tris.position[draw][2];
		transmat[3][3] = 1.0f;

		//matrixMult(finalTransform, fractalTree[firstPreLeaf+draw], tmpMatrix);
		matrixMult(finalTransform, transmat, tmpMatrix);

		// encode the color in the matrix
		const float colorA[3] = {0.3f, 0.5f, 1.0f};
		const float colorB[3] = {1.0f, 0.8f, 0.3f};
		const float colorC[3] = {2.0f, 0.4f, 0.1f};
		float color[3];
		float colorT = fCurTime * 0.01f;
		if (colorT > 1.0f) colorT = 1.0f;
		for (int c = 0; c < 3; c++)
		{
			color[c] = colorA[c] * (1.0f - colorT) + colorT * colorB[c];
		}
		if (fCurTime > 100.0f)
		{
			colorT = (fCurTime - 100.0f) * 0.1f - ((draw*draw*17)%63) * 0.1f;
			if (colorT < 0.0f) colorT = 0.0f;
			if (colorT > 1.0f) colorT = 1.0f;
			for (int c = 0; c < 3; c++)
			{
				color[c] = colorB[c] * (1.0f - colorT) + colorT * colorC[c];
			}
		}
		// Some color noise
		for (int c = 0; c < 3; c++)
		{
			float n = ((((draw*draw+13)*(c*c+7)+(draw*(c+1))) % 100) - 50) * 0.0025f;
			color[c] += n;
		}

		tmpMatrix[3][0] = color[0] * beating * 0.8f * triBrightness * (1.0f + colorburn*2.5f);
		tmpMatrix[3][1] = color[1] * beating * 0.8f * triBrightness * (1.0f + colorburn*2.5f);
		tmpMatrix[3][2] = color[2] * beating * 0.8f * triBrightness * (1.0f + colorburn*2.5f);

		int location = glGetUniformLocation(shaderPrograms[0], "t");
		glUniformMatrix4fv(location, 1, GL_FALSE, &(tmpMatrix[0][0]));
		glDrawArrays(GL_POINTS, 0, FRACTAL_NUM_LEAVES);
	}

	// Draw the "background"
	createTransforms(fCurTime* 0.01f);
	buildTree();
	generateParticles();

	for (int i = 0; i < 1000; i++)
	{
		float randquat[4];
		randomQuaternion2(randquat, fCurTime*.1f + i*0.37f);
		float transmat[4][4];
		float tmpMatrix[4][4];
		matrixFromQuaternion(randquat, transmat);
		for (int i =0; i < 15; i++)
		{
			transmat[0][i] *= 1.5f;
		}
		transmat[0][3] = randquat[0]*1.f;
		transmat[1][3] = randquat[1]*1.f;
		transmat[2][3] = randquat[2]*1.f;
		matrixMult(finalTransform, transmat, tmpMatrix);

		tmpMatrix[3][0] = 0.7f + 0.2f * randquat[1];
		tmpMatrix[3][1] = 0.8f + 0.2f * randquat[2];
		tmpMatrix[3][2] = 0.3f + 0.2f * randquat[3];

		int location = glGetUniformLocation(shaderPrograms[0], "t");
		glUniformMatrix4fv(location, 1, GL_FALSE, &(tmpMatrix[0][0]));
		glDrawArrays(GL_POINTS, 0, FRACTAL_NUM_LEAVES);
	}
	
	// The credits:
	float creditsStartTime = 190.8f;
	float creditsStartTime2 = 191.5f;
	if (fCurTime > creditsStartTime)
	{
		glBindTexture(GL_TEXTURE_2D, creditsTexture);
		float timerli = (fCurTime - creditsStartTime) * 0.3f;
		if (timerli < 1.0f)
		{
			float alpha = sinf(timerli * 3.1415f);
			float pos = -0.3f;
			float width = alpha;
			float high = cosf(timerli * 3.1415f) * 0.05f;
			drawQuad(pos-width, pos+width, -0.1f-high, 0.4f+high, -0.1f + high, 0.4f-high, 0.0f, 0.175f, alpha);
		}
		timerli = (fCurTime - creditsStartTime2) * 0.3f;
		if (timerli > 0.0f && timerli < 1.0f)
		{
			float alpha = sinf(timerli * 3.1415f);
			float pos = 0.3f;
			float width = alpha;
			float high = cosf(timerli * 3.1415f) * 0.05f;
			drawQuad(pos-width, pos+width, -0.5f+high, 0.0f-high, -0.5f-high, 0.0f+high, 0.175f, 0.35f, alpha);
		}
	}

	// The bunny
	float bunnyStartTime = 113.2f;
	if (fCurTime > bunnyStartTime)
	{
		glBindTexture(GL_TEXTURE_2D, bunnyTexture);
		float timerli = (fCurTime - bunnyStartTime) * 0.3f;
		if (timerli < 1.0f)
		{
			float alpha = sinf(timerli * 3.1415f);
			float pos = -0.0f;
			float width = alpha * 0.45f;
			float high = cosf(timerli * 3.1415f) * 0.05f;
			drawQuad(pos-width, pos+width, -0.8f-high, 0.8f+high, -0.8f + high, 0.8f-high, 0.01f, 0.99f, alpha);
		}
	}

}

// This function is finding the right thing to do based on the time that we are in
void doTheScripting(long itime)
{	
	while (itime >= sceneStartTime + sceneDuration)
	{
		sceneStartTime += sceneDuration;
		sceneNumber++;
	}

	int sceneID = sceneNumber; // That's the easiest thing, really
	int sceneTime = itime - sceneStartTime;

	// Create the stuff based on the current timing thing
	transformationSeed = sceneID;
	createTransforms((float)sceneTime / 44100.0f);
	buildTree();
	generateParticles();
	transformationSeed = sceneID;
	generateOGLTransforms((float)sceneTime / (float)sceneDuration);
}

void intro_do( long itime )
{
	static int lastTime = 0;
	static int timeDiff = 0;
	//2:0.35(45) 3:0.31(40) 4:0.00(0) 5:0.20(25) 6:0.16(21) 8:0.22(28)
	//9:0.00(0) 12:0.38(48) 13:0.54(69) 14:0.16(21) 15:0.00(0) 16:0.00(0) 

#ifdef _DEBUG
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
#endif

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

	if (false)
	{
		// intro
		doTheScripting(itime);
	}
	else
	{
		// main part
		glUseProgram(shaderPrograms[0]);
		doTheMainPart(itime);
	}
	// set the viewport (not neccessary?)
	//glGetIntegerv(GL_VIEWPORT, viewport);
	//glViewport(0, 0, XRES, YRES);
	
	glUseProgram(shaderPrograms[0]);
}
