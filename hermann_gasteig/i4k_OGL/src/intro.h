//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#ifndef _INTRO_H_
#define _INTRO_H_

void loadShaders(void);
void intro_init( void );
void intro_do( long time );
void intro_end( void );

extern HWND hWnd;

#define NUM_GL_NAMES 16
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
#define glMultiTexCoord2f ((PFNGLMULTITEXCOORD2FPROC)glFP[15])

typedef void (*GenFP)(void); // pointer to openGL functions
extern GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions


#endif
