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
#define BLOB_FADE_SPEED 1.0f

// The used effect (will be changeable later on)
#define NUM_USED_PROGRAMS 10
char *usedShader[NUM_USED_PROGRAMS] = {"empty.jlsl", "vp1.jlsl", "vp2.jlsl", "vp3.jlsl", "vp4.jlsl", "vp5.jlsl", "vp6.jlsl", "vp7.jlsl", "vp8.jlsl", "vp9.jlsl"};
char *usedProgram[NUM_USED_PROGRAMS] = {"empty.gprg", "vp1.gprg", "vp2.gprg", "vp3.gprg", "vp4.gprg", "vp5.gprg", "vp6.gprg", "vp7.gprg", "vp8.gprg", "vp9.gprg"};
int usedIndex = 0;
float aspectRatio = (float)XRES / (float)YRES;

// Used to overwrite the move parameter
int destination_distance_ = -1;  // do not use

float music_start_time_ = -10000.0f;
float otone_start_time_ = 1.0e20f;
float real_otone_start_time_ = 1.0e20f;  // triggered by close-to-entrance rotation
float masako_start_time_ = 1.0e20f;
float real_masako_start_time_ = 1.0e20f;  // triggered by close-to-entrance rotation
float title_start_time_ = 1.0e20f;

// Time when theatre goes black (end/start)
float black_end_time_ = 1.0e20f;
// Set to true if the animation stops
bool rotation_stopped_ = false;

// Engawa logo
float ending_start_time_ = 1.0e20f;

long start_time_ = 0;
//FluidSimulation fluid_simulation_;
HSTREAM mp3Str;

// Of the music stuff
const int kNumEdges = 8;
const int kNumSegmentsPerEdge = 16;
const int kNumSegments = kNumEdges * kNumSegmentsPerEdge;
// Created by music, used by floaty stuff
float interpolation_quad_[2][kNumSegments][4][2];

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

        case '0':
            destination_distance_ = -1;
            break;
        case '1':
            destination_distance_ = 0;
            break;
        case '2':
            destination_distance_ = 1;
            break;
        case '3':
            destination_distance_ = 2;
            break;
        case '4':
            destination_distance_ = 3;
            break;
        case '5':
            destination_distance_ = 4;
            break;

        case 'q':
        case 'Q':
            music_start_time_ = 0.001f * (timeGetTime() - start_time_);
            black_end_time_ = -10000.0f;
            ending_start_time_ = 1.0e20f;
            rotation_stopped_ = false;
            // start music playback
#ifdef MUSIC
            BASS_Init(-1, 44100, 0, hWnd, NULL);
            mp3Str = BASS_StreamCreateFile(FALSE, "Goethe_music_quick.mp3", 0, 0, 0);
            BASS_ChannelPlay(mp3Str, TRUE);
            BASS_Start();
#endif
            break;
        case 'w':
        case 'W':
            music_start_time_ = -10000.0f;
            black_end_time_ = -10000.0f;
            ending_start_time_ = 1.0e20f;
            rotation_stopped_ = false;
#ifdef MUSIC
            BASS_Stop();
            BASS_ChannelStop(mp3Str);
            BASS_StreamFree(mp3Str);
            BASS_Free();
#endif
            break;

        case 'a':
        case 'A':
            //fluid_simulation_.request_set_points_ = true;
            otone_start_time_ = 1.0e20f;
            real_otone_start_time_ = otone_start_time_;
            masako_start_time_ = 1.0e20f;
            real_masako_start_time_ = masako_start_time_;
            if (black_end_time_ > 0.001f * (timeGetTime() - start_time_)) {
                black_end_time_ = 0.001f * (timeGetTime() - start_time_);
            }
            ending_start_time_ = 1.0e20f;
            rotation_stopped_ = false;
            break;

        case 's':
        case 'S':
            otone_start_time_ = 0.001f * (timeGetTime() - start_time_);
            black_end_time_ = -10000.0f;
            ending_start_time_ = 1.0e20f;
            rotation_stopped_ = false;
            break;

        case 'd':
        case 'D':
            masako_start_time_ = 0.001f * (timeGetTime() - start_time_);
            black_end_time_ = -10000.0f;
            ending_start_time_ = 1.0e20f;
            rotation_stopped_ = false;
            break;

        case 'o':
        case 'O':
            //fluid_simulation_.Init(false);
            rotation_stopped_ = false;
            break;

        case 'p':
        case 'P':
            //fluid_simulation_.Init(true);
            rotation_stopped_ = true;
            break;

        case 'x':
        case 'X':
            //fluid_simulation_.PushApart();
            title_start_time_ = 1.0e20f;
            black_end_time_ = 1.0e20f;
            ending_start_time_ = 1.0e20f;
            rotation_stopped_ = true;
            break;

        case 'c':
        case 'C':
            //fluid_simulation_.PushApart();
            black_end_time_ = 1.0e20f;
            ending_start_time_ = 0.001f * (timeGetTime() - start_time_);
            break;

        case 'n':
        case 'N':
            black_end_time_ = 1.0e20f;
            title_start_time_ = 0.001f * (timeGetTime() - start_time_);
            break;

        case 'm':
        case 'M':
            if (GetAsyncKeyState(VK_CONTROL) < 0) {
                // TODO: Minimization again.
                SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                ShowWindow(hWnd, SW_MAXIMIZE);
                GetClientRect(hWnd, &windowRect);
                glViewport(0, 0, windowRect.right - windowRect.left, abs(windowRect.bottom - windowRect.top)); //NEW
                aspectRatio = (float)(windowRect.right - windowRect.left) / (float)(abs(windowRect.bottom - windowRect.top));
                ShowCursor(false);
            }
            break;

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


void drawQuad(float startX, float endX, float startY, float endY, float startV, float endV, float alpha)
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

// interpolation with the interpolation_quad
void DrawTearCircle(float start_angle, float end_angle, float distance,
                    float center_x, float center_y,
                    int person, float interpolation,
                    float ftime,
                    float thickness) {
    float kTearWidth = 0.04f * thickness;
    float cur_angle = start_angle;
    float delta_angle = (end_angle - start_angle) / kNumSegments;
    float tear_delta_position = 1.0f / (kNumSegments + 1);
    float tear_position = tear_delta_position / 8.0f;

    // Initialize OGL stuff
    GLuint programID;
    char errorText[MAX_ERROR_LENGTH + 1];
    shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
    glUseProgram(programID);
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);

    for (int i = 0; i < kNumSegments; i++) {
        float left_angle = cur_angle;
        float right_angle = cur_angle + delta_angle;
        float tear_left_pos = tear_position;  // nothing for 0.
        float tear_right_pos = tear_left_pos + tear_delta_position;
        float left_width = kTearWidth * sinf(tear_left_pos * 3.1415f) / sqrtf(tear_left_pos);
        float right_width = kTearWidth * sinf(tear_right_pos * 3.1415f) / sqrtf(tear_right_pos);

        float left_pos[2] = {
            distance * sinf(left_angle) + center_x,
            distance * cosf(left_angle) + center_y
        };
        float right_pos[2] = {
            distance * sinf(right_angle) + center_x,
            distance * cosf(right_angle) + center_y
        };
        float left_normal[2] = { sinf(left_angle), cosf(left_angle) };
        float right_normal[2] = { sinf(right_angle), cosf(right_angle) };

        float t = interpolation;
        float k = 1.0f - t;
        float vertices[4][2] = {
            {left_pos[0] - left_width * left_normal[0], (left_pos[1] - left_width * left_normal[1]) * aspectRatio},
            {left_pos[0] + left_width * left_normal[0], (left_pos[1] + left_width * left_normal[1]) * aspectRatio},
            {right_pos[0] + right_width * right_normal[0], (right_pos[1] + right_width * right_normal[1]) * aspectRatio},
            {right_pos[0] - right_width * right_normal[0],(right_pos[1] - right_width * right_normal[1]) * aspectRatio}
        };
        glVertex2f(t * interpolation_quad_[person][i][0][0] + k * vertices[0][0], t * interpolation_quad_[person][i][0][1] + k * vertices[0][1]);
        glVertex2f(t * interpolation_quad_[person][i][1][0] + k * vertices[1][0], t * interpolation_quad_[person][i][1][1] + k * vertices[1][1]);
        glVertex2f(t * interpolation_quad_[person][i][2][0] + k * vertices[2][0], t * interpolation_quad_[person][i][2][1] + k * vertices[2][1]);
        glVertex2f(t * interpolation_quad_[person][i][3][0] + k * vertices[3][0], t * interpolation_quad_[person][i][3][1] + k * vertices[3][1]);

        cur_angle = right_angle;
        tear_position = tear_right_pos;
    }

    glEnd();
}

void DrawMusic(float ftime) {
    float delta_angle = 3.141592f * 2.0f / kNumEdges;
    const float music_beat = 0.405f;
    const float rotation_speed = 0.1f;

    // Initialize OGL stuff
    GLuint programID;
    char errorText[MAX_ERROR_LENGTH + 1];
    shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
    glUseProgram(programID);
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);

    // Just a 5-edge thingie
    for (int person = 0; person < 2; person++) {
        //float divergence = person * -sinf(ftime * 3.1415f / 160.0f * 8.0f);
        //if (divergence < 0.0f) divergence = 0.0f;
        float divergence = person * sinf(ftime * 3.1415f / 160.0f * 7.5f);
        divergence *= divergence;
        if (ftime < 20.0f) divergence = 0.0f;

        int interpolation_quad_id = 0;
        for (int edge = 0; edge < kNumEdges; edge++) {
            float start_angle = edge * delta_angle;
            int next_edge = edge + 1;
            if (edge == kNumEdges - 1) next_edge = 0;
            float end_angle = next_edge * delta_angle;

            float beat_overdrive = sinf(ftime * 3.1415f / 160.0f) * 2.0f + .5f;
            if (ftime > 80.0f) beat_overdrive = 1.0f;
            float rotation = rotation_speed * 3.1415f * 2.0f * ftime;
            float beat = sinf(ftime * 3.1415f * 2.0f / music_beat);
            //beat *= beat * beat;
            rotation += rotation_speed * beat * music_beat * beat_overdrive * 0.35f;
            start_angle -= rotation;
            end_angle -= rotation;

            float start_line[2] = { cosf(start_angle), sinf(start_angle) };
            float end_line[2] = { cosf(end_angle), sinf(end_angle) };

            float star_amount = 0.2f * sinf(ftime * 3.1415f / 160.0f);
            if (ftime > 80.0f) star_amount = 0.2f;
            float inner_dist_start = 0.27f + star_amount * sinf(0.2f * ftime + 3.1415f * 2.0f * edge / kNumEdges * 10 + divergence*star_amount);
            float inner_dist_end = 0.27f + star_amount * sinf(0.2f * ftime + 3.1415f * 2.0f * next_edge / kNumEdges * 10 + divergence*star_amount);
            float outer_dist_start = 0.35f + star_amount * sinf(0.2f * ftime + 3.1415f * 2.0f * edge / kNumEdges * 10 + divergence);
            float outer_dist_end = 0.35f + star_amount * sinf(0.2f * ftime + 3.1415f * 2.0f * next_edge / kNumEdges * 10 + divergence);

            float size_overdrive = sinf(ftime * 3.1415f / 160.0f) * 2.0f + .4f;
            float size = 1.0f + 0.2f * size_overdrive +
                size_overdrive * (0.25f * sinf(ftime * 0.4f + divergence * 3.14f) + 0.15f * sinf(ftime * 0.25f + divergence * 3.14f));
            inner_dist_start *= size;
            inner_dist_end *= size;
            outer_dist_start *= size;
            outer_dist_end *= size;

            float vertices[4][2] = {
                {start_line[0] * inner_dist_start, start_line[1] * inner_dist_start * aspectRatio},
                {start_line[0] * outer_dist_start, start_line[1] * outer_dist_start * aspectRatio},
                {end_line[0] * outer_dist_end, end_line[1] * outer_dist_end * aspectRatio},
                {end_line[0] * inner_dist_end, end_line[1] * inner_dist_end * aspectRatio}
            };
            for (int segment = 0; segment < kNumSegmentsPerEdge; segment++) {
                float t_start = (float)segment / (float)(kNumSegmentsPerEdge);
                float k_start = 1.0f - t_start;
                float t_end = (float)(segment + 1) / (float)(kNumSegmentsPerEdge);
                float k_end = 1.0f - t_end;
                interpolation_quad_[person][interpolation_quad_id][0][0] = k_start * vertices[0][0] + t_start * vertices[3][0];
                interpolation_quad_[person][interpolation_quad_id][0][1] = k_start * vertices[0][1] + t_start * vertices[3][1];
                interpolation_quad_[person][interpolation_quad_id][1][0] = k_start * vertices[1][0] + t_start * vertices[2][0];
                interpolation_quad_[person][interpolation_quad_id][1][1] = k_start * vertices[1][1] + t_start * vertices[2][1];
                interpolation_quad_[person][interpolation_quad_id][3][0] = k_end * vertices[0][0] + t_end * vertices[3][0];
                interpolation_quad_[person][interpolation_quad_id][3][1] = k_end * vertices[0][1] + t_end * vertices[3][1];
                interpolation_quad_[person][interpolation_quad_id][2][0] = k_end * vertices[1][0] + t_end * vertices[2][0];
                interpolation_quad_[person][interpolation_quad_id][2][1] = k_end * vertices[1][1] + t_end * vertices[2][1];
                glVertex2f(interpolation_quad_[person][interpolation_quad_id][0][0], interpolation_quad_[person][interpolation_quad_id][0][1]);
                glVertex2f(interpolation_quad_[person][interpolation_quad_id][1][0], interpolation_quad_[person][interpolation_quad_id][1][1]);
                glVertex2f(interpolation_quad_[person][interpolation_quad_id][2][0], interpolation_quad_[person][interpolation_quad_id][2][1]);
                glVertex2f(interpolation_quad_[person][interpolation_quad_id][3][0], interpolation_quad_[person][interpolation_quad_id][3][1]);
                interpolation_quad_id++;
            }
        }
    }

    glEnd();
}

// Anfang langsamer
// Anfang Masako immer synchron, kein Überlappen!
// Nach Musik langsamer
// Black aprupt
// Streit kein Kreis - ggf. zwei unterschiedliche Zentren für die Kreise
// Bei Masako eh voraussichtlich Kreis 70* drehen
// Feste Parameter, dass ich nur noch auf Tasten drücken muss...
// Abstand ruckartig, ca. 3 unterschiedliche Stufen. Geschwindigkeit immer gleich?
//    --> Zusammengehen immer langsam.
//    --> Problem nach dem Streit: Zusammen <-> Auseinander hin-und-her: Wie mach ich das?
// Mittwoch: 19:00
// Donnerstag: Ab 18:00 Sprichst mit Masako
// Freitag: 9-11

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
        if (destination_distance_ >= 0) {
            if (i == 2 || i == 3) destination_value = destination_distance_ * 0.25f;
            else destination_value = 0.0f;
        }
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
#if 0
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

#if 0
    if (usedIndex > 4) {
        glViewport(0, 0, X_HIGHLIGHT, Y_HIGHLIGHT);
    }
    else {
        glViewport(0, 0, X_OFFSCREEN, Y_OFFSCREEN);
    }
#endif

    // TODO: Here is the rendering done!
    interpolatedParameters[6] = 0.4f;
    float red = 1.0f + sinf(ftime * 0.3f) * interpolatedParameters[6];
    float green = 1.0f + sinf(ftime * 0.4f) * interpolatedParameters[6];
    float blue = 1.0f - sinf(ftime * 0.3f) * interpolatedParameters[6];
    glClearColor(red, green, blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shaderManager.getProgramID("DitherTexture.gprg", &programID, errorText);
    glUseProgram(programID);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);

    glDisable(GL_CULL_FACE);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glDisable(GL_BLEND);

    if (ftime - music_start_time_ > 159.0f ||
        ftime - music_start_time_ < 0.5f) {  // Do the fluid stuff
#if 0
        fluid_simulation_.UpdateTime(fdelta_time);
        fluid_simulation_.GetTexture();
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
        const float line_length = 0.35f;
        float rotation_speed = 0.09f + 0.75f * interpolatedParameters[13];
        static float rotation = 0.0f;
        if (rotation > 2.0f * 3.1415928f) rotation -= 2.0f * 3.1415928f;
        float distance1 = interpolatedParameters[2] * 0.7f + 1.0f;
        float distance2 = interpolatedParameters[3] * 0.7f + 1.0f;

        if (ftime >= otone_start_time_ &&
            ftime < real_otone_start_time_) {
            // The theatre just started, init the rotation to where it should be
            rotation = 2.0f * 3.141529f - 1.5f;
            real_otone_start_time_ = ftime;
        }
        float incoming1 = (real_otone_start_time_ - ftime) * 0.04f + 0.75f;
        if (incoming1 < 0.0f) incoming1 = 0.0f;
        incoming1 *= incoming1;
        distance1 += 1.1f * incoming1 / line_length;

        // slow down in the beginning
        float inverter = 1.0f;
        if (!rotation_stopped_) {
            rotation += rotation_speed * fdelta_time * 3.1415f * 2.0f * (1.0f - incoming1);
        }
        else {
            inverter = -1.0f;
            //distance1 += 0.1f;
            //distance2 -= 0.5f;
            rotation += 3.1415f * 0.4f;
        }

        float center1_move_x = sinf(ftime * 1.7f) * interpolatedParameters[12] * 0.4f;
        float center1_move_y = sinf(ftime * 0.57f) * interpolatedParameters[12] * 0.4f;
        float center2_move_x = sinf(ftime * 1.97f) * interpolatedParameters[12] * 0.4f;
        float center2_move_y = sinf(ftime * 0.3f) * interpolatedParameters[12] * 0.4f;

        static float masako_rotation_error = 0.0f;
        if (ftime >= masako_start_time_ &&
            ftime < real_masako_start_time_) {
            // The theatre just started, init the rotation to where it should be
            float destination_rotation = 2.0f * 3.141529f - 1.5f;
            if (rotation - 1.0f < destination_rotation) {
                masako_rotation_error = destination_rotation - rotation;
            } else {
                masako_rotation_error = destination_rotation - rotation + 2.0f * 3.141529f;
            }

            // Only start Masako if she is about at the right position
            if (fabsf(masako_rotation_error) < 0.5f) {
                real_masako_start_time_ = ftime;
            }
        }
        float incoming2 = (real_masako_start_time_ - ftime) * 0.04f + 0.75f;
        if (incoming2 < 0.0f) incoming2 = 0.0f;
        incoming2 *= incoming2;
        distance2 += 1.1f * incoming2 / line_length;
        float masako_rotation = rotation + 3.1415f;
        float rotation_adaptation = 1.0f - 0.03f * (ftime - real_masako_start_time_);
        if (rotation_adaptation < 0.0f) rotation_adaptation = 0.0f;
        rotation_adaptation = 1.0f - cosf(rotation_adaptation * 3.1415f / 2.0f);
        masako_rotation += rotation_adaptation * masako_rotation_error;

        // Interpolation with the music stuff
        float interpolation = 1.0f - (ftime - music_start_time_ - 159.0f) * 0.015f;
        if (interpolation < 0.0f || ftime - music_start_time_ < 0.5f) interpolation = 0.0f;
        interpolation *= interpolation;

        float length_difference = 1.2f * interpolatedParameters[4];
        if (ftime >= real_otone_start_time_) {
            DrawTearCircle(rotation + inverter * (1.6f - length_difference) / distance1,
                rotation - inverter * (1.6f - length_difference) / distance1,
                0.35f * distance1,
                -0.7f * incoming1 - center1_move_x, -0.8f * incoming1 - center1_move_y,
                0, interpolation, ftime,
                1.0f - length_difference * 0.2f);
        }
        if (ftime >= real_masako_start_time_) {
            DrawTearCircle(masako_rotation + inverter * (1.6f + length_difference) / distance2,
                masako_rotation - inverter * (1.6f + length_difference) / distance2,
                0.35f * distance2,
                0.7f * incoming2 - center2_move_x, 0.8f * incoming2 - center2_move_y,
                1, interpolation, ftime,
                1.0f + length_difference * 0.2f);
        }

        if (rotation_stopped_) {
            rotation  -= 3.1415f * 0.4f;
        }
    } else {  // Do the rigit dance stuff
#if 0
        glViewport(0, 0, X_HIGHLIGHT, Y_HIGHLIGHT);
        DrawMusic(ftime - music_start_time_);
        textureManager.getTextureID(TM_HIGHLIGHT_NAME, &textureID, errorText);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_HIGHLIGHT, Y_HIGHLIGHT);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, fluid_simulation_.GetBackBuffer());
        fluid_simulation_.SetBackBuffer();
#endif
        int xres = windowRect.right - windowRect.left;
        int yres = windowRect.bottom - windowRect.top;
        glViewport(0, 0, xres, yres);
        DrawMusic(ftime - music_start_time_);
    }

    if (ftime < black_end_time_ + 2.0f) {
        //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT);
        char errorString[MAX_ERROR_LENGTH + 1];
        GLuint texID;
        shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
        glUseProgram(programID);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        textureManager.getTextureID("black.tga", &texID, errorString);
        glBindTexture(GL_TEXTURE_2D, texID);
        float alpha = 1.0f - (ftime - black_end_time_) * 0.5f;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        alpha = 0.5f - 0.5f * (float)cos(alpha * 3.14159);
        drawQuad(-9.0f, 9.0f, -9.0f, 9.0f, 0.0f, 0.0f, alpha);

        if (ftime > title_start_time_) {
            textureManager.getTextureID("title_1x1.tga", &texID, errorString);
            glBindTexture(GL_TEXTURE_2D, texID);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            alpha = (ftime - title_start_time_) * 0.5f;
            if (alpha < 0.0f) alpha = 0.0f;
            if (alpha > 1.0f) alpha = 1.0f;
            alpha = 0.5f - 0.5f * (float)cos(alpha * 3.14159);
            drawQuad(-0.3f, 0.3f, -0.3f * aspectRatio, 0.3f * aspectRatio, 0.0f, 1.0f, alpha);
        }
    }

    if (ftime > ending_start_time_)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
        glUseProgram(programID);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        // Draw icon
        char errorString[MAX_ERROR_LENGTH + 1];
        GLuint texID;
        textureManager.getTextureID("icon.tga", &texID, errorString);
        glBindTexture(GL_TEXTURE_2D, texID);
        float alpha;
        if (ftime - ending_start_time_ < 0.4f) alpha = 0.0f;
        else alpha = 1.0f;
        drawQuad(-0.5f, 0.5f, -0.35f * aspectRatio, 0.65f * aspectRatio, 0.0f, 1.0f, alpha);

        // Draw first highlight
        textureManager.getTextureID("icon_highlight1.tga", &texID, errorString);
        glBindTexture(GL_TEXTURE_2D, texID);
        if (ftime - ending_start_time_ < 0.3f) alpha = (ftime - ending_start_time_) / 0.3f;
        else alpha = 1.1f - (ftime - ending_start_time_ - 0.3f) * 0.5f;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        alpha = 0.5f - 0.5f * (float)cos(alpha * 3.14159);
        drawQuad(-0.5f, 0.5f, -0.35f * aspectRatio, 0.65f * aspectRatio, 0.0f, 1.0f, alpha*0.75f);

        // Draw second highlight
        textureManager.getTextureID("icon_highlight2.tga", &texID, errorString);
        glBindTexture(GL_TEXTURE_2D, texID);
        if (ftime - ending_start_time_ < 0.4f) alpha = (ftime - ending_start_time_ - 0.3f) / 0.1f;
        else alpha = 1.2f - (ftime - ending_start_time_ - 0.4f) * 1.2f;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        alpha = 0.5f - 0.5f * (float)cos(alpha * 3.14159);
        alpha *= 0.75f;
        drawQuad(-0.5f, 0.5f, -0.35f * aspectRatio, 0.65f * aspectRatio, 0.0f, 1.0f, alpha*0.75f);

        // draw some sparkles
        textureManager.getTextureID("sparkle.tga", &texID, errorString);
        glBindTexture(GL_TEXTURE_2D, texID);
        float sparkleTime = (ftime - ending_start_time_ - 0.4f) * 0.4f;
        for (int i = 0; i < 16; i++)
        {
            float sparkleDuration = 1.3f + 0.4f * sinf(i*2.4f + 2.3f);
            if (sparkleTime > 0.0f && sparkleTime < sparkleDuration)
            {
                float amount = sqrtf(sinf((sparkleTime / sparkleDuration * 3.1415f)));
                float iconDistance = 0.5f;
                float ASPECT_RATIO = aspectRatio;
                float centerX = -0.3f + iconDistance * (0.55f + 0.35f * sinf(i*2.1f + 7.3f));
                centerX += (0.7f + 0.15f*sinf(i*1.4f + 8.3f)) * iconDistance / sparkleDuration * sparkleTime -
                    0.1f * sparkleTime*sparkleTime / sparkleDuration / sparkleDuration;
                float centerY = 0.6f + iconDistance * ASPECT_RATIO * (0.8f + 0.3f * sinf(i*4.6f + 2.9f) - 1.0f);
                centerY += (0.5f + 0.2f*sinf(i*6.8f + 3.0f)) * iconDistance / sparkleDuration * sparkleTime * ASPECT_RATIO -
                    0.2f * sparkleTime*sparkleTime / sparkleDuration / sparkleDuration;
                float width = iconDistance * 0.25f;
                drawQuad(centerX - width, centerX + width,
                    centerY - width * ASPECT_RATIO, centerY + width * ASPECT_RATIO,
                    0.0f, 1.0f, amount);
            }
        }

        // draw the homepage thingie
        textureManager.getTextureID("homepage_4x1.tga", &texID, errorString);
        glBindTexture(GL_TEXTURE_2D, texID);
        alpha = 1.0f;
        if (ftime - ending_start_time_ < 4.5f) alpha = sinf((ftime - ending_start_time_ - 2.5f) * 0.5f * 3.1415f * 0.5f);
        if (ftime - ending_start_time_ < 2.5f) alpha = 0.0f;
        alpha = 1.0f;
        drawQuad(-0.5f, 0.5f, -0.4f * aspectRatio, -0.15f * aspectRatio, 0.0f, 1.0f, alpha);

        glDisable(GL_BLEND);
    }


#if 0
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
    glUseProgram(programID);
    textureManager.getTextureID("blatt.tga", &textureID, errorText);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
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

#if 0
	// Copy backbuffer to texture
	if (usedIndex > 4) {
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