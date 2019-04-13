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

int Shader::loadShader(const char *name, char *errorString)
{
	char combinedName[SM_MAX_FILENAME_LENGTH+1] = SM_DIRECTORY;

	// Set the name of the shader:
	strcpy_s(shaderName, SM_MAX_FILENAME_LENGTH, name);

	// load from file
	strcat_s(combinedName, SM_MAX_FILENAME_LENGTH, name);
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
	GLchar *compilationText = new GLchar[2*SM_MAX_PROGRAM_LENGTH+1];

	strcpy_s(compilationText, SM_MAX_PROGRAM_LENGTH, shaderText);
	const GLchar *ptText = compilationText;
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

	// Possibly delete old shader and build new one if successful
	GLuint oldShaderID = shaderID;
	shaderID = glCreateShader(type);
	int retVal = compileShader(errorString);
	if (retVal)
	{
		// It failed, get the old one back!
		glDeleteShader(shaderID);
		shaderID = oldShaderID;		
	}
	else
	{
		// The old shader is no longer needed.
		glDeleteShader(oldShaderID);
	}
	
	return retVal;
}

ShaderProgram::ShaderProgram(void)
{
	numShaders = 0;
	programID = 0;
}

ShaderProgram::~ShaderProgram(void)
{
}

void ShaderProgram::release(void)
{
	if (numShaders > 0)
	{
		glDeleteProgram(programID);
		programID = 0;
	}
	numShaders = 0;
}

int ShaderProgram::loadProgram(const char *name, ShaderManager *manager, char *errorString)
{
	char programText[SM_MAX_PROGRAM_LENGTH+1];
	char combinedName[SM_MAX_FILENAME_LENGTH+1] = SM_DIRECTORY;
	// Names of the shaders
	char *shaderName[SM_MAX_PROGRAM_SHADERS];

	// Release the shader program if there was one.
	release();

	// Set the name of the shader:
	strcpy_s(programName, SM_MAX_FILENAME_LENGTH, name);

	// load from file
	strcat_s(combinedName, SM_MAX_FILENAME_LENGTH, name);
	FILE *fid;
	if (fopen_s(&fid, combinedName, "rb"))
	{
		sprintf_s(errorString, 	MAX_ERROR_LENGTH,
			      "IO Error\nCould not open '%s' in ./shaders", combinedName);
		return -1;
	}
	int programSize = fread(programText, 1, SM_MAX_PROGRAM_LENGTH, fid);
	programText[programSize] = 0; // 0-terminate, just to make sure.
	fclose(fid);
	if (programSize == SM_MAX_PROGRAM_LENGTH)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "Shader Error\nprogram %s in ./shaders is too long.", combinedName);
		return -1;
	}
	
	// Very simple parser of the shader program
	int numNames = 0;
	boolean withinName = false; // Currently parsing inside a shader name
	for (int i = 0; i < programSize; i++)
	{
		switch (programText[i])
		{
		case '\t':
		case ' ':
		case '\r':
		case '\n':
			withinName = false;
			programText[i] = 0;
			break;

		default:
			if (!withinName)
			{
				if (numNames >= SM_MAX_PROGRAM_SHADERS)
				{
					sprintf_s(errorString, MAX_ERROR_LENGTH,
						"Shader Error\nprogram %s has too many shaders.", combinedName);
					return -1;
				}
				shaderName[numNames] = &(programText[i]);
				numNames++;
				withinName = true;
			}
			break;
		}
	}

	// For now I only support programs with two shaders.
	if (numNames < 2)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Unsupported number of shaders: %d",
				  numNames);
		return -1;
	}
	numShaders = numNames;

	// Get the shader IDs from the manager:
	for (int i = 0; i < numShaders; i++)
	{
		int retVal = manager->getShader(shaderName[i], &(usedShader[i]), errorString);
		if (retVal)
		{
			numShaders = 0;
			return retVal;
		}
	}

	// Start the linking
	// TODO: Check whether it actually worked...
	programID = glCreateProgram();

	for (int i = 0; i < numShaders; i++)
	{
		glAttachShader(programID, usedShader[i]->getID());
	}
	
	glLinkProgram(programID);

	// TODO: If the linking fails (e.g. because there is no main), then
	// I am severely screwed!!!!!!!

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

ShaderManager::ShaderManager(void)
{
	numShaders = 0;
}

ShaderManager::~ShaderManager(void)
{
	releaseAll();
}

void ShaderManager::releaseAll(void)
{	
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
		case 'j':
		case 'J':
			type = GL_FRAGMENT_SHADER;
			break;
        case 'g':
        case 'G':
            type = GL_GEOMETRY_SHADER_EXT;
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

	// Load the programs in the shaders directory
	// Go to first file in shaders directory
	hFind = FindFirstFile(SM_DIRECTORY SM_PROGRAM_WILDCARD, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "IO Error\nThere are no programs in " SM_DIRECTORY);
		return -1;
	}

	// Load all the programs in the directory
	do
	{
		program[numPrograms] = new ShaderProgram();
		
		int retVal = program[numPrograms]->loadProgram(ffd.cFileName, this, errorString);
		if (retVal)
		{
			delete program[numPrograms];
			return retVal;
		}
		numPrograms++;
	} while (FindNextFile(hFind, &ffd));

	return 0;
}

int ShaderManager::getProgramID(const char *name, GLuint *id, char *errorString)
{
	for (int i = 0; i < numPrograms; i++)
	{
		if (program[i]->isProgram(name))
		{
			*id = program[i]->getID();
			return 0;
		}
	}

	// Got here without finding a texture, return error.
	sprintf_s(errorString, MAX_ERROR_LENGTH,
			  "Could not find shader program '%s'", name);
	return 0;
}

int ShaderManager::updateShader(const char *shaderName, const char *shaderText,
								char *errorText)
{
	Shader *theShader;

	int retval = getShader(shaderName, &theShader, errorText);
	if (retval) return retval;
	
	retval = theShader->changeShader(shaderText, errorText);
	if (retval) return retval;

	// Update programs if applicable
	for (int i = 0; i < numPrograms; i++)
	{
		retval = program[i]->update(theShader, errorText);
		if (retval) return retval;
	}

	return 0;
}

int ShaderManager::getShader(const char *name, Shader **result, char *errorString)
{
	for (int i = 0; i < numShaders; i++)
	{
		if (shader[i]->isShader(name))
		{
			*result = shader[i];
			return 0;
		}
	}

	// Got here without finding a texture, return error.
	sprintf_s(errorString, MAX_ERROR_LENGTH,
			  "Could not find shader '%s'", name);
	return -1;
}

int ShaderManager::saveProgress(const char *shaderName, char *errorText, Editor *editor)
{
	Shader *shader;
	int retVal = getShader(shaderName, &shader, errorText);
	if (retVal) return retVal;

	SYSTEMTIME sysTime;
	GetSystemTime(&sysTime);

	char filename[SM_MAX_FILENAME_LENGTH+1];
	sprintf_s(filename, SM_MAX_FILENAME_LENGTH, "%s%s.%d_%d_%d_%d_%d_%d",
		SM_PROGRESS_DIRECTORY, shaderName, sysTime.wYear, sysTime.wMonth,
		sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	FILE *fid;
	if (fopen_s(&fid, filename, "wb"))
	{
		sprintf_s(errorText, MAX_ERROR_LENGTH,
			      "Could not write to '%s'", filename);
		return -1;
	}
	fwrite(shader->getShaderText(), sizeof(char), strlen(shader->getShaderText()), fid);
	fclose(fid);

	return 0;
}