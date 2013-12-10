//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>
#include "../intro.h"
#include "../mzk.h"
#include "../config.h"

extern "C" int _fltused = 0;

static const int wavHeader[11] = {
    0x46464952, 
    MZK_NUMSAMPLESC*sizeof(short)+36, 
    0x45564157, 
    0x20746D66, 
    16, 
    WAVE_FORMAT_PCM|(MZK_NUMCHANNELS<<16), 
    MZK_RATE, 
    MZK_RATE*MZK_NUMCHANNELS*sizeof(short), 
    (MZK_NUMCHANNELS*sizeof(short))|((8*sizeof(short))<<16),
    0x61746164, 
    MZK_NUMSAMPLESC*sizeof(short)
    };

static const PIXELFORMATDESCRIPTOR pfd =
    {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0,
    PFD_MAIN_PLANE,
    0, 0, 0, 0
    };

#ifdef SETRESOLUTION
#pragma data_seg(".screenSettings")
static DEVMODE screenSettings = { {0},
    #if _MSC_VER < 1400
    0,0,148,0,0x001c0000,{0},0,0,0,0,0,0,0,0,0,{0},0,32,XRES,YRES,0,0,      // Visual C++ 6.0
    #else
    0,0,156,0,0x001c0000,{0},0,0,0,0,0,{0},0,32,XRES,YRES,{0}, 0,           // Visuatl Studio 2005
    #endif
    #if(WINVER >= 0x0400)
    0,0,0,0,0,0,
    #if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
    0,0
    #endif
    #endif
    };
#endif

// OpenGL function stuff
GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions

static int glAttribs[7] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
						   WGL_CONTEXT_MINOR_VERSION_ARB, 3,
						   WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			               NULL}; 

#ifdef SHADER_DEBUG
const static char* glnames[NUM_GL_NAMES]={
	 "wglCreateContextAttribsARB",
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glGenVertexArrays", "glBindVertexArray", "glGenBuffers",
	 "glBindBuffer", "glBufferData", "glVertexAttribPointer",
	 "glBindAttribLocation", "glEnableVertexAttribArray",
	 "glGetShaderiv","glGetShaderInfoLog", "glGetProgramiv"
};
#else
const static char* glnames[NUM_GL_NAMES]={
	 "wglCreateContextAttribsARB",
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glGenVertexArrays", "glBindVertexArray", "glGenBuffers",
	 "glBindBuffer", "glBufferData", "glVertexAttribPointer",
	 "glBindAttribLocation", "glEnableVertexAttribArray"
};
#endif

static short myMuzik[MZK_NUMSAMPLESC+22];

void entrypoint( void )
{              
    // full screen
    #ifdef SETRESOLUTION
    if( ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return;
    #endif	
    // create window
    HWND hWnd = CreateWindow( "static",0,WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,0,0);
    HDC hDC = GetDC(hWnd);
    
	// initalize opengl
    if( !SetPixelFormat(hDC,ChoosePixelFormat(hDC,&pfd),&pfd) ) return;

	HGLRC tempOpenGLContext;
    tempOpenGLContext = wglCreateContext(hDC);
	wglMakeCurrent(hDC, tempOpenGLContext);
	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);
	HGLRC hRC = wglCreateContextAttribsARB(hDC, NULL, glAttribs);
	// Remove temporary context and set new one
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(tempOpenGLContext);
	wglMakeCurrent(hDC, hRC);

	// init intro
	intro_init();

#ifdef USEDSOUND
	// calculate music
	mzk_init( myMuzik+22 );
	// and play it 
    memcpy( myMuzik, wavHeader, 44 );
    sndPlaySound( (const char*)&myMuzik, SND_ASYNC|SND_MEMORY );
#endif

    long t;
	long to = timeGetTime();
    do 
	{
		MSG msg;
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

		ShowCursor(0);
        t = timeGetTime(); 
        //if( !to ) to=t; 
        t = t-to;//-150;

        intro_do(t);
        //SwapBuffers ( hDC );   
        wglSwapLayerBuffers( hDC, WGL_SWAP_MAIN_PLANE );
	} while ( !(GetAsyncKeyState(VK_ESCAPE) || GetAsyncKeyState(VK_F4)) && t<(MZK_DURATION*1000) );

    sndPlaySound(0,0);

    ExitProcess(0);
}
