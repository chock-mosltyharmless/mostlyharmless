// LiveCoding.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "LiveCoding.h"
#include "Configuration.h"
#include "glext.h"
#include "wglext.h"
#include "GLNames.h"
#include "ShaderManager.h"
#include "TextureManager.h"
//#include "FluidSimulation.h"
#include "Editor.h"
#include "Parameter.h"
#include "bass.h"

#define MAX_LOADSTRING 100
#define BLOB_FADE_SPEED 0.01f

// The used effect (will be changeable later on)
#define NUM_USED_PROGRAMS 10
char *usedShader[NUM_USED_PROGRAMS] = {"empty.jlsl", "vp1.jlsl", "vp2.jlsl", "vp3.jlsl", "vp4.jlsl", "vp5.jlsl", "vp6.jlsl", "vp7.jlsl", "vp8.jlsl", "vp9.jlsl"};
char *usedProgram[NUM_USED_PROGRAMS] = {"empty.gprg", "vp1.gprg", "vp2.gprg", "vp3.gprg", "vp4.gprg", "vp5.gprg", "vp6.gprg", "vp7.gprg", "vp8.gprg", "vp9.gprg"};
int usedIndex = 0;
float aspectRatio = (float)XRES / (float)YRES;

long start_time_ = 0;

//FluidSimulation fluid_simulation_;
HSTREAM mp3Str;

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
Editor editor;

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
	-1.0f, -1.0f,
	0.0f, 0.0f,	0.0f, 0.0f, 0.0f,	// 2-6 ~= 1-5
	-1.0f,
	0.0f, 0.0f,						// 8,9 ~= 6,7
	-1.0f, -1.0f,
	0.0f, 0.0f,						// 12,13 ~= 8-9
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,			// 14-22 ~= 1b-9b
};
float interpolatedParameters[maxNumParameters];
const int NUM_KEYS = 127;
static int keyPressed[NUM_KEYS] = {0};

// BPM stuff
const int NUM_BEAT_TIMES = 7;
float BPM = 0.0f;
int beatDurations[NUM_BEAT_TIMES] = {900, 1200, 1100, 1000, 1400, 1000, 1000};
int lastBeatTime = 0;
float blob = 0.;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void window_end(WININFO *info)
{
    if (info->hRC)
    {
        wglMakeCurrent(0, 0);
        wglDeleteContext(info->hRC);
    }

    if (info->hDC) ReleaseDC(info->hWnd, info->hDC);
    if (info->hWnd) DestroyWindow(info->hWnd);

    UnregisterClass(info->wndclass, info->hInstance);

    if (info->full)
    {
        ChangeDisplaySettings(0, 0);
        ShowCursor(1);
    }
}


static int window_init(WININFO *info, bool use_custom_pixel_format = false, int custom_pixel_format = 0)
{
    unsigned int	PixelFormat;
    DWORD			dwExStyle, dwStyle;
    DEVMODE			dmScreenSettings;
    RECT			rec;

    WNDCLASS		wc;

    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = info->hInstance;
    wc.lpszClassName = info->wndclass;

    if (!RegisterClass(&wc))
        return(0);

    if (info->full)
    {
        dmScreenSettings.dmSize = sizeof(DEVMODE);
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        dmScreenSettings.dmBitsPerPel = 24;
        dmScreenSettings.dmPelsWidth = XRES;
        dmScreenSettings.dmPelsHeight = YRES;
        if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
            return(0);
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle = WS_VISIBLE | WS_POPUP;// | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        ShowCursor(0);
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
    }

    rec.left = 0;
    rec.top = 0;
    rec.right = XRES;
    rec.bottom = YRES;
    AdjustWindowRect(&rec, dwStyle, 0);
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = XRES;
    windowRect.bottom = YRES;

    info->hWnd = CreateWindowEx(dwExStyle, wc.lpszClassName, "live coding", dwStyle,
        (GetSystemMetrics(SM_CXSCREEN) - rec.right + rec.left) >> 1,
        (GetSystemMetrics(SM_CYSCREEN) - rec.bottom + rec.top) >> 1,
        rec.right - rec.left, rec.bottom - rec.top, 0, 0, info->hInstance, 0);
    if (!info->hWnd)
        return(0);

    if (!(info->hDC = GetDC(info->hWnd)))
        return(0);

    if (!use_custom_pixel_format) {
        if (!(PixelFormat = ChoosePixelFormat(info->hDC, &pfd)))
            return(0);

        if (!SetPixelFormat(info->hDC, PixelFormat, &pfd))
            return(0);
    } else {
        if (!SetPixelFormat(info->hDC, custom_pixel_format, &pfd))
            return(0);
    }

    if (!(info->hRC = wglCreateContext(info->hDC)))
        return(0);

    if (!wglMakeCurrent(info->hDC, info->hRC))
        return(0);

    return(1);
}

/*************************************************
 * OpenGL initialization
 *************************************************/
static int initGL(WININFO *winInfo)
{
	char errorString[MAX_ERROR_LENGTH + 1];

	// Create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

    // NEHE MULTISAMPLE
    bool    arbMultisampleSupported = false;
    int arbMultisampleFormat = 0;
    // Get Our Pixel Format
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    HDC hDC = winInfo->hDC;
    int pixelFormat;
    int valid;
    UINT numFormats;
    float fAttributes[] = { 0,0 };
    // These Attributes Are The Bits We Want To Test For In Our Sample
    // Everything Is Pretty Standard, The Only One We Want To
    // Really Focus On Is The SAMPLE BUFFERS ARB And WGL SAMPLES
    // These Two Are Going To Do The Main Testing For Whether Or Not
    // We Support Multisampling On This Hardware
    int iAttributes[] = { WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
        WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB,24,
        WGL_ALPHA_BITS_ARB,8,
        WGL_DEPTH_BITS_ARB,0,
        WGL_STENCIL_BITS_ARB,0,
        WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
        WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
        WGL_SAMPLES_ARB, 4,                        // Check For 4x Multisampling
        0,0 };
    // First We Check To See If We Can Get A Pixel Format For 4 Samples
    valid = wglChoosePixelFormatARB(hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);
    // if We Returned True, And Our Format Count Is Greater Than 1
    if (valid && numFormats >= 1) {
        arbMultisampleSupported = true;
        arbMultisampleFormat = pixelFormat;
    } else {
        // Our Pixel Format With 4 Samples Failed, Test For 2 Samples
        iAttributes[19] = 2;
        valid = wglChoosePixelFormatARB(hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);
        if (valid && numFormats >= 1) {
            arbMultisampleSupported = true;
            arbMultisampleFormat = pixelFormat;
        }
    }
    if (arbMultisampleSupported) {
        //SetPixelFormat(winInfo->hDC, arbMultisampleFormat, &pfd);
        //wglMakeCurrent(winInfo->hDC, winInfo->hRC);
        //DestroyWindow(winInfo->hWnd);
        window_end(winInfo);
        // Remove all messages...
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE));
        window_init(winInfo, true, arbMultisampleFormat);
    }
    glEnable(GL_MULTISAMPLE_ARB);

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

	// Create the text editor
	if (editor.init(&shaderManager, &textureManager, errorString))
	{
		MessageBox(winInfo->hWnd, errorString, "Editor init", MB_OK);
		return -1;
	}
	blob = 0.;

    // Create the fluid simulation stuff
    //fluid_simulation_.Init(false);

	return 0;
}

/*************************************************
 * Windows callback
 *************************************************/
static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    int time = timeGetTime() - start_time_;
    float ftime = 0.001f * time;

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

		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_END:
		case VK_HOME:
			break;

		case VK_RETURN:
		case VK_DELETE:
		case VK_BACK:
			break;

#if 0
            BASS_Init(-1, 44100, 0, hWnd, NULL);
            mp3Str = BASS_StreamCreateFile(FALSE, "Goethe_music_quick.mp3", 0, 0, 0);
            BASS_ChannelPlay(mp3Str, TRUE);
            BASS_Start();
            BASS_Stop();
            BASS_ChannelStop(mp3Str);
            BASS_StreamFree(mp3Str);
            BASS_Free();
#endif

		default:
			break;
		}
    }

    if (uMsg == WM_KEYUP) {
        switch(wParam) {
        default:
            break;
        }
    }

    return( DefWindowProc(hWnd,uMsg,wParam,lParam) );
}


void DrawQuad(float startX, float endX, float startY, float endY, float startV, float endV, float alpha)
{
    // set up matrices
    glEnable(GL_TEXTURE_2D);

    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, endV);
    glVertex3f(startX, endY, 0.99f);
    glTexCoord2f(1.0f, endV);
    glVertex3f(endX, endY, 0.99f);
    glTexCoord2f(1.0f, startV);
    glVertex3f(endX, startY, 0.99f);
    glTexCoord2f(0.0, startV);
    glVertex3f(startX, startY, 0.99f);
    glEnd();
}

void intro_do(long t, long delta_time)
{
    char errorText[MAX_ERROR_LENGTH + 1];
    float ftime = 0.001f*(float)t;
    float fdelta_time = 0.001f * (float)(delta_time);
    GLuint textureID;

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    // Those are key-Press indicators. I only act on 0-to-1.
    for (int i = 0; i < maxNumParameters; i++)
    {
        float destination_value = params.getParam(i, defaultParameters[i]);
        interpolatedParameters[i] = expf(-2.5f*fdelta_time) * interpolatedParameters[i] +
            (1.0f - expf(-2.5f*fdelta_time)) * destination_value;
    }
    // Update key press events.
    for (int i = 0; i < NUM_KEYS; i++)
    {
        if (params.getParam(i, 0.0) > 0.5f) keyPressed[i]++;
        else keyPressed[i] = 0;
    }

    // BPM => spike calculation
    float BPS = BPM / 60.0f;
    float jumpsPerSecond = BPS / 1.0f; // Jump on every fourth beat.
    static float phase = 0.0f;
    float jumpTime = (ftime * jumpsPerSecond) + phase;
    jumpTime -= (float)floor(jumpTime);
    if (keyPressed[41] == 1)
    {
        phase -= jumpTime;
        jumpTime = 0.0f;
        if (phase < 0.0f) phase += 1.0;
    }
    jumpTime = jumpTime * jumpTime;
    // spike is between 0.0 and 1.0 depending on the position within whatever.
    float spike = 0.5f * cosf(jumpTime * 3.1415926f * 1.5f) + 0.5f;
    // blob is growing down from 1. after keypress
    static float lastFTime = 0.f;
    blob *= (float)exp(-(float)(ftime - lastFTime) * BLOB_FADE_SPEED);
    lastFTime = ftime;

    // Set the program uniforms
    GLuint programID;
    shaderManager.getProgramID(usedProgram[usedIndex], &programID, errorText);
    glUseProgram(programID);
#if 1
    GLuint loc = glGetUniformLocation(programID, "aspectRatio");
    glUniform1f(loc, aspectRatio);
    loc = glGetUniformLocation(programID, "time");
    glUniform1f(loc, (float)(t * 0.001f));
    // For now I am just sending the spike to the shader. I might need something better...
    loc = glGetUniformLocation(programID, "spike");
    glUniform1f(loc, spike);
    loc = glGetUniformLocation(programID, "blob");
    glUniform1f(loc, blob);
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
#endif

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
    textureManager.getTextureID("hermaniak.tga", &textureID, errorText);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glActiveTexture(GL_TEXTURE1);
    //textureManager.getTextureID(TM_DEPTH_SENSOR_NAME, &textureID, errorText);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glActiveTexture(GL_TEXTURE0);
    textureManager.getTextureID(TM_NOISE3D_NAME, &textureID, errorText);
    glBindTexture(GL_TEXTURE_3D, textureID);

#if 1
    glViewport(0, 0, X_OFFSCREEN, Y_OFFSCREEN);
#endif

    // TODO: Here is the rendering done!
    interpolatedParameters[6] = 0.4f;
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
    glClearColor(red, green, blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
    glUseProgram(programID);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_DST_COLOR, GL_ZERO);

    // Draw one iteration of the IFS
    textureManager.getTextureID("black.tga", &textureID, errorText);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    DrawQuad(0.5f, 0.7f, 0.3f, 0.8f, 0.0f, 1.0f, 1.0f);

#if 1
	// Copy backbuffer to texture
    textureManager.getTextureID(TM_OFFSCREEN_NAME, &textureID, errorText);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN);

	// Copy backbuffer to front (so far no improvement)
	int xres = windowRect.right - windowRect.left;
	int yres = windowRect.bottom - windowRect.top;
	glViewport(0, 0, xres, yres);
    shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
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
#endif
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

	// Initialize COM
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) exit(-1);

	// Example editor usage
	char errorText[MAX_ERROR_LENGTH+1];
	char filename[SM_MAX_FILENAME_LENGTH+1];
	sprintf_s(filename, SM_MAX_FILENAME_LENGTH, "shaders/%s", usedShader[usedIndex]);
	if (editor.loadText(filename, errorText))
	{
		MessageBox(info->hWnd, errorText, "Editor init", MB_OK);
		return -1;
	}

    start_time_ = timeGetTime();
    long last_time = 0;
    
    while( !done )
        {
		long t = timeGetTime() - start_time_;

        while( PeekMessage(&msg,0,0,0,PM_REMOVE) )
        {
            if( msg.message==WM_QUIT ) done=1;
		    TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

        intro_do(t, t - last_time);
		editor.render(t);
        last_time = t;

		SwapBuffers( info->hDC );
	}

    window_end( info );

#ifdef MUSIC
	// music uninit
	BASS_Free();
#endif

	// Un-initialize COM
	CoUninitialize();

    return( 0 );
}

// Note that a key was pressed
void registerParameterChange(int keyID)
{
	const int beatKey = 41;
	const int blobKey = 40;
	const int minBeatTime = 100;
	const int maxBeatTime = 5000;
	int sortedBeatDurations[NUM_BEAT_TIMES];

	// get the blobber
	if (params.getParam(blobKey) > 0.5f) blob = 1.f;

	// Do nothing on key release!
	if (params.getParam(beatKey) < 0.5f) return;

	if (keyID == beatKey)
	{
		int t = timeGetTime();
		int timeDiff = t - lastBeatTime;
		if (timeDiff > minBeatTime && timeDiff < maxBeatTime)
		{
			for (int i = 0; i < NUM_BEAT_TIMES-1; i++)
			{
				beatDurations[i] = beatDurations[i+1];
			}
			beatDurations[NUM_BEAT_TIMES-1] = timeDiff;
		}
		lastBeatTime = t;
	}

	// copy sorted beat durations
	for (int i = 0; i < NUM_BEAT_TIMES; i++)
	{
		sortedBeatDurations[i] = beatDurations[i];
	}

	// Calculate median of beat durations by bubble sorting.
	bool sorted = false;
	while (!sorted)
	{
		sorted = true;
		for (int i = 0; i < NUM_BEAT_TIMES-1; i++)
		{
			if (sortedBeatDurations[i] < sortedBeatDurations[i+1]) {
				int tmp = sortedBeatDurations[i+1];
				sortedBeatDurations[i+1] = sortedBeatDurations[i];
				sortedBeatDurations[i] = tmp;
				sorted = false;
			}
		}
	}

	BPM = 60.0f * 1000.0f / sortedBeatDurations[NUM_BEAT_TIMES/2];
}