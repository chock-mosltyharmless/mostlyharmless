#pragma once

typedef void (*GenFP)(void);
extern GenFP glFP[];

// Stuff that I need for opengl
#define NUM_GL_NAMES 17
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
#define glDeleteProgram ((PFNGLDELETEPROGRAMPROC)glFP[10])
#define glDeleteShader ((PFNGLDELETESHADERPROC)glFP[11])
#define glActiveTexture ((PFNGLACTIVETEXTUREPROC)glFP[12])
#define glGetUniformLocation ((PFNGLGETUNIFORMLOCATIONPROC)glFP[13])
#define glUniform1i ((PFNGLUNIFORM1IPROC)glFP[14])
#define glUniform1f ((PFNGLUNIFORM1FPROC)glFP[15])
#define glMultiTexCoord2f ((PFNGLMULTITEXCOORD2FPROC)glFP[16])