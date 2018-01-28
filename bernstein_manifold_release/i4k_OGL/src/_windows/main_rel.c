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

#include "../intro.c"
#include "../mzk.c"

//extern "C" int _fltused = 0;
int _fltused = 0;

// Audio playback stuff
static HWAVEOUT hWaveOut; // audio device handle
//static MMTIME timer; // Using getPosition of wave audio playback
#define NUM_PLAY_BLOCKS 8
static int nextPlayBlock = 0; // The block that must be filled and played next
short myMuzikBlock[NUM_PLAY_BLOCKS][AUDIO_BUFFER_SIZE*MZK_NUMCHANNELS]; // The audio blocks
static WAVEHDR header[NUM_PLAY_BLOCKS];    // header of the audio block
static const WAVEFORMATEX wfx = {
    WAVE_FORMAT_PCM,					// wFormatTag
    MZK_NUMCHANNELS,					// nChannels
    MZK_RATE,							// nSamplesPerSec
    MZK_RATE*MZK_NUMCHANNELS*2,			// nAvgBytesPerSec
    MZK_NUMCHANNELS*2,					// nBlockAlign
    16,									// wBitsPerSample
    0									// cbSize
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

#if USE_GL_ATTRIBS
static int glAttribs[7] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
						   WGL_CONTEXT_MINOR_VERSION_ARB, 3,
						   WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			               0};
#endif

#ifdef SHADER_DEBUG
const static char* glnames[NUM_GL_NAMES]={
    "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
    "glAttachShader", "glLinkProgram", "glUseProgram",
    "glGenVertexArrays", "glBindVertexArray", "glGenBuffers",
    "glBindBuffer", "glBufferData", "glVertexAttribPointer",
    "glEnableVertexAttribArray",
    "glUniformMatrix4fv",
    "glGetShaderiv","glGetShaderInfoLog", "glGetProgramiv"
};
#else
#define MAX_GLNAME_LEN 27
const static char glnames[NUM_GL_NAMES][MAX_GLNAME_LEN]={
    "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
    "glAttachShader", "glLinkProgram", "glUseProgram",
    "glGenVertexArrays", "glBindVertexArray", "glGenBuffers",
    "glBindBuffer", "glBufferData", "glVertexAttribPointer",
    "glEnableVertexAttribArray",
    "glUniformMatrix4fv",
};
#endif

#pragma code_seg(".thread_func")
DWORD WINAPI thread_func(LPVOID lpParameter)
{
    while (1)
    {
        // Try to unprepare header
        if (waveOutUnprepareHeader(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR))
            != WAVERR_STILLPLAYING)
        {
#ifdef USEDSOUND
            mzk_prepare_block(myMuzikBlock[nextPlayBlock]);
#endif
            waveOutPrepareHeader(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR));
            waveOutWrite(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR));
            nextPlayBlock++;
            if (nextPlayBlock >= NUM_PLAY_BLOCKS) nextPlayBlock = 0;
        }
        else
        {
            //Sleep(1);
        }
    }

    return 0;
}

#pragma code_seg(".entrypoint")
void entrypoint( void )
{              
	int i;
	HGLRC hRC;
	long t, to, itime;

    // full screen
    #ifdef SETRESOLUTION
    if( ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return;
    #endif	
    // create window
    HWND hWnd = CreateWindow("static",0,WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,0,0);
    HDC hDC = GetDC(hWnd);
    
	// initalize opengl
    if( !SetPixelFormat(hDC,ChoosePixelFormat(hDC,&pfd),&pfd) ) return;

    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
    for (i = 0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);
    // create music block
    mzk_init();

	// init intro
	intro_init();

#ifdef USEDSOUND
    // open audio device
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

    // prepare and play music block
    for (int i = 0; i < NUM_PLAY_BLOCKS; i++)
    {
        header[i].lpData = (char *)myMuzikBlock[i];
        header[i].dwBufferLength = AUDIO_BUFFER_SIZE * MZK_NUMCHANNELS * 2;
        waveOutPrepareHeader(hWaveOut, &(header[i]), sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &(header[i]), sizeof(WAVEHDR));
    }

    CreateThread(NULL, 0, thread_func, NULL, 0, 0);
#endif

	to = timeGetTime();
    do 
	{
		MSG msg;
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

		ShowCursor(0);
        t = timeGetTime(); 
        //if( !to ) to=t; 
        t = t - to;//-150;
        itime = t * 441 / 10;

        intro_do(itime);
        //SwapBuffers ( hDC );   
        wglSwapLayerBuffers( hDC, WGL_SWAP_MAIN_PLANE );
	} while ( !(GetAsyncKeyState(VK_ESCAPE) || GetAsyncKeyState(VK_F4)) && itime<(MZK_DURATION*AUDIO_BUFFER_SIZE) );

    ExitProcess(0);
}
