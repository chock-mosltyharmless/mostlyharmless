// LiveCoding.cpp : Definiert den Einstiegspunkt f�r die Anwendung.
//

#include "stdafx.h"
#include "LiveCoding.h"
#include "Configuration.h"
#include "glext.h"
#include "GLNames.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "Parameter.h"

#define MAX_LOADSTRING 100

// Gloabal aspect ratio of everything
float aspect_ratio_ = (float)XRES / (float)YRES;

/*************************************************
 * GL Core variables
 *************************************************/
GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
	 "glDeleteProgram", "glDeleteShader",
	 "glActiveTexture", "glGetUniformLocation", "glUniform1i", "glUniform1f",
	 "glMultiTexCoord2f"
};

/*************************************************
 * The core managing units that hold the resources
 *************************************************/
ShaderManager shaderManager;
TextureManager textureManager;

/*************************************************
 * Window core variables
 *************************************************/
HINSTANCE hInst;								// Aktuelle Instanz
TCHAR szTitle[MAX_LOADSTRING];					// Titelleistentext
TCHAR szWindowClass[MAX_LOADSTRING];			// Klassenname des Hauptfensters
// The size of the window that we render to...
RECT windowRect;

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
							{'l','c','_',0}
                            };

/**************************************************
 * Parameters from the midi stuff
 ****************************************************/
// ---------------------------------------------------------------
//					Parameter interpolation stuff
// ------------------------------------------------------------
// I want to interpolate the new values from the old ones.
const int maxNumParameters = 25;
const static float defaultParameters[maxNumParameters] = 
{
    0.0f
};
static float interpolatedParameters[maxNumParameters];
const int NUM_KEYS = 127;
static int keyPressed[NUM_KEYS] = {0};

/*************************************************
 * OpenGL initialization
 *************************************************/
static int initGL(WININFO *winInfo)
{
	char errorString[MAX_ERROR_LENGTH + 1];

	// Create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// Create and initialize the shader manager
	if (shaderManager.init(errorString))
	{
		MessageBox(winInfo->hWnd, errorString, "Shader Manager Load", MB_OK);
		return -1;
	}

	// Create and initialize everything needed for texture Management
	if (textureManager.init(errorString))
	{
		MessageBox(winInfo->hWnd, errorString, "Texture Manager Load", MB_OK);
		return -1;
	}

	return 0;
}

/*************************************************
 * Windows callback
 *************************************************/
static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	// salvapantallas
	if( uMsg==WM_SYSCOMMAND && (wParam==SC_SCREENSAVE || wParam==SC_MONITORPOWER) )
		return( 0 );

	// boton x o pulsacion de escape
	//if( uMsg==WM_CLOSE || uMsg==WM_DESTROY || (uMsg==WM_KEYDOWN && wParam==VK_ESCAPE) )
	if( uMsg==WM_CLOSE || uMsg==WM_DESTROY )
		{
		PostQuitMessage(0);
        return( 0 );
		}

	// Reaction to command keys
    if( uMsg==WM_KEYDOWN )
    {
		switch (wParam)
		{
#if 0
		case VK_ESCAPE:
			PostQuitMessage(0);
			return 0;
#endif

		case 'm':
		case 'M':
			if (GetAsyncKeyState(VK_CONTROL) < 0)
			{
				// TODO: Minimization again.
				SetWindowLong(hWnd, GWL_STYLE, WS_POPUP|WS_VISIBLE);
				ShowWindow(hWnd, SW_MAXIMIZE);
				GetClientRect(hWnd, &windowRect);
				glViewport(0, 0, windowRect.right-windowRect.left, abs(windowRect.bottom - windowRect.top)); //NEW
				aspect_ratio_ = (float)(windowRect.right-windowRect.left) / (float)(abs(windowRect.bottom - windowRect.top));
				ShowCursor(false);
			}
			break;

		default:
			break;
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
	windowRect.left = 0;
	windowRect.top = 0;
	windowRect.right = XRES;
	windowRect.bottom = YRES;

    info->hWnd = CreateWindowEx( dwExStyle, wc.lpszClassName, "live coding", dwStyle,
                               (GetSystemMetrics(SM_CXSCREEN)-rec.right+rec.left)>>1,
                               (GetSystemMetrics(SM_CYSCREEN)-rec.bottom+rec.top)>>1,
                               rec.right-rec.left, rec.bottom-rec.top, 0, 0, info->hInstance, 0 );
    if( !info->hWnd )
        return( 0 );

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


void intro_do(long t, float music_loudness)
{
	char errorText[MAX_ERROR_LENGTH+1];
	float ftime = 0.001f*(float)t;
	GLuint textureID;

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	// Those are key-Press indicators. I only act on 0-to-1.
	for (int i = 0; i < maxNumParameters; i++)
	{
		interpolatedParameters[i] = 0.0f * interpolatedParameters[i] +
									1.0f * params.getParam(i, defaultParameters[i]);
	}
	// Update key press events.
	for (int i = 0; i < NUM_KEYS; i++)
	{
		if (params.getParam(i, 0.0) > 0.5f) keyPressed[i]++;
		else keyPressed[i] = 0;
	}

	// Set the program uniforms
	GLuint programID;
	shaderManager.getProgramID("empty.gprg", &programID, errorText);
	glUseProgram(programID);
	GLuint loc = glGetUniformLocation(programID, "aspect_ratio");
	glUniform1f(loc, aspect_ratio_);
	loc = glGetUniformLocation(programID, "time");
	glUniform1f(loc, (float)(t * 0.001f));
	loc = glGetUniformLocation(programID, "knob1");
	glUniform1f(loc, interpolatedParameters[14]);
	loc = glGetUniformLocation(programID, "knob2");
	glUniform1f(loc, interpolatedParameters[15]);
	loc = glGetUniformLocation(programID, "knob3");
	glUniform1f(loc, interpolatedParameters[16]);
	loc = glGetUniformLocation(programID, "knob4");
	glUniform1f(loc, interpolatedParameters[17]);
	loc = glGetUniformLocation(programID, "knob5");
	glUniform1f(loc, interpolatedParameters[18]);
	loc = glGetUniformLocation(programID, "knob6");
	glUniform1f(loc, interpolatedParameters[19]);
	loc = glGetUniformLocation(programID, "knob7");
	glUniform1f(loc, interpolatedParameters[20]);
	loc = glGetUniformLocation(programID, "knob8");
	glUniform1f(loc, interpolatedParameters[21]);
	loc = glGetUniformLocation(programID, "knob9");
	glUniform1f(loc, interpolatedParameters[22]);
	loc = glGetUniformLocation(programID, "slider1");
	glUniform1f(loc, interpolatedParameters[2]);
	loc = glGetUniformLocation(programID, "slider2");
	glUniform1f(loc, interpolatedParameters[3]);
	loc = glGetUniformLocation(programID, "slider3");
	glUniform1f(loc, interpolatedParameters[4]);
	loc = glGetUniformLocation(programID, "slider4");
	glUniform1f(loc, interpolatedParameters[5]);
	loc = glGetUniformLocation(programID, "slider5");
	glUniform1f(loc, interpolatedParameters[6]);
	loc = glGetUniformLocation(programID, "slider6");
	glUniform1f(loc, interpolatedParameters[8]);
	loc = glGetUniformLocation(programID, "slider7");
	glUniform1f(loc, interpolatedParameters[9]);
	loc = glGetUniformLocation(programID, "slider8");
	glUniform1f(loc, interpolatedParameters[12]);
	loc = glGetUniformLocation(programID, "slider9");
	glUniform1f(loc, interpolatedParameters[13]);

	// Set texture identifiers
	GLint texture_location;
	texture_location = glGetUniformLocation(programID, "Noise3DTexture");
	glUniform1i(texture_location, 0);
	texture_location = glGetUniformLocation(programID, "DepthSensorTexture");
	glUniform1i(texture_location, 1);
	texture_location = glGetUniformLocation(programID, "BGTexture");
	glUniform1i(texture_location, 2);

	// render to larger offscreen texture
	glActiveTexture(GL_TEXTURE2);
	textureManager.getTextureID("hermaniak.png", &textureID, errorText);
	glBindTexture(GL_TEXTURE_2D, textureID);
    // Only do if you want to use depth sensor texture
//	glActiveTexture(GL_TEXTURE1);
//	textureManager.getTextureID(TM_DEPTH_SENSOR_NAME, &textureID, errorText);
//	glBindTexture(GL_TEXTURE_2D, textureID);
	glActiveTexture(GL_TEXTURE0);
	textureManager.getTextureID(TM_NOISE3D_NAME, &textureID, errorText);
	glBindTexture(GL_TEXTURE_3D, textureID);

	if (false) {
		glViewport(0, 0, X_HIGHLIGHT, Y_HIGHLIGHT);
	} else {
		glViewport(0, 0, X_OFFSCREEN, Y_OFFSCREEN);
	}
    glColor4f(1.0f, 0.5f, 0.2f, 1.0f);
	glRectf(-1.0, -1.0, 1.0, 1.0);

	// Copy backbuffer to texture
	if (false) {
		textureManager.getTextureID(TM_HIGHLIGHT_NAME, &textureID, errorText);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_HIGHLIGHT, Y_HIGHLIGHT);
	} else {
		textureManager.getTextureID(TM_OFFSCREEN_NAME, &textureID, errorText);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN);
	}

	// Copy backbuffer to front (so far no improvement)
	int xres = windowRect.right - windowRect.left;
	int yres = windowRect.bottom - windowRect.top;
	glViewport(0, 0, xres, yres);
	if (false) {
		shaderManager.getProgramID("DitherTexture.gprg", &programID, errorText);
	} else {
		shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
	}
	glUseProgram(programID);
	loc = glGetUniformLocation(programID, "time");
	glUniform1f(loc, (float)(t * 0.001f));
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDisable(GL_BLEND);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(1.0f, 1.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(-1.0f, 1.0f);
	glEnd();
}

int WINAPI WinMain( HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    MSG         msg;
    int         done=0;
    WININFO     *info = &wininfo;

    info->hInstance = GetModuleHandle( 0 );

    //if( MessageBox( 0, "fullscreen?", info->wndclass, MB_YESNO|MB_ICONQUESTION)==IDYES ) info->full++;

    if (!window_init(info))
    {
        window_end(info);
        MessageBox(0, "window_init()!", "error", MB_OK|MB_ICONEXCLAMATION);
        return 0;
    }

	if (initGL(info))
	{
		return 0;
	}

    //intro_init();

	// start music playback
#ifdef MUSIC
	BASS_Init(-1,44100,0,info->hWnd,NULL);
	HSTREAM mp3Str=BASS_StreamCreateFile(FALSE,"Musik/set1.mp3",0,0,0);
	BASS_ChannelPlay(mp3Str, TRUE);
	BASS_Start();
#endif


    // Initialize Wave input
    HWAVEIN hWaveIn = 0;
    WAVEFORMATEX WaveFormat;
    WAVEHDR WaveHdr;
    MMRESULT MMResult;
    WaveFormat.wFormatTag=WAVE_FORMAT_PCM;     // simple, uncompressed format
    WaveFormat.nChannels=1;                    // 1=mono, 2=stereo
    WaveFormat.nSamplesPerSec=44100;
    WaveFormat.wBitsPerSample=16;
    WaveFormat.nBlockAlign=
        WaveFormat.nChannels*(WaveFormat.wBitsPerSample/8);
    WaveFormat.nAvgBytesPerSec=
        WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
    WaveFormat.cbSize=0;
    int num_samples = WaveFormat.nSamplesPerSec/20;
    WaveHdr.dwBufferLength =
         num_samples * (WaveFormat.wBitsPerSample/8) * WaveFormat.nChannels;
    WaveHdr.dwBytesRecorded = 0L;
    WaveHdr.dwUser = 0L;
    WaveHdr.dwFlags = 0L;
    WaveHdr.dwLoops = 0L;
    short *wave_data = new short[WaveHdr.dwBufferLength / sizeof(short)];
    WaveHdr.lpData = (char *)wave_data;
    // open device
    MMResult = waveInOpen(&hWaveIn,WAVE_MAPPER,&WaveFormat,0,0,CALLBACK_NULL);
    // prepare header
    MMResult = waveInPrepareHeader(hWaveIn, &WaveHdr, sizeof(WAVEHDR));
    // send buffer to Windows
    MMResult = waveInAddBuffer(hWaveIn, &WaveHdr, sizeof(WAVEHDR));
    // start input
    MMResult = waveInStart(hWaveIn);
    float music_loudness = 0.0f;

	// Initialize COM
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) exit(-1);

    long to=timeGetTime();
    while( !done )
        {
		long t = timeGetTime() - to;

        while( PeekMessage(&msg,0,0,0,PM_REMOVE) )
        {
            if( msg.message==WM_QUIT ) done=1;
		    TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

        // Get Music:
        if (waveInUnprepareHeader(hWaveIn, &WaveHdr, sizeof(WAVEHDR)) != WAVERR_STILLPLAYING) {
            double cumulative_square = 0.0f;
            for (int i = 0; i < num_samples; i++) {
                cumulative_square += (double)wave_data[i] * (double)wave_data[i];
            }
            if (cumulative_square < 1.0f) cumulative_square = 1.0f;
            music_loudness = (float)(log(cumulative_square));
            MMResult = waveInStop(hWaveIn);
            MMResult = waveInPrepareHeader(hWaveIn, &WaveHdr, sizeof(WAVEHDR));
            MMResult = waveInAddBuffer(hWaveIn, &WaveHdr, sizeof(WAVEHDR));
            MMResult = waveInStart(hWaveIn);
        }

        intro_do(t, music_loudness);

		SwapBuffers( info->hDC );
	}    

    params.SaveToClipboard();

    window_end( info );

#ifdef MUSIC
	// music uninit
	BASS_ChannelStop(mp3Str);
	BASS_StreamFree(mp3Str);
	BASS_Free();
#endif

	// Un-initialize COM
	CoUninitialize();

    return( 0 );
}

// Note that a key was pressed
void registerParameterChange(int keyID)
{
}