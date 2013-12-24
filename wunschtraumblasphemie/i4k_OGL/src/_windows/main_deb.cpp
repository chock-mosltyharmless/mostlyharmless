//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "../config.h"
#include "../intro.h"
#include "../mzk.h"

//----------------------------------------------------------------------------

typedef struct
{
    //---------------
    HINSTANCE   hInstance;
    HDC         hDC;
    HGLRC       hRC;
    HWND        hWnd;
    //---------------
    int         full;
    //---------------
    char        wndclass[4];	// window class and title :)
    //---------------
}WININFO;



static const PIXELFORMATDESCRIPTOR pfd =
    {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,  // accum
    0,             // zbuffer
    0,              // stencil!
    0,              // aux
    PFD_MAIN_PLANE,
    0, 0, 0, 0
    };

static WININFO wininfo = {  0,0,0,0,0,
							{'i','q','_',0}
                            };

static int glAttribs[7] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
						   WGL_CONTEXT_MINOR_VERSION_ARB, 3,
						   WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			               NULL}; 

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

// OpenGL function stuff
GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions

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
};
#endif

//==============================================================================================


static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	// salvapantallas
	if( uMsg==WM_SYSCOMMAND && (wParam==SC_SCREENSAVE || wParam==SC_MONITORPOWER) )
		return( 0 );

	// boton x o pulsacion de escape
	if( uMsg==WM_CLOSE || uMsg==WM_DESTROY || (uMsg==WM_KEYDOWN && wParam==VK_ESCAPE) )
		{
		PostQuitMessage(0);
        return( 0 );
		}

    if( uMsg==WM_CHAR )
        {
        if( wParam==VK_ESCAPE )
            {
            PostQuitMessage(0);
            return( 0 );
            }
        }

    return( DefWindowProc(hWnd,uMsg,wParam,lParam) );
}

static void window_end( WININFO *info )
{
    if( info->hRC )
    {
        wglMakeCurrent( 0, 0 );
        wglDeleteContext( info->hRC );
    }

    if( info->hDC  ) ReleaseDC( info->hWnd, info->hDC );
    if( info->hWnd ) DestroyWindow( info->hWnd );

    UnregisterClass( info->wndclass, info->hInstance );

    if( info->full )
    {
        ChangeDisplaySettings( 0, 0 );
		ShowCursor( 1 );
    }
}

static int window_init( WININFO *info )
{
	unsigned int	PixelFormat;
    DWORD			dwExStyle, dwStyle;
    DEVMODE			dmScreenSettings;
    RECT			rec;

    WNDCLASS		wc;

    ZeroMemory( &wc, sizeof(WNDCLASS) );
    wc.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = info->hInstance;
    wc.lpszClassName = info->wndclass;
	
    if( !RegisterClass(&wc) )
        return( 0 );

    if( info->full )
    {
        dmScreenSettings.dmSize       = sizeof(DEVMODE);
        dmScreenSettings.dmFields     = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
        dmScreenSettings.dmBitsPerPel = 24;
        dmScreenSettings.dmPelsWidth  = XRES;
        dmScreenSettings.dmPelsHeight = YRES;
        if( ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
            return( 0 );
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle   = WS_VISIBLE | WS_POPUP;// | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		ShowCursor( 0 );
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle   = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
    }

    rec.left   = 0;
    rec.top    = 0;
    rec.right  = XRES;
    rec.bottom = YRES;
    AdjustWindowRect( &rec, dwStyle, 0 );

    info->hWnd = CreateWindowEx( dwExStyle, wc.lpszClassName, "avada kedabra!", dwStyle,
                               (GetSystemMetrics(SM_CXSCREEN)-rec.right+rec.left)>>1,
                               (GetSystemMetrics(SM_CYSCREEN)-rec.bottom+rec.top)>>1,
                               rec.right-rec.left, rec.bottom-rec.top, 0, 0, info->hInstance, 0 );
    if(!info->hWnd) return 0;
	hWnd = info->hWnd;

    if(!(info->hDC = GetDC(info->hWnd))) return 0;
    if(!(PixelFormat = ChoosePixelFormat(info->hDC, &pfd))) return 0;
    if(!SetPixelFormat(info->hDC, PixelFormat, &pfd)) return 0;

	HGLRC tempOpenGLContext;
    if (!(tempOpenGLContext = wglCreateContext(info->hDC))) return 0;
	if (!wglMakeCurrent(info->hDC, tempOpenGLContext)) return 0;

	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);
	if (!(info->hRC = wglCreateContextAttribsARB(info->hDC, NULL, glAttribs))) return 0;
	
	// Remove temporary context and set new one
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(tempOpenGLContext);
	if (!wglMakeCurrent(info->hDC, info->hRC)) return 0;

	// Test GL version
	int nMajorVersion = -1;
	int nMinorVersion = -1;
	glGetIntegerv(GL_MAJOR_VERSION, &nMajorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &nMinorVersion);
	//printf("Reported GL Version %d.%d [Supported]\n", nMajorVersion, nMinorVersion);

    return( 1 );
}


//==============================================================================================

int WINAPI WinMain( HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    MSG         msg;
    int         done=0;
    WININFO     *info = &wininfo;

    info->hInstance = GetModuleHandle( 0 );

    //if( MessageBox( 0, "fullscreen?", info->wndclass, MB_YESNO|MB_ICONQUESTION)==IDYES ) info->full++;

    if (!window_init(info))
    {
        window_end( info );
        MessageBox( 0, "window_init()!","error",MB_OK|MB_ICONEXCLAMATION );
        return( 0 );
    }

#ifdef USEDGRAPHICS
    intro_init();
#endif

	// open audio device
	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 
					0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
	{
		MessageBox(0, "unable to open WAVE_MAPPER device", "error", MB_OK|MB_ICONEXCLAMATION);
		return 0;
	}

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
    while (!done)
    {
		waveOutGetPosition(hWaveOut, &timer, sizeof(timer));
		DWORD t = timer.u.sample;

        while (PeekMessage(&msg,0,0,0,PM_REMOVE))
        {
            if (msg.message==WM_QUIT) done=1;
		    TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

#ifdef USEDGRAPHICS
        intro_do(t);
#endif

#if 0
        if( t > (MZK_DURATION*1000) )
		{
			done = 1;
		}
#endif
        
		SwapBuffers(info->hDC);

		// Try to unprepare header
		if (waveOutUnprepareHeader(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR))
			!= WAVERR_STILLPLAYING)
		{
#ifdef USEDSOUND
			mzk_prepare_block(myMuzikBlock[nextPlayBlock]);
#endif
			waveOutPrepareHeader(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, &(header[nextPlayBlock]), sizeof(WAVEHDR));
			nextPlayBlock = 1 - nextPlayBlock;
		}
    }

    // Close the wave output (for savety?)
	waveOutClose(hWaveOut);
    window_end( info );

    return( 0 );
}



