//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include "config.h"

#ifndef _INTRO_H_
#define _INTRO_H_

void intro_init( void );
void intro_do( long time );
void intro_end( void );

extern HWND hWnd;

// OpenGL functions stuff
typedef HGLRC (APIENTRY *PFNWGLCREATECONTEXTATTRIBSARB)(HDC hdc,
													    HGLRC hshareContext,
														const int *attribList);
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define GL_MAJOR_VERSION 0x821B
#define GL_MINOR_VERSION 0x821C

#ifdef SHADER_DEBUG
#define NUM_GL_NAMES 19
#else
#define NUM_GL_NAMES 16
#endif

typedef void (*GenFP)(void); // pointer to openGL functions
extern GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions

//#define wglCreateContextAttribsARB ((PFNWGLCREATECONTEXTATTRIBSARB)glFP[0])
#define glCreateShader ((PFNGLCREATESHADERPROC)glFP[0])
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)glFP[1])
#define glShaderSource ((PFNGLSHADERSOURCEPROC)glFP[2])
#define glCompileShader ((PFNGLCOMPILESHADERPROC)glFP[3])
#define glAttachShader ((PFNGLATTACHSHADERPROC)glFP[4])
#define glLinkProgram ((PFNGLLINKPROGRAMPROC)glFP[5])
#define glUseProgram ((PFNGLUSEPROGRAMPROC)glFP[6])
#define glGenVertexArrays ((PFNGLGENVERTEXARRAYSPROC)glFP[7])
#define glBindVertexArray ((PFNGLBINDVERTEXARRAYPROC)glFP[8])
#define glGenBuffers ((PFNGLGENBUFFERSPROC)glFP[9])
#define glBindBuffer ((PFNGLBINDBUFFERPROC)glFP[10])
#define glBufferData ((PFNGLBUFFERDATAPROC)glFP[11])
#define glVertexAttribPointer ((PFNGLVERTEXATTRIBPOINTERPROC)glFP[12])
#define glEnableVertexAttribArray ((PFNGLENABLEVERTEXATTRIBARRAYPROC)glFP[13])
#define glUniformMatrix4fv ((PFNGLUNIFORMMATRIX4FVPROC)glFP[14])
#define glGetUniformLocation ((PFNGLGETUNIFORMLOCATIONPROC)glFP[15])
#ifdef SHADER_DEBUG
#define glGetShaderiv ((PFNGLGETSHADERIVPROC)glFP[17])
#define glGetShaderInfoLog ((PFNGLGETSHADERINFOLOGPROC)glFP[18])
#define glGetProgramiv ((PFNGLGETPROGRAMIVPROC)glFP[19])
#endif

#endif
