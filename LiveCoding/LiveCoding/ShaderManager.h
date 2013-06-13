#pragma once

#include "stdafx.h"

#define SM_MAX_NUM_SHADERS 256
#define SM_MAX_NUM_PROGRAMS 256
#define SM_MAX_FILENAME_LENGTH 1024
// Max 1 MB shaders supported
#define SM_MAX_SHADER_LENGTH (1024*1024)
// Maximum number of shaders in a shader program
#define SM_MAX_PROGRAM_SHADERS 2

#define SM_DIRECTORY "shaders/"
#define SM_SHADER_WILDCARD "*.?lsl"

class Shader
{
public:
	// Set the type to GL_VERTEX_SHADER or GL_FRAGMENT_SHADER or...
	Shader(GLenum type);
	~Shader(void);

public: // functions
	// Returns 0 if successful, -1 otherwise
	// The errorString is filled with the error message if anything fails,
	// it must have a length of at least MAX_ERROR_LENGTH
	// Additional space characters are removed.
	int changeShader(const char *newText, char *errorString);

	// Returns 0 if successful, -1 otherwise
	// The errorString is filled with the error message if anything fails,
	// it must have a length of at least MAX_ERROR_LENGTH
	// Filename is relative to the shader directory.
	int loadShader(const char *filename, char *errorString);

	GLuint getID(void) {return shaderID;}
	boolean isShader(const char *name) {return strcmp(shaderName, name) == 0;}

private: // functions
	int compileShader(char *errorString);

private: // data
	char shaderName[SM_MAX_FILENAME_LENGTH+1];
	char shaderText[SM_MAX_SHADER_LENGTH+1];
	// The shaderID will stay the same all the time
	GLuint shaderID;
	GLenum type;
};

// Currently only support for two shaders
class ShaderProgram
{
public:
	ShaderProgram(void);
	~ShaderProgram(void);

public: // functions
	// Create new shader with that number of shaders. The shaders
	// Must be existent and compiled.
	// The program is linked
	// Returns 0 on success
	int init(int numUsedShaders, Shader *shaders[], char *errorText);
	
	// Checks whether this program uses the shader and if so, relinks the
	// program. If shader is set to NULL, the program is always linked
	// Returns 0 on success
	int update(Shader *shader, char *errorText);

	// Check whether this is the program I am looking for
	boolean isProgram(int numUsedShaders, Shader *checkShaders[]);

	GLuint getID(void) {return programID;}

private: // data
	int numShaders;
	Shader *usedShader[SM_MAX_PROGRAM_SHADERS];
	GLuint programID;
};

// Loads all the shaders in the ./shader directory
class ShaderManager
{
public:
	ShaderManager(void);
	~ShaderManager(void);

public: // functions
	// Load all shaders from the ./shader directory.
	// Returns 0 if successful, -1 otherwise
	// The error string must hold space for at least MAX_ERROR_LENGTH
	// characters. It contains errors from compilation/loading
	int init(char *errorString);

	// Creates a new program using a set of shaders.
	// shaderNames are an array of filenames of shaders relative to the shaders dir.
	// You can call this every time a program is used without using additional
	// resources, but you will waste some processing power.
	// TODO: WAIT! I do not know whether the program still exists after an update...
	// returns 0 if successful
	int createProgram(int numUsedShaders, char *shaderNames[], GLuint *programID,
				      char *errorText);

private: // functions
	void releaseAll();

private: // data
	Shader *shader[SM_MAX_NUM_SHADERS];
	int numShaders;
	ShaderProgram *program[SM_MAX_NUM_PROGRAMS];
	int numPrograms;

	// These shaders are used to test written shaders without affecting the
	// system if anything fails.
	Shader *vertexTestShader;
	Shader *fragmentTestShader;
};

