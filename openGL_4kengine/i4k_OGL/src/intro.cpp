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

HWND hWnd;

static GLuint shaderPrograms[1];

#ifdef SHADER_DEBUG
char err[4097];
#endif

// The vertex array and vertex buffer
unsigned int vaoID;
unsigned int vboID;
// And the actual vertices
GLfloat vertices[18];

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

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

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

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

	//glClearColor(0.5f, 0.3f, 0.1f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);

	// set the viewport (not neccessary?)
	//glGetIntegerv(GL_VIEWPORT, viewport);
	//glViewport(0, 0, XRES, YRES);
	
	glUseProgram(shaderPrograms[0]);

	//glBindVertexArray(vaoID);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glBindVertexArray(0);
}
