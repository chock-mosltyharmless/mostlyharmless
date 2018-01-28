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

HWND hWnd;

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

// Audio playback stuff
static HWAVEOUT hWaveOut; // audio device handle
static MMTIME timer; // Using getPosition of wave audio playback
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

static int glAttribs[7] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
						   WGL_CONTEXT_MINOR_VERSION_ARB, 3,
						   WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			               0}; 

// OpenGL function stuff
GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions

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
const static char* glnames[NUM_GL_NAMES]={
    "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
    "glAttachShader", "glLinkProgram", "glUseProgram",
    "glGenBuffers",
    "glBindBuffer", "glBufferData", "glVertexAttribPointer",
    "glEnableVertexAttribArray",
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
	HGLRC tempOpenGLContext;
    WNDCLASS		wc;
	int i;
	int nMajorVersion = -1;
	int nMinorVersion = -1;

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

    if (!(tempOpenGLContext = wglCreateContext(info->hDC))) return 0;
	if (!wglMakeCurrent(info->hDC, tempOpenGLContext)) return 0;

	// create openGL functions
	for (i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);
#ifdef USE_GL_ATTRIBS
	if (!(info->hRC = wglCreateContextAttribsARB(info->hDC, NULL, glAttribs))) return 0;
#else
    if (!(info->hRC = wglCreateContext(info->hDC))) return 0;
#endif

	// Remove temporary context and set new one
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(tempOpenGLContext);
	if (!wglMakeCurrent(info->hDC, info->hRC)) return 0;

	// Test GL version
	glGetIntegerv(GL_MAJOR_VERSION, &nMajorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &nMinorVersion);
	//printf("Reported GL Version %d.%d [Supported]\n", nMajorVersion, nMinorVersion);

    return( 1 );
}


//==============================================================================================

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
            Sleep(1);
        }
    }

    return 0;
}


//==============================================================================================

int WINAPI WinMain( HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    MSG         msg;
    int         done=0;
    WININFO     *info = &wininfo;
	long to;

    info->hInstance = GetModuleHandle( 0 );

    //if( MessageBox( 0, "fullscreen?", info->wndclass, MB_YESNO|MB_ICONQUESTION)==IDYES ) info->full++;

    if( !window_init(info) )
    {
        window_end( info );
        MessageBox( 0, "window_init()!","error",MB_OK|MB_ICONEXCLAMATION );
        return( 0 );
    }

    intro_init();

#ifdef USEDSOUND
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
    for (int i = 0; i < NUM_PLAY_BLOCKS; i++)
    {
        header[i].lpData = (char *)myMuzikBlock[i];
        header[i].dwBufferLength = AUDIO_BUFFER_SIZE * MZK_NUMCHANNELS * 2;
        waveOutPrepareHeader(hWaveOut, &(header[i]), sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &(header[i]), sizeof(WAVEHDR));
    }

    CreateThread(NULL, 0, thread_func, NULL, 0, 0);
#endif

    to=timeGetTime();
    while( !done )
        {
		long t = timeGetTime() - to;
        long itime = t * 441 / 10;

        while( PeekMessage(&msg,0,0,0,PM_REMOVE) )
        {
            if( msg.message==WM_QUIT ) done=1;
		    TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        intro_do( itime );

        if( itime > (MZK_DURATION*AUDIO_BUFFER_SIZE) )
		{
			done = 1;
		}
        SwapBuffers( info->hDC );
        }

    window_end( info );

    return( 0 );
}



