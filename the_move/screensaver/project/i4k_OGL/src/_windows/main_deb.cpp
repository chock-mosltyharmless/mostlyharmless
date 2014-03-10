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
#include "bass.h"
#include "../config.h"
#include "../intro.h"

//----------------------------------------------------------------------------

int realXRes;
int realYRes;

bool isScreenSaverRunning = true;
int screenSaverStartTime = 0;
int screenSaverID = 0;
bool isAlarmRinging = false;
int alarmStartTime = 0;
int demoStartTime = 0;
int itemDeleteStartTime = (1<<28);
bool isEndScene = false;
int endSceneStartTime = 0;

bool isFlickering;
int flickerStartTime;

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
    24,
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

//==============================================================================================


static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	RECT rec;
	GetClientRect(hWnd, &rec);

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
		switch (wParam)
		{
		case VK_ESCAPE:
			PostQuitMessage(0);
			return 0;

#if 0
		case 'y':
		case 'Y':
		case VK_F1:
			compileShaders();
			break;
#endif

		case 'l':
		case 'L':
			isEndScene = !isEndScene;
			endSceneStartTime = timeGetTime();
			if (isEndScene)
			{
				PlaySound("sounds/swoosh.wav", NULL, SND_FILENAME | SND_ASYNC);
			}
			else
			{
				PlaySound(NULL, NULL, 0);
			}
			isFlickering = false;
			break;

		case 'q':
		case 'Q':
			isScreenSaverRunning = false;
			//ShowCursor(true);
			PlaySound(NULL, NULL, 0);
			break;

		case 'm':
		case 'M':
			isAlarmRinging = true;
			alarmStartTime = timeGetTime();
			PlaySound("sounds/alarm.wav", NULL, SND_FILENAME | SND_ASYNC);
			break;

		case 'b':
		case 'B':
			intro_blackout(true);
			break;

		case 'v':
		case 'V':
			intro_blackout(false);
			break;

		case 'x':
		case 'X':
			isFlickering = true;
			flickerStartTime = timeGetTime();
			break;

		case 'z':
		case 'Z':
			isFlickering = false;
			break;
			
		case 'd':
		case 'D':
			itemDeleteStartTime = timeGetTime();
			break;

		case 's':
		case 'S':
			itemDeleteStartTime = timeGetTime() + 1000 * 1000;
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
			itemDeleteStartTime = timeGetTime() + 1000 * 1000;
			//ShowCursor(false);
			isScreenSaverRunning = true;
			screenSaverStartTime = timeGetTime();
			screenSaverID = wParam - '1';
			if (screenSaverID >= 0)
			{
				if (screenSaverID < 5)
				{
					PlaySound("sounds/kaze_no_kaori.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
				}
				else
				{
					PlaySound("sounds/kaze_no_kaori.wav", NULL, SND_FILENAME | SND_ASYNC);
				}
			}
			else
			{
				PlaySound(NULL, NULL, 0);
			}
			break;

		default:
			break;
		}
		return 1;
    }

	if ( uMsg==WM_LBUTTONDOWN)
	{
		int xPos = lParam & 0xffff;
		int yPos = lParam >> 16;
		float relXPos = (float)xPos / float(rec.right - rec.left);
		float relYPos = (float)yPos / float(rec.bottom - rec.top);
		intro_left_click(relXPos, relYPos, timeGetTime(), wParam & MK_CONTROL);
		return 1;
	}

	if ( uMsg==WM_RBUTTONDOWN)
	{
		int xPos = lParam & 0xffff;
		int yPos = lParam >> 16;
		float relXPos = (float)xPos / float(rec.right - rec.left);
		float relYPos = (float)yPos / float(rec.bottom - rec.top);
		intro_right_click(relXPos, relYPos, timeGetTime(), wParam & MK_CONTROL);
		return 1;
	}

	if ( uMsg==WM_MOUSEMOVE )
	{
		int xPos = lParam & 0xfff;
		int yPos = lParam >> 16;
		float relXPos = (float)xPos / float(rec.right - rec.left);
		float relYPos = (float)yPos / float(rec.bottom - rec.top);
		intro_cursor(relXPos, relYPos);
		return 1;
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
		//ShowCursor( 1 );
    }
}

static int window_init( WININFO *info )
{
	unsigned int	PixelFormat;
    DWORD			dwExStyle, dwStyle;
//    DEVMODE			dmScreenSettings;
    RECT			rec;

    WNDCLASS		wc;

    ZeroMemory( &wc, sizeof(WNDCLASS) );
    wc.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = info->hInstance;
    wc.lpszClassName = info->wndclass;
	
    if( !RegisterClass(&wc) )
        return( 0 );

	rec.left   = 0;
	rec.top    = 0;
	rec.right  = XRES;
	rec.bottom = YRES;

    if( info->full )
    {
        //dmScreenSettings.dmSize       = sizeof(DEVMODE);
        //dmScreenSettings.dmFields     = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
        //dmScreenSettings.dmBitsPerPel = 24;
        //dmScreenSettings.dmPelsWidth  = XRES;
        //dmScreenSettings.dmPelsHeight = YRES;
        //if( ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
        //    return( 0 );
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle   = WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_MAXIMIZE;// | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		//ShowCursor( 0 );
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		//dwExStyle = WS_EX_APPWINDOW;
        dwStyle   = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
		//dwStyle   = WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;

		realXRes = XRES;
		realYRes = YRES;
		//ShowCursor(0);
		AdjustWindowRect( &rec, dwStyle, 0 );
    }

    info->hWnd = CreateWindowEx( dwExStyle, wc.lpszClassName, "avada kedabra!", dwStyle,
                               (GetSystemMetrics(SM_CXSCREEN)-rec.right+rec.left)>>1,
                               (GetSystemMetrics(SM_CYSCREEN)-rec.bottom+rec.top)>>1,
                               rec.right-rec.left, rec.bottom-rec.top, 0, 0, info->hInstance, 0 );
	if (info->full)
	{
		GetWindowRect(info->hWnd, &rec);
		realXRes = rec.right - rec.left;
		realYRes = rec.bottom - rec.top;
	}

	if( !info->hWnd )
        return( 0 );
	hWnd = info->hWnd;

    if( !(info->hDC=GetDC(info->hWnd)) )
        return( 0 );

    if( !(PixelFormat=ChoosePixelFormat(info->hDC,&pfd)) )
        return( 0 );

    if( !SetPixelFormat(info->hDC,PixelFormat,&pfd) )
        return( 0 );

    if( !(info->hRC=wglCreateContext(info->hDC)) )
        return( 0 );

    if( !wglMakeCurrent(info->hDC,info->hRC) )
        return( 0 );
    
    return( 1 );
}

int WINAPI WinMain( HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    MSG         msg;
    int         done=0;
    WININFO     *info = &wininfo;

	// music data
	HSTREAM mp3Str = 0;

    info->hInstance = GetModuleHandle( 0 );

    //if( MessageBox( 0, "fullscreen?", info->wndclass, MB_YESNO|MB_ICONQUESTION)==IDYES ) info->full++;

    if( !window_init(info) )
    {
        window_end( info );
        MessageBox( 0, "window_init()!","error",MB_OK|MB_ICONEXCLAMATION );
        return( 0 );
    }

    intro_init();

	// start music playback
	BASS_Init(-1,44100,0,info->hWnd,NULL);
	mp3Str=BASS_StreamCreateFile(FALSE,"ana.mp3",0,0,0);
	//BASS_ChannelPlay(mp3Str, TRUE);
	//BASS_Start();

    //long to=timeGetTime();
	isScreenSaverRunning = false;
	ShowCursor(false);
	screenSaverStartTime = timeGetTime();
	demoStartTime = timeGetTime();
    while( !done )
        {
		long t = timeGetTime();// - to;

        while( PeekMessage(&msg,0,0,0,PM_REMOVE) )
        {
            if( msg.message==WM_QUIT ) done=1;
		    TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        intro_do( t );

#if 0
		if( t>(MZK_DURATION*1000) )
		{
			done = 1;
		}
#endif

        SwapBuffers( info->hDC );
        }

	// music uninit
	BASS_ChannelStop(mp3Str);
	BASS_StreamFree(mp3Str);
	BASS_Free();

    window_end( info );

    return( 0 );
}



