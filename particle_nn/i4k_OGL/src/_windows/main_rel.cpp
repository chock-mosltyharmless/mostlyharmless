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
	 "glEnableVertexAttribArray",
	 "glBufferSubData",
	 "glUniformMatrix4fv",
	 "glGetUniformLocation",
	 "glGetShaderiv","glGetShaderInfoLog", "glGetProgramiv"
};
#else
const static char* glnames[NUM_GL_NAMES]={
	 "wglCreateContextAttribsARB",
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glGenVertexArrays", "glBindVertexArray", "glGenBuffers",
	 "glBindBuffer", "glBufferData", "glVertexAttribPointer",
	 "glEnableVertexAttribArray",
	 "glBufferSubData",
	 "glUniformMatrix4fv",
	 "glGetUniformLocation",
};
#endif

// Audio playback stuff
static HWAVEOUT hWaveOut; // audio device handle
// TODO: Use more than 2 buffers with very small sizes so that I have
//       no influence on frame rate!
static MMTIME timer; // Using getPosition of wave audio playback
static int nextPlayBlock = 0; // The block that must be filled and played next
short myMuzikBlock[2][AUDIO_BUFFER_SIZE*MZK_NUMCHANNELS]; // The audio blocks
static WAVEHDR header[2];    // header of the audio block
static const WAVEFORMATEX wfx = {
	WAVE_FORMAT_PCM,					// wFormatTag
	MZK_NUMCHANNELS,					// nChannels
	MZK_RATE,							// nSamplesPerSec
	MZK_RATE*MZK_NUMCHANNELS*2,			// nAvgBytesPerSec
	MZK_NUMCHANNELS*2,					// nBlockAlign
	16,									// wBitsPerSample
	0									// cbSize
};

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

	// open audio device
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    // create music block
	mzk_init();
	// prepare and play music block
	header[0].lpData = (char *)myMuzikBlock[0];
	header[1].lpData = (char *)myMuzikBlock[1];
	header[0].dwBufferLength = AUDIO_BUFFER_SIZE * MZK_NUMCHANNELS * 2;
	header[1].dwBufferLength = AUDIO_BUFFER_SIZE * MZK_NUMCHANNELS * 2;
	waveOutPrepareHeader(hWaveOut, &(header[0]), sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &(header[0]), sizeof(WAVEHDR));
	waveOutPrepareHeader(hWaveOut, &(header[1]), sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &(header[1]), sizeof(WAVEHDR));

	timer.wType = TIME_SAMPLES;
    do 
	{
		MSG msg;
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

		ShowCursor(0);
		waveOutGetPosition(hWaveOut, &timer, sizeof(timer));
		DWORD t = timer.u.sample;

        intro_do(t);
        //SwapBuffers ( hDC );   
        wglSwapLayerBuffers( hDC, WGL_SWAP_MAIN_PLANE );

		// Try to unprepare header
		if (waveOutUnprepareHeader(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR))
			!= WAVERR_STILLPLAYING)
		{
			mzk_prepare_block(myMuzikBlock[nextPlayBlock]);
			waveOutPrepareHeader(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR));
			nextPlayBlock = 1 - nextPlayBlock;
		}
	} while ( !(GetAsyncKeyState(VK_ESCAPE) || GetAsyncKeyState(VK_F4)));

    sndPlaySound(0,0);

    ExitProcess(0);
}
