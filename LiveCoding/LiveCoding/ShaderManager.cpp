#include "StdAfx.h"
#include "ShaderManager.h"
#include "glext.h"
#include "GLnames.h"
#include "Configuration.h"

Shader::Shader(GLenum type)
{
	shaderID = glCreateShader(type);
	this->type = type;
}

Shader::~Shader(void)
{
	glDeleteShader(shaderID);
}

int Shader::loadShader(const char *filename, char *errorString)
{
	char combinedName[SM_MAX_FILENAME_LENGTH+1] = SM_DIRECTORY;

	// Set the name of the shader:
	strcpy_s(shaderName, SM_MAX_FILENAME_LENGTH, filename);

	// load from file
	strcat_s(combinedName, SM_MAX_FILENAME_LENGTH, filename);
	FILE *fid;
	if (fopen_s(&fid, combinedName, "rb"))
	{
		sprintf_s(errorString, 	MAX_ERROR_LENGTH,
			      "IO Error\nCould not open %s in ./shaders", combinedName);
		return -1;
	}
	for (int i = 0; i < sizeof(shaderText); i++)
	{
		shaderText[i] = 0;
	}
	int numRead = fread(shaderText, 1, sizeof(shaderText) - 1, fid);
	fclose(fid);
	if (numRead == sizeof(shaderText) - 1)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "Shader Error\nShader %s in ./shaders is too long.", combinedName);
		return -1;
	}
	
	return compileShader(errorString);
}

int Shader::compileShader(char *errorString)
{
	const GLchar *ptText = shaderText;
	glShaderSource(shaderID, 1, &ptText, NULL);
	glCompileShader(shaderID);

	// Check compilation
	int tmp, tmp2;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(shaderID, 4096, &tmp2, errorString);
		errorString[tmp2]=0;
		strcat_s(errorString, MAX_ERROR_LENGTH, "\n");
		strcat_s(errorString, MAX_ERROR_LENGTH, shaderName);
		return -1;
	}
	
	return 0;
}

int Shader::changeShader(const char *newText, char *errorString)
{
	// Copy shaderText to the current shader text
	// If last character was a space and next one is too, don't copy
	int srcIndex = 0;
	int dstIndex = 0;
	char lastCharacter = 0;
	while (newText[srcIndex] != 0)
	{
		if (lastCharacter != ' ' || newText[srcIndex] != ' ')
		{
			shaderText[dstIndex] = newText[srcIndex];
			dstIndex++;
			if (dstIndex > SM_MAX_SHADER_LENGTH - 2)
			{
				sprintf_s(errorString, MAX_ERROR_LENGTH, 
						  "Shader is too long to process.");
				return -1;
			}
		}
		srcIndex++;
	}
	shaderText[dstIndex] = 0;
	dstIndex++;

	// Delete old shader and build new one
	glDeleteShader(shaderID);
	shaderID = glCreateShader(type);
	return compileShader(errorString);
}

ShaderProgram::ShaderProgram(void)
{
	numShaders = 0;
	programID = 0;
}

ShaderProgram::~ShaderProgram(void)
{
	if (numShaders > 0)
	{
		glDeleteProgram(programID);
		programID = 0;
	}
}

int ShaderProgram::init(int numUsedShaders, Shader* shaders[], char *errorText)
{
	if (numShaders != 0)
	{
		sprintf_s(errorText, MAX_ERROR_LENGTH, "Program can only be compiled once.");
		return -1;
	}

	if (numUsedShaders != 2)
	{
		sprintf_s(errorText, MAX_ERROR_LENGTH, "Unsupported number of shaders: %d",
				  numUsedShaders);
		return -1;
	}
	numShaders = numUsedShaders;

	// Start the linking
	// TODO: Check whether it actually worked...
	programID = glCreateProgram();

	for (int i = 0; i < numShaders; i++)
	{
		glAttachShader(programID, shaders[i]->getID());
		usedShader[i] = shaders[i];
	}
	
	glLinkProgram(programID);

	return 0;
}
	
int ShaderProgram::update(Shader *shader, char *errorText)
{
	boolean needsUpdate = false;

	if (!shader) needsUpdate = true;

	for (int i = 0; i < numShaders; i++)
	{
		if (usedShader[i] == shader) needsUpdate = true;
	}

	if (!needsUpdate) return 0;

	if (numShaders == 0)
	{
		sprintf_s(errorText, MAX_ERROR_LENGTH, "Trying to update uninit program...");
	}

	// delete program and link
	glDeleteProgram(programID);
	programID = glCreateProgram();
	for (int i = 0; i < numShaders; i++)
	{
		glAttachShader(programID, usedShader[i]->getID());
	}
	glLinkProgram(programID);

	return 0;
}

boolean ShaderProgram::isProgram(int numUsedShaders, Shader *checkShaders[])
{
	if (numUsedShaders != numShaders)
	{
		return false;
	}

	for (int i = 0; i < numShaders; i++)
	{
		if (checkShaders[i] != usedShader[i])
		{
			return false;
		}
	}

	// no differences found, this is go
	return true;
}

ShaderManager::ShaderManager(void)
{
	numShaders = 0;
	vertexTestShader = 0;
	fragmentTestShader = 0;
}

ShaderManager::~ShaderManager(void)
{
	releaseAll();
}

void ShaderManager::releaseAll(void)
{
	if (vertexTestShader) delete vertexTestShader;
	vertexTestShader = 0;
	if (fragmentTestShader) delete fragmentTestShader;
	fragmentTestShader = 0;
	
	for (int i = 0; i < numShaders; i++)
	{
		if (shader[i]) delete shader[i];
		shader[i] = 0;
	}
	numShaders = 0;

	for (int i = 0; i < numPrograms; i++)
	{
		if (program[i]) delete program[i];
		program[i] = 0;
	}
	numPrograms = 0;
}

int ShaderManager::init(char *errorString)
{
	// Free everything if there was something before.
	releaseAll();

	// Go throught the shaders directory and load all shaders.
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;

	// Go to first file in shaders directory
	hFind = FindFirstFile(SM_DIRECTORY SM_SHADER_WILDCARD, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "IO Error\nThere are no shaders in " SM_DIRECTORY);
		return -1;
	}

	// Load all the shaders in the directory
	do
	{
		// Find out what type of shader this is
		GLenum type = 0;
		switch (ffd.cFileName[strlen(ffd.cFileName) - 4])
		{
		case 'v':
		case 'V':
			type = GL_VERTEX_SHADER;
			break;
		case 'f':
		case 'F':
			type = GL_FRAGMENT_SHADER;
			break;
		default:
			sprintf_s(errorString, MAX_ERROR_LENGTH,
					  "IO Error\nUnknown shader type %s", ffd.cFileName);
			break;
		}

		shader[numShaders] = new Shader(type);
		
		int retVal = shader[numShaders]->loadShader(ffd.cFileName, errorString);
		if (retVal)
		{
			delete shader[numShaders];
			return retVal;
		}
		numShaders++;
	} while (FindNextFile(hFind, &ffd));

	fragmentTestShader = new Shader(GL_FRAGMENT_SHADER);
    vertexTestShader = new Shader(GL_VERTEX_SHADER);

	return 0;
}

int ShaderManager::createProgram(int numUsedShaders, char *shaderNames[], GLuint *programID,
								 char *errorText)
{
	Shader *progShader[SM_MAX_NUM_SHADERS];

	if (numPrograms >= SM_MAX_NUM_PROGRAMS)
	{
		sprintf_s(errorText, MAX_ERROR_LENGTH, "Too many shader programs.");
		return -1;
	}

	// Search for the shaders
	for (int i = 0; i < numUsedShaders; i++)
	{
		for (int j = 0; j < numShaders; j++)
		{
			if (shader[j]->isShader(shaderNames[i]))
			{
				progShader[i] = shader[j];
				break;
			}
		}
		// Did not find the shader
		sprintf_s(errorText, MAX_ERROR_LENGTH, "Unknown Shader: %s",
				  shaderNames[i]);
		return -1;
	}

	// Check if program already exists
	for (int i = 0; i < numPrograms; i++)
	{
		if (program[i]->isProgram(numUsedShaders, progShader))
		{
			// Hooray, the program was already made!
			*programID = program[i]->getID();
			return 0;
		}
	}

	// Before creating a new shader check whether there is space left.
	if (numUsedShaders > SM_MAX_PROGRAM_SHADERS)
	{
		sprintf_s(errorText, MAX_ERROR_LENGTH, "Too many shaders in program: %d",
			      numUsedShaders);
		return -1;
	}

	// And now create the program
	program[numPrograms] = new ShaderProgram();
	int retval = program[numPrograms]->init(numUsedShaders, progShader, errorText);
	if (retval != 0)
	{
		delete program[numPrograms];
		return retval;
	}
	
	// Everything worked well, there is a new program
	*programID = program[numPrograms]->getID();
	numPrograms++;
	return 0;
}