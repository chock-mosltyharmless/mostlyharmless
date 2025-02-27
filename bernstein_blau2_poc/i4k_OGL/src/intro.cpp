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

#include <stdio.h>

#include "config.h"
#include "intro.h"
#include "mzk.h"
#include "Parameter.h"

// -------------------------------------------------------------------
//                          INTRO SCRIPT:
// -------------------------------------------------------------------
// Would go here somehow

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define NUM_PARTICLES_PER_DIM 128
#define TOTAL_NUM_PARTICLES (NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM)

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
GLfloat vertices_[TOTAL_NUM_PARTICLES * 3];
GLfloat colors_[TOTAL_NUM_PARTICLES * 4];

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

// Note that the character data must be allocated.
void LoadTextFile(const char *filename, char *data, int data_size) {
	FILE *fid = fopen(filename, "rb");
	int num_read = fread(data, 1, data_size - 1, fid);
	data[num_read] = 0;
	fclose(fid);
}

// Create the particle locations and move them to the GPU
void GenerateParticles(void) {
    unsigned int seed = 23690984;

    // Set vertex location
    int vertex_id = 0;
    int color_id = 0;
    for (int z = 0; z < NUM_PARTICLES_PER_DIM; z++) {
        float zp = 1.0f - 2.0f * (float)z / (float)NUM_PARTICLES_PER_DIM;
        for (int y = 0; y < NUM_PARTICLES_PER_DIM; y++) {
            float yp = -1.0f + 2.0f * (float)y / (float)NUM_PARTICLES_PER_DIM;
            for (int x = 0; x < NUM_PARTICLES_PER_DIM; x++) {
                float xp = -1.0f + 2.0f * (float)x / (float)NUM_PARTICLES_PER_DIM;
                vertices_[vertex_id++] = xp + 2.0f * frand(&seed) / (float)NUM_PARTICLES_PER_DIM;
                vertices_[vertex_id++] = yp + 2.0f * frand(&seed) / (float)NUM_PARTICLES_PER_DIM;
                vertices_[vertex_id++] = zp + 2.0f * frand(&seed) / (float)NUM_PARTICLES_PER_DIM;
                colors_[color_id++] = 0.9f;
                colors_[color_id++] = 0.7f;
                colors_[color_id++] = 0.5f;
                // fran
                colors_[color_id++] = frand(&seed);
                //colors_[color_id - 1] = 0.5f;
            }
        }
    }
}

void intro_init( void ) {
	// Create and link shader and stuff:

	// init objects:
	GLuint vMainParticle = glCreateShader(GL_VERTEX_SHADER);
	GLuint gMainParticle = glCreateShader(GL_GEOMETRY_SHADER_EXT);
	GLuint fMainParticle = glCreateShader(GL_FRAGMENT_SHADER);
	shaderPrograms[0] = glCreateProgram();
	// compile sources:
	const int kMaxShaderLength = 1000000;
	GLchar *shader_text = new char[kMaxShaderLength + 1];
	LoadTextFile("shaders/vertex_main_particle.txt", shader_text, kMaxShaderLength);
	glShaderSource(vMainParticle, 1, (const GLchar**)&shader_text, NULL);
	glCompileShader(vMainParticle);
	LoadTextFile("shaders/geometry_main_particle.txt", shader_text, kMaxShaderLength);
	glShaderSource(gMainParticle, 1, (const GLchar**)&shader_text, NULL);
	glCompileShader(gMainParticle);
	LoadTextFile("shaders/fragment_main_particle.txt", shader_text, kMaxShaderLength);
	glShaderSource(fMainParticle, 1, (const GLchar**)&shader_text, NULL);
	glCompileShader(fMainParticle);
	delete[] shader_text;

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

    // Fill the vertices_ and colors_ array with reasonable values
    GenerateParticles();

	// Set up vertex buffer and stuff
	glGenVertexArrays(1, &vaoID); // Create our Vertex Array Object  
	glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  
  
	int maxAttrt;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrt);

	glGenBuffers(2, vboID); // Generate our Vertex Buffer Object  

	// Vertex array position data
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vboID[0]); // Bind our Vertex Buffer Object  
	glBufferData(GL_ARRAY_BUFFER, TOTAL_NUM_PARTICLES * 3 * sizeof(GLfloat),
		         vertices_, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
	glVertexAttribPointer(0, // attribute
						  3, // size
						  GL_FLOAT, // type
						  GL_FALSE, // normalized?
						  0, // stride
						  (void*)0); // array buffer offset
	
	// Vertex array color data
	// change to GL_STATIC_DRAW and single update for speed.
	// Move the GenerateParticles copy operation to here.
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vboID[1]); // Bind our Vertex Buffer Object  
	glBufferData(GL_ARRAY_BUFFER, TOTAL_NUM_PARTICLES * 4 * sizeof(GLfloat),
		         colors_, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
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

    // Set parameters to other locations
    parameterMatrix[0][1] = params.getParam(2, 0.5f);
    parameterMatrix[0][2] = params.getParam(3, 0.5f);
    parameterMatrix[0][3] = params.getParam(4, 0.5f);

    parameterMatrix[1][0] = params.getParam(5, 0.5f);
    parameterMatrix[1][1] = params.getParam(6, 0.5f);
    parameterMatrix[1][2] = params.getParam(8, 0.5f);
    parameterMatrix[1][3] = params.getParam(9, 0.5f);

    parameterMatrix[2][0] = params.getParam(12, 0.5f);
    parameterMatrix[2][1] = params.getParam(13, 0.5f);
    parameterMatrix[2][2] = params.getParam(14, 0.5f);
    parameterMatrix[2][3] = params.getParam(15, 0.5f);

    parameterMatrix[3][0] = params.getParam(16, 0.5f);
    parameterMatrix[3][1] = params.getParam(17, 0.5f);
    parameterMatrix[3][2] = params.getParam(18, 0.5f);
    parameterMatrix[3][3] = params.getParam(19, 0.5f);

	int location = glGetUniformLocation(shaderPrograms[0], "r");
	glUniformMatrix4fv(location, 1, GL_FALSE, &(parameterMatrix[0][0]));
    
    location = glGetUniformLocation(shaderPrograms[0], "R");
    parameterMatrix[0][1] = params.getParam(20, 0.5f);
    parameterMatrix[0][2] = params.getParam(21, 0.5f);
    parameterMatrix[0][3] = params.getParam(22, 0.5f);
    glUniformMatrix4fv(location, 1, GL_FALSE, &(parameterMatrix[0][0]));
    // render
    glDisable( GL_CULL_FACE );
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// set the viewport (not neccessary?)
	//glGetIntegerv(GL_VIEWPORT, viewport);
	//glViewport(0, 0, XRES, YRES);
	
	glUseProgram(shaderPrograms[0]);

    glDrawArrays(GL_POINTS, 0, TOTAL_NUM_PARTICLES);
}
