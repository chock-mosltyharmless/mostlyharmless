#pragma once

#include "stdafx.h"

#define SM_MAX_NUM_SHADERS 256
#define SM_MAX_NUM_PROGRAMS 256
#define SM_MAX_FILENAME_LENGTH 1024
// Max 1 MB shaders supported
#define SM_MAX_SHADER_LENGTH (1024*1024)
// MAX 64k shaders supported
#define SM_MAX_PROGRAM_LENGTH (64*1024)
// Maximum number of shaders in a shader program
#define SM_MAX_PROGRAM_SHADERS 3

#define SM_DIRECTORY "shaders/"
#define SM_PROGRESS_DIRECTORY (SM_DIRECTORY "progress/")
#define SM_SHADER_WILDCARD "*.?lsl"
#define SM_PROGRAM_WILDCARD "*.gprg"

// Forward declaration for reference
class ShaderManager;
class Editor;

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
	int loadShader(const char *name, char *errorString);

	GLuint getID(void) {return shaderID;}
	boolean isShader(const char *name) {return strcmp(shaderName, name) == 0;}

	// Save the progress of the shader text to a file
	// The current date and time are encoded in the filename
	void saveProgress(void);

	char *getShaderText(void) {return shaderText;}

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
	// Loads a shader program from a .gprg file.
	// The program is linked
	// The program must be in the shader directory
	// Returns 0 on success
	int loadProgram(const char *name, ShaderManager *manager, char *errorString);
	
	// Checks whether this program uses the shader and if so, relinks the
	// program. If shader is set to NULL, the program is always linked
	// Returns 0 on success
	int update(Shader *shader, char *errorText);

	GLuint getID(void) {return programID;}

	boolean isProgram(const char *name) {return strcmp(programName, name) == 0;}

	void release(void);

private: // data
	int numShaders;
	Shader *usedShader[SM_MAX_PROGRAM_SHADERS];
	char programName[SM_MAX_FILENAME_LENGTH+1];
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

	// Get the ID of a shader program.
	// Returns 0 if successful.
	int getProgramID(const char *programName, GLuint *id, char *errorText);

	// Get the ID of a shader.
	// Returns 0 if successful.
	int getShader(const char *shaderName, Shader **result, char *errorText);

	// Update a shader and compile it into programs if neccessary.
	int updateShader(const char *shaderName, const char *shaderText, char *errorText);

	// Save the progress of the shader text to a file
	// The current date and time are encoded in the filename
	int saveProgress(const char *shaderName, char *errorText, Editor *editor = NULL);

private: // functions
	void releaseAll();

private: // data
	Shader *shader[SM_MAX_NUM_SHADERS];
	int numShaders;
	ShaderProgram *program[SM_MAX_NUM_PROGRAMS];
	int numPrograms;
};

