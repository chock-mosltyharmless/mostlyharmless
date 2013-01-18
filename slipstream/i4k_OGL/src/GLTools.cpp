#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"
#include "GLTools.h"
#include "stdio.h"

#define MAX_SHADER_SIZE 65535
static GLchar shaderSourceBuffer[MAX_SHADER_SIZE + 1];

const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
	 // VBO stuff
	 "glGenBuffers", "glBindBuffer", "glBufferData", "glDeleteBuffers",
};

GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions

void initGLTools()
{
	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);
}

bool loadShaderSource(const char *filename)
{
	memset(shaderSourceBuffer, 0, sizeof(shaderSourceBuffer));
	FILE *fid = fopen(filename, "rb");
	if (fid == NULL)
	{
		MessageBox(hWnd, filename, "Could not open shader", MB_OK);
		return false;
	}
	fread(shaderSourceBuffer, sizeof(char), MAX_SHADER_SIZE, fid);
	fclose(fid);

	return true;
}

bool checkShader(const char *shaderName, GLuint shaderID)
{
	int tmp, tmp2;
	char err[4097];
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(shaderID, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, shaderName, MB_OK);
		return false;
	}

	return true;
}

bool createShaderProgram(const char *vertexShaderFile,
					     const char *geometryShaderFile,
	                     const char *fragmentShaderFile,
						 GLuint *shaderProgram)
{
	const GLchar *stringPointer = shaderSourceBuffer;

	// init objects:
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint gMainTransform = 0;
	if (geometryShaderFile)
	{
		// TODO: I need a newer glext.h!!!
		gMainTransform = glCreateShader(GL_GEOMETRY_SHADER_ARB);
	}
	GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);	
	*shaderProgram = glCreateProgram();
	
	// compile sources:
	if (!loadShaderSource(vertexShaderFile))
	{
		return false;
	}
	glShaderSource(vMainObject, 1, &stringPointer, NULL);
	glCompileShader(vMainObject);
	if (geometryShaderFile)
	{
		if (!loadShaderSource(geometryShaderFile))
		{
			return false;
		}
		glShaderSource(gMainTransform, 1, &stringPointer, NULL);
		glCompileShader(gMainTransform);
	}
	if (!loadShaderSource(fragmentShaderFile))
	{
		return false;
	}
	glShaderSource(fMainBackground, 1, &stringPointer, NULL);
	glCompileShader(fMainBackground);

	// Check programs
	if (!checkShader(vertexShaderFile, vMainObject))
	{
		return false;
	}
	if (gMainTransform)
	{
		if (!checkShader(geometryShaderFile, gMainTransform))
		{
			return false;
		}
	}
	if (!checkShader(fragmentShaderFile, fMainBackground))
	{
		return false;
	}

	// link shaders:
	glAttachShader(*shaderProgram, vMainObject);
	if (geometryShaderFile)
	{
		glAttachShader(*shaderProgram, gMainTransform);
	}
	glAttachShader(*shaderProgram, fMainBackground);
	glLinkProgram(*shaderProgram);

	return true;
}