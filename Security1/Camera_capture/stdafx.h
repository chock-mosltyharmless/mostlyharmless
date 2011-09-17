// stdafx.h : Includedatei für Standardsystem-Includedateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Teile der Windows-Header nicht einbinden.
// Windows-Headerdateien:
#include <windows.h>

// C RunTime-Headerdateien
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>

#include <dshow.h>
#include <Dvdmedia.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"
#include "qedit.h"

// Opengl stuff (This is a mess, the rest is in Security1.cpp
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
#define glActiveTexture ((PFNGLACTIVETEXTUREPROC)glFP[10])
#define glGetUniformLocation ((PFNGLGETUNIFORMLOCATIONPROC)glFP[11])
#define glUniform1i ((PFNGLUNIFORM1IPROC)glFP[12])
#define glMultiTexCoord2f ((PFNGLMULTITEXCOORD2FPROC)glFP[13])
typedef void (*GenFP)(void); // pointer to openGL functions
extern GenFP glFP[NUM_GL_NAMES];

// Global definitions that work for everything...
#define ASPECT_RATIO (2.0f * 640.0f / 480.0f)

// TODO: Hier auf zusätzliche Header, die das Programm erfordert, verweisen.
