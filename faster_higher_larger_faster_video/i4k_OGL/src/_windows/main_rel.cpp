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

// Audio playback stuff
static HWAVEOUT hWaveOut; // audio device handle
// TODO: Use more than 2 buffers with very small sizes so that I have
//       no influence on frame rate!
//static MMTIME timer; // Using getPosition of wave audio playback
#define NUM_PLAY_BLOCKS 8
static int nextPlayBlock = 0; // The block that must be filled and played next
short myMuzikBlock[NUM_PLAY_BLOCKS][AUDIO_BUFFER_SIZE*MZK_NUMCHANNELS]; // The audio blocks
static WAVEHDR header[NUM_PLAY_BLOCKS];    // header of the audio block
#pragma data_seg(".wfx")
static const WAVEFORMATEX wfx = {
	WAVE_FORMAT_PCM,					// wFormatTag
	MZK_NUMCHANNELS,					// nChannels
	MZK_RATE,							// nSamplesPerSec
	MZK_RATE*MZK_NUMCHANNELS*2,			// nAvgBytesPerSec
	MZK_NUMCHANNELS*2,					// nBlockAlign
	16,									// wBitsPerSample
	0									// cbSize
};

#pragma data_seg(".pfd")
static const PIXELFORMATDESCRIPTOR pfd =
    {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32,
    0, 0, 0, 0, 0, 0, 8, 0,
    0, 0, 0, 0, 0,
    32, 0, 0,
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

#pragma code_seg(".thread_func")
DWORD WINAPI thread_func(LPVOID lpParameter)
{
	while (true)
	{
		// Try to unprepare header
		if (waveOutUnprepareHeader(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR))
			!= WAVERR_STILLPLAYING)
		{
#ifdef USEDSOUND
			mzkPlayBlock(myMuzikBlock[nextPlayBlock]);
#endif
			waveOutPrepareHeader(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR));
			nextPlayBlock++;
			if (nextPlayBlock >= NUM_PLAY_BLOCKS) nextPlayBlock = 0;
		}
		else
		{
			Sleep(1);
		}
	}
}

#pragma code_seg(".entrypoint")
void entrypoint( void )
{              
    // full screen
    #ifdef SETRESOLUTION
    if( ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return;
    ShowCursor( 0 );
    #endif
    // create window
    HWND hWnd = CreateWindow("static",0,WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,0,0);
    HDC hDC = GetDC(hWnd);
    // initalize opengl
    if( !SetPixelFormat(hDC,ChoosePixelFormat(hDC,&pfd),&pfd) ) return;
    HGLRC hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC,hRC);

	// init intro
	intro_init();

	// open audio device
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    // create music block
	mzk_init();
	// prepare and play music block
	for (int i = 0; i < NUM_PLAY_BLOCKS; i++)
	{
		header[i].lpData = (char *)myMuzikBlock[i];
		header[i].dwBufferLength = AUDIO_BUFFER_SIZE * MZK_NUMCHANNELS * 2;
		waveOutPrepareHeader(hWaveOut, &(header[i]), sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, &(header[i]), sizeof(WAVEHDR));
	}

	HANDLE thread = CreateThread(NULL, 0, thread_func, NULL, 0, 0);

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
    
        intro_do( t );
    
        //SwapBuffers ( hDC );   
        wglSwapLayerBuffers( hDC, WGL_SWAP_MAIN_PLANE );
	}while ( !GetAsyncKeyState(VK_ESCAPE) && t<(MZK_DURATION*MZK_BLOCK_SIZE/MZK_RATE*1000) );

    waveOutClose(hWaveOut);
    
	ExitProcess(0);
}

