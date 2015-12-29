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

#define wglCreateContextAttribsARB ((PFNWGLCREATECONTEXTATTRIBSARB)glFP[0])
#define glCreateShader ((PFNGLCREATESHADERPROC)glFP[1])
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)glFP[2])
#define glShaderSource ((PFNGLSHADERSOURCEPROC)glFP[3])
#define glCompileShader ((PFNGLCOMPILESHADERPROC)glFP[4])
#define glAttachShader ((PFNGLATTACHSHADERPROC)glFP[5])
#define glLinkProgram ((PFNGLLINKPROGRAMPROC)glFP[6])
#define glUseProgram ((PFNGLUSEPROGRAMPROC)glFP[7])
#define glGenVertexArrays ((PFNGLGENVERTEXARRAYSPROC)glFP[8])
#define glBindVertexArray ((PFNGLBINDVERTEXARRAYPROC)glFP[9])
#define glGenBuffers ((PFNGLGENBUFFERSPROC)glFP[10])
#define glBindBuffer ((PFNGLBINDBUFFERPROC)glFP[11])
#define glBufferData ((PFNGLBUFFERDATAPROC)glFP[12])
#define glVertexAttribPointer ((PFNGLVERTEXATTRIBPOINTERPROC)glFP[13])
#define glBindAttribLocation ((PFNGLBINDATTRIBLOCATIONPROC)glFP[14])
#define glEnableVertexAttribArray ((PFNGLENABLEVERTEXATTRIBARRAYPROC)glFP[15])
#ifdef SHADER_DEBUG
#define glGetShaderiv ((PFNGLGETSHADERIVPROC)glFP[16])
#define glGetShaderInfoLog ((PFNGLGETSHADERINFOLOGPROC)glFP[17])
#define glGetProgramiv ((PFNGLGETPROGRAMIVPROC)glFP[18])
#endif

#endif
