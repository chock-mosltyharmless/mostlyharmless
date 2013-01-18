#ifndef _GLTOOLS_H_
#define _GLTOOLS_H_
#pragma once

#define NUM_GL_NAMES 14

#define glCreateShader ((PFNGLCREATESHADERPROC)glFP[0])
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)glFP[1])
#define glShaderSource ((PFNGLSHADERSOURCEPROC)glFP[2])
#define glCompileShader ((PFNGLCOMPILESHADERPROC)glFP[3])
#define glAttachShader ((PFNGLATTACHSHADERPROC)glFP[4])
#define glLinkProgram ((PFNGLLINKPROGRAMPROC)glFP[5])
#define glUseProgram ((PFNGLUSEPROGRAMPROC)glFP[6])
#define glTexImage3D ((PFNGLTEXIMAGE3DPROC)glFP[7])
#define glGetShaderiv ((PFNGLGETSHADERIVPROC)glFP[8])
#define glGetShaderInfoLog ((PFNGLGETSHADERINFOLOGPROC)glFP[9])
#define glGenBuffers ((PFNGLGENBUFFERSPROC)glFP[10])
#define glBindBuffer ((PFNGLBINDBUFFERPROC)glFP[11])
#define glBufferData ((PFNGLBUFFERDATAPROC)glFP[12])
#define glDeleteBuffers ((PFNGLDELETEBUFFERSPROC)glFP[13])

typedef void (*GenFP)(void); // pointer to openGL functions
extern GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions

extern HWND hWnd;

void initGLTools();

bool createShaderProgram(const char *vertexShaderFile,
						 const char *geometryShaderFile,
						 const char *fragmentShaderFile,
						 GLuint *shaderProgram);

#endif