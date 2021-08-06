// LiveCoding.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "main.h"
#include "Configuration.h"
#include "glext.h"
#include "GLNames.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "Parameter.h"
#include "bass.h"
#include "TimeLine.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_opengl2.h"
#include "../imgui/imgui_impl_win32.h"

#define MAX_LOADSTRING 100

// Gloabal aspect ratio of everything
float aspect_ratio_ = (float)XRES / (float)YRES;

// Script data
TimeLine timeline_;

// music data
HSTREAM mp3Str_;
double music_time_ = 0.0f;
double music_length_ = 0.0f;

// Global GUI stuff
bool is_editor_ = false;  // If set to false run fullscreen without ImGui
bool loop_ = false;  // Re-start after end.
float playback_ = true;
int edit_keyframe_id_ = 0;


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
	 "glMultiTexCoord2f", "glUniformMatrix4fv",
     "glGenVertexArrays", "glBindVertexArray", "glGenBuffers",
     "glBindBuffer", "glBufferData", "glVertexAttribPointer",
     "glEnableVertexAttribArray",
     "glBufferSubData",
};

/*************************************************
 * The core managing units that hold the resources
 *************************************************/
ShaderManager shaderManager;
TextureManager textureManager;

/*************************************************
* Particle stuff
*************************************************/
#define NUM_PARTICLES_PER_DIM 200
#define TOTAL_NUM_PARTICLES (NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM)
// The vertex array and vertex buffer
unsigned int vaoID;
// 0 is for particle positions, 1 is for particle colors
unsigned int vboID[2];
// And the actual vertices
GLfloat vertices_[TOTAL_NUM_PARTICLES * 3];
GLfloat colors_[TOTAL_NUM_PARTICLES * 4];


/*************************************************
 * Window core variables
 *************************************************/
HINSTANCE hInst;								// Aktuelle Instanz
TCHAR szTitle[MAX_LOADSTRING];					// Titelleistentext
TCHAR szWindowClass[MAX_LOADSTRING];			// Klassenname des Hauptfensters
// The size of the window that we render to...
RECT window_rect;

typedef struct
{
    //---------------
    HINSTANCE   hInstance;
    HDC         hDC;
    HGLRC       hRC;
    HWND        hWnd;
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

static WININFO wininfo = {  0,0,0,0,
							{'l','c','_',0}
                            };

// Multiplices two 4x4 matrices
void matrixMult(float src1[4][4], float src2[4][4], float dest[4][4])
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            dest[i][j] = 0.0f;
            for (int k = 0; k < 4; k++)
            {
                dest[i][j] += src1[i][k] * src2[k][j];
            }
        }
    }
}

// Note that the character data must be allocated.
void LoadTextFile(const char *filename, char *data, int data_size) {
    FILE *fid = fopen(filename, "rb");
    int num_read = fread(data, 1, data_size - 1, fid);
    data[num_read] = 0;
    fclose(fid);
}

// TODO: Check implementation from somewhere else. Esp. %65535? Numeric recipies.
float frand(unsigned int *seed)
{
    unsigned long a = 214013;
    unsigned long c = 2531011;
    unsigned long m = 4294967296 - 1;
    *seed = ((*seed) * a + c) % m;
    //return (seed >> 8) % 65535;
    return (float)(((*seed) >> 8) % 65536) * (1.0f / 65536.0f);
}

// Create the particle locations and move them to the GPU
void GenerateParticles(void) {
    unsigned int seed = 23690984;

    // Set vertex location
    int vertex_id = 0;
    int color_id = 0;
    for (int z = 0; z < NUM_PARTICLES_PER_DIM; z++) {
        float zp = 1.0f - 2.0f * (float)z / (float)NUM_PARTICLES_PER_DIM;
        for (int y = 0; y < NUM_PARTICLES_PER_DIM; y++) {
            float yp = -1.0f + 2.0f * (float)y / (float)NUM_PARTICLES_PER_DIM;
            for (int x = 0; x < NUM_PARTICLES_PER_DIM; x++) {
                float xp = -1.0f + 2.0f * (float)x / (float)NUM_PARTICLES_PER_DIM;
                vertices_[vertex_id++] = xp + 2.0f * frand(&seed) / (float)NUM_PARTICLES_PER_DIM;
                vertices_[vertex_id++] = yp + 2.0f * frand(&seed) / (float)NUM_PARTICLES_PER_DIM;
                vertices_[vertex_id++] = zp + 2.0f * frand(&seed) / (float)NUM_PARTICLES_PER_DIM;
                colors_[color_id++] = 0.9f;
                colors_[color_id++] = 0.7f;
                colors_[color_id++] = 0.5f;
                // fran
                colors_[color_id++] = frand(&seed);
                //colors_[color_id - 1] = 0.5f;
            }
        }
    }
}


/*************************************************
 * OpenGL initialization
 *************************************************/
static int initGL(WININFO *win_info) {
	char errorString[MAX_ERROR_LENGTH + 1];

	// Create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// Create and initialize the shader manager
	if (shaderManager.init(errorString)) {
		MessageBox(win_info->hWnd, errorString, "Shader Manager Load", MB_OK);
		return -1;
	}

	// Create and initialize everything needed for texture Management
	if (textureManager.init(errorString)) {
		MessageBox(win_info->hWnd, errorString, "Texture Manager Load", MB_OK);
		return -1;
	}

    // imGUI initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplOpenGL2_Init();
    ImGui_ImplWin32_Init(win_info->hWnd);
    ImGui::StyleColorsDark();

    // Fill the vertices_ and colors_ array with reasonable values
    GenerateParticles();

    // Set up vertex buffer and stuff
    glGenVertexArrays(1, &vaoID); // Create our Vertex Array Object  
    glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  

    int maxAttrt;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrt);

    glGenBuffers(2, vboID); // Generate our Vertex Buffer Object  

                            // Vertex array position data
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vboID[0]); // Bind our Vertex Buffer Object  
    glBufferData(GL_ARRAY_BUFFER, TOTAL_NUM_PARTICLES * 3 * sizeof(GLfloat),
        vertices_, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
    glVertexAttribPointer(0, // attribute
        3, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0); // array buffer offset

                   // Vertex array color data
                   // change to GL_STATIC_DRAW and single update for speed.
                   // Move the GenerateParticles copy operation to here.
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vboID[1]); // Bind our Vertex Buffer Object  
    glBufferData(GL_ARRAY_BUFFER, TOTAL_NUM_PARTICLES * 4 * sizeof(GLfloat),
        colors_, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
    glVertexAttribPointer(1, // attribute
        4, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0); // array buffer offset

    glBindVertexArray(0); // Bind our Vertex Array Object so we can use it  
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Bind our Vertex Buffer Object  

	return 0;
}

static int ReleaseGL(WININFO *win_info) {
    return 0;
}

void PauseResume(void)
{
    if (playback_) BASS_Pause();
    else BASS_Start();
    playback_ = !playback_;
}

/*************************************************
 * Windows callback
 *************************************************/
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) return true;

	// boton x o pulsacion de escape
	//if( uMsg==WM_CLOSE || uMsg==WM_DESTROY || (uMsg==WM_KEYDOWN && wParam==VK_ESCAPE) )
	if (uMsg == WM_DESTROY) {
		PostQuitMessage(0);
        return 0;
    }

    if (uMsg == WM_SIZE) {
        GetClientRect(hWnd, &window_rect);
        glViewport(0, 0, window_rect.right - window_rect.left, abs(window_rect.bottom - window_rect.top));
        aspect_ratio_ = (float)(window_rect.right - window_rect.left) / (float)(abs(window_rect.bottom - window_rect.top));
        return 0;
    }

    if(uMsg == WM_SYSCOMMAND) {
        if ((wParam & 0xfff0) == SC_KEYMENU) {
            return 0;  // Disable ALT apllication menu
        }
    }

    if (uMsg == WM_KEYDOWN)
    {
        switch(wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;
        case VK_SPACE:
            PauseResume();
            break;
        case 'B':
        case 'b':
            music_time_ = (double)timeline_.time(edit_keyframe_id_);
            BASS_ChannelSetPosition(mp3Str_, BASS_ChannelSeconds2Bytes(mp3Str_, (float)music_time_), BASS_POS_BYTE);
            break;
        case 'N':
        case 'n':
            edit_keyframe_id_--;
            if (edit_keyframe_id_ < 0) edit_keyframe_id_ = 0;
            break;
        case 'M':
        case 'm':
            edit_keyframe_id_++;
            if (edit_keyframe_id_ >= timeline_.NumKeyFrames()) edit_keyframe_id_ = timeline_.NumKeyFrames() - 1;
            break;

        default:
            break;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Callback from Midi:
// This function modifies the values of the currently selected keyframe
void NotifyParamChange(int index, float value)
{
    switch (index)
    {
    case 2:
        timeline_.SetValue(edit_keyframe_id_, 9, value);
        break;
    case 3:
        timeline_.SetValue(edit_keyframe_id_, 10, value);
        break;
    case 4:
        timeline_.SetValue(edit_keyframe_id_, 11, value);
        break;
    case 5:
        timeline_.SetValue(edit_keyframe_id_, 12, value);
        break;
    case 6:
        timeline_.SetValue(edit_keyframe_id_, 13, value);
        break;
    case 8:
        timeline_.SetValue(edit_keyframe_id_, 14, value);
        break;
    case 9:
        timeline_.SetValue(edit_keyframe_id_, 15, value);
        break;
    case 12:
        timeline_.SetValue(edit_keyframe_id_, 16, value);
        break;
    case 13:
        timeline_.SetValue(edit_keyframe_id_, 17, value);
        break;
    case 14:
        timeline_.SetValue(edit_keyframe_id_, 0, value);
        break;
    case 15:
        timeline_.SetValue(edit_keyframe_id_, 1, value);
        break;
    case 16:
        timeline_.SetValue(edit_keyframe_id_, 2, value);
        break;
    case 17:
        timeline_.SetValue(edit_keyframe_id_, 3, value);
        break;
    case 18:
        timeline_.SetValue(edit_keyframe_id_, 4, value);
        break;
    case 19:
        timeline_.SetValue(edit_keyframe_id_, 5, value);
        break;
    case 20:
        timeline_.SetValue(edit_keyframe_id_, 6, value);
        break;
    case 21:
        timeline_.SetValue(edit_keyframe_id_, 7, value);
        break;
    case 22:
        timeline_.SetValue(edit_keyframe_id_, 8, value);
        break;
    default:
        break;
    }
}

static void window_end (WININFO *info) {
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    if (info->hRC) {
        wglMakeCurrent( 0, 0 );
        wglDeleteContext( info->hRC );
    }

    if (info->hDC) ReleaseDC(info->hWnd, info->hDC);
    if (info->hWnd) DestroyWindow(info->hWnd);

    UnregisterClass(info->wndclass, info->hInstance);
}

static int window_init( WININFO *info )
{
	unsigned int	PixelFormat;
    DWORD			dwExStyle, dwStyle;
	RECT			rec;

    WNDCLASS		wc;

    ZeroMemory( &wc, sizeof(WNDCLASS) );
    wc.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = info->hInstance;
    wc.lpszClassName = info->wndclass;
	
    if( !RegisterClass(&wc) )
        return( 0 );

    dwExStyle = 0;
    if (is_editor_)
    {
        dwStyle = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX;
    }
    else
    {
        dwStyle = WS_VISIBLE | WS_POPUP;
    }
    info->hWnd = CreateWindowEx(dwExStyle, wc.lpszClassName, "Editor", dwStyle,
          CW_USEDEFAULT, CW_USEDEFAULT, 1480, 760,
          0, 0, info->hInstance, 0 );
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
    
    //if (is_editor_) ShowWindow(info->hWnd, SW_SHOWDEFAULT);
    //else ShowWindow(info->hWnd, SW_MAXIMIZE);
    ShowWindow(info->hWnd, SW_MAXIMIZE);
    UpdateWindow(info->hWnd);

    if (!is_editor_) ShowCursor(false);

    return( 1 );
}

static void intro_do(float time)
{
	char errorText[MAX_ERROR_LENGTH+1];
	GLuint textureID;

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

    //glClearColor(0.7f, 0.5f , 0.3f, 1.0f);
    //glClearColor(0.35f, 0.25f, 0.15f, 1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  
    glBindBuffer(GL_ARRAY_BUFFER, vboID[0]); // Bind our Vertex Buffer Object  

	// Set the program uniforms
	GLuint programID;
	//shaderManager.getProgramID("example.gprg", &programID, errorText);
    shaderManager.getProgramID("main.gprg", &programID, errorText);
	glUseProgram(programID);

    // Set texture identifiers
	//GLint texture_location;
    //texture_location = glGetUniformLocation(programID, "BGTexture");
    //glUniform1i(texture_location, 0);
    //texture_location = glGetUniformLocation(programID, "Noise3DTexture");
	//glUniform1i(texture_location, 1);

    // Set the render matrix
    float parameterMatrix[4][4];
    //parameterMatrix[0][0] = itime / 44100.0f;

    // Get parameters from timeline (top row first)
#if 1
    float values[KF_NUM_VALUES];
    timeline_.GetValues((float)music_time_, values);
#else
    float *values = timeline_.GetStartValues();
#endif
    
    parameterMatrix[0][1] = values[9];
    parameterMatrix[0][2] = values[10];
    parameterMatrix[0][3] = values[11];

    parameterMatrix[1][0] = values[12];
    parameterMatrix[1][1] = values[13];
    parameterMatrix[1][2] = values[14];
    parameterMatrix[1][3] = values[15];

    parameterMatrix[2][0] = values[16];
    parameterMatrix[2][1] = values[17];
    // from here: top row
    parameterMatrix[2][2] = values[0];
    parameterMatrix[2][3] = values[1];

    parameterMatrix[3][0] = values[2];
    parameterMatrix[3][1] = values[3];
    parameterMatrix[3][2] = values[4];
    parameterMatrix[3][3] = values[5];

    float fly_start = 364.0f;
    float fly_slow = 500.0f;
    if ((float)music_time_ > fly_start)
    {
        float quadratic = 0.003f;
        float linear = 0.06f;
        float intersection = linear / (2 * quadratic);
        float move = (music_time_ - fly_start) * (music_time_ - fly_start) * quadratic;
        if ((float)music_time_ > intersection + fly_start)
        {
            move = (music_time_ - fly_start) * linear - (linear * linear) / (4.0f * quadratic);
        }
        if ((float)music_time_ > fly_slow)
        {
            move -= (music_time_ - fly_slow) * (music_time_ - fly_slow) * 0.00038f;
        }
        parameterMatrix[2][1] += move;
    }

    int location = glGetUniformLocation(programID, "r");
    glUniformMatrix4fv(location, 1, GL_FALSE, &(parameterMatrix[0][0]));
    location = glGetUniformLocation(programID, "R");
    parameterMatrix[0][1] = values[6];
    parameterMatrix[0][2] = values[7];
    parameterMatrix[0][3] = values[8];
    glUniformMatrix4fv(location, 1, GL_FALSE, &(parameterMatrix[0][0]));    

	// render to larger offscreen texture
	//glActiveTexture(GL_TEXTURE0);
	textureManager.getTextureID("code.png", &textureID, errorText);
	glBindTexture(GL_TEXTURE_2D, textureID);
	//glActiveTexture(GL_TEXTURE1);
	//textureManager.getTextureID(TM_NOISE3D_NAME, &textureID, errorText);
	//glBindTexture(GL_TEXTURE_3D, textureID);
    //glActiveTexture(GL_TEXTURE0);

    if (is_editor_)
    {
        glViewport(0, 0, X_OFFSCREEN, Y_OFFSCREEN);
    }

    GLuint loc = glGetUniformLocation(programID, "time");
    glUniform1f(loc, time);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_POINTS, 0, TOTAL_NUM_PARTICLES);

    // Draw some texture
    shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
    glUseProgram(programID);

    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    textureManager.getTextureID("vignette.png", &textureID, errorText);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBegin(GL_QUADS);
    float alpha = 1.0f;
    alpha = 17.0f - ((float)music_time_) / 10.0f;
    if (alpha < 0.5f) alpha = 0.5f;
    glColor4f(alpha, alpha, alpha, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 0.5f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.5f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 0.5f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 0.5f);
    glEnd();

    glBlendFunc(GL_ONE, GL_ONE);
    //glRectf(-0.5, -0.5, 0.5, 0.5);
    if (music_time_ > 160 && music_time_ < 165)
    {
        textureManager.getTextureID("code.png", &textureID, errorText);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBegin(GL_QUADS);
        float xp = -0.25f;
        float yp = -0.15f;
        float width = sin((music_time_ - 160.0f) / 5.0f * 3.1415f);
        float alpha = sqrtf(width);
        glColor4f(alpha, alpha, alpha, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(width * -0.5f+xp, 0.2f+yp, 0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(width * 0.5f+xp, 0.2f+yp, 0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(width * 0.5f+xp, -0.2f+yp, 0.5f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(width * -0.5f+xp, -0.2f+yp, 0.5f);
        glEnd();
    }
    if (music_time_ > 163 && music_time_ < 168)
    {
        textureManager.getTextureID("music.png", &textureID, errorText);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBegin(GL_QUADS);
        float xp = +0.15f;
        float yp = -0.4f;
        float width = sin((music_time_ - 163.0f) / 5.0f * 3.1415f);
        float alpha = sqrtf(width);
        glColor4f(alpha, alpha, alpha, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(width * -0.5f + xp, 0.2f + yp, 0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(width * 0.5f + xp, 0.2f + yp, 0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(width * 0.5f + xp, -0.2f + yp, 0.5f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(width * -0.5f + xp, -0.2f + yp, 0.5f);
        glEnd();
    }


    if (is_editor_)
    {
	    // Copy backbuffer to texture
        textureManager.getTextureID(TM_OFFSCREEN_NAME, &textureID, errorText);
	    glBindTexture(GL_TEXTURE_2D, textureID);
	    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN);

	    // Copy backbuffer to front (so far no improvement)
#if 0	    
        int xres = window_rect.right - window_rect.left;
	    int yres = window_rect.bottom - window_rect.top;
	    glViewport(0, 0, xres, yres);
	    if (false) {
		    shaderManager.getProgramID("DitherTexture.gprg", &programID, errorText);
	    } else {
		    shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
	    }
#endif
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

int WINAPI WinMain( HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    WININFO     *info = &wininfo;

    info->hInstance = GetModuleHandle( 0 );

    if (strstr(lpCmdLine, "-editor")) is_editor_ = true;
    if (strstr(lpCmdLine, "-loop")) loop_ = true;

    if (!window_init(info)) {
        window_end(info);
        MessageBox(0, "window_init()!", "error", MB_OK|MB_ICONEXCLAMATION);
        return 0;
    }

	if (initGL(info)) {
		return 0;
	}

	// start music playback
#ifdef MUSIC
	BASS_Init(-1,44100,0,info->hWnd,NULL);
	HSTREAM mp3Str=BASS_StreamCreateFile(FALSE,"Musik/set1.mp3",0,0,0);
	BASS_ChannelPlay(mp3Str, TRUE);
	BASS_Start();
#endif

	// Initialize COM
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) exit(-1);

    // IMGUI STUFF
    bool show_demo_window = false;
    bool show_preview_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    bool done = false;
    
    // start music playback
    BASS_Init(-1, 44100, 0, info->hWnd, NULL);
    mp3Str_ = BASS_StreamCreateFile(FALSE, "music.mp3", 0, 0, 0);
    BASS_ChannelSetPosition(mp3Str_, BASS_ChannelSeconds2Bytes(mp3Str_, 0.0), BASS_POS_BYTE);
    BASS_ChannelPlay(mp3Str_, TRUE);
    music_length_ = BASS_ChannelBytes2Seconds(mp3Str_, BASS_ChannelGetLength(mp3Str_, BASS_POS_BYTE));
    BASS_Start();

    timeline_.Init((float)music_length_);

    FILE *fid;
    if (is_editor_) fid = fopen("script.autosave.txt", "rb");
    else fid = fopen("script.txt", "rb");
    if (fid)
    {
        timeline_.Load(fid);
        fclose(fid);
    }

    if (is_editor_) loop_ = true;

    while (!done) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) done = true;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

#define FPS60
#ifdef FPS60
        static int counter = 0;
        music_time_ = static_cast<float>(counter) / 60.0f;
        counter++;
#else
        music_time_ = BASS_ChannelBytes2Seconds(mp3Str_, BASS_ChannelGetPosition(mp3Str_, BASS_POS_BYTE));
#endif
        if (music_time_ >= music_length_ - 0.2f)
        {
            if (loop_)
            {
                BASS_ChannelSetPosition(mp3Str_, BASS_ChannelSeconds2Bytes(mp3Str_, 0.0), BASS_POS_BYTE);
                music_time_ = 0.0f;
                //if (!is_editor_) BASS_Start();
            }
            else
            {
                done = true;
            }
        }
        intro_do((float)music_time_);

        if (is_editor_)
        {
            // Start the ImGui frame
            ImGui_ImplOpenGL2_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // 1. Show a simple window.
            // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
            {
                ImGui::Begin("Timeline Window");
                static float f = 0.0f;
                f = (float)music_time_;
                //ImGui::Text("Current time: %f", music_time_);
                ImGui::PushItemWidth(-1);
                if (ImGui::SliderFloat("Playback Time", &f, 0.0f, (float)music_length_, "Playback Time: %.2f s"))            // Edit 1 float using a slider from 0.0f to music_time_
                {
                    music_time_ = f;
                    BASS_ChannelSetPosition(mp3Str_, BASS_ChannelSeconds2Bytes(mp3Str_, f), BASS_POS_BYTE);
                }
                ImGui::PopItemWidth();
            
                if (playback_ && ImGui::Button("Pause <SPACE>")) PauseResume();
                if (!playback_ && ImGui::Button("Play  <SPACE>")) PauseResume();
                ImGui::SameLine();
                if (ImGui::Button("Restart"))
                {
                    music_time_ = 0.0f;
                    BASS_ChannelSetPosition(mp3Str_, BASS_ChannelSeconds2Bytes(mp3Str_, (float)music_time_), BASS_POS_BYTE);
                }
                ImGui::SameLine();
                if (ImGui::Button("To Keyframe <B>"))
                {
                    music_time_ = (double)timeline_.time(edit_keyframe_id_);
                    BASS_ChannelSetPosition(mp3Str_, BASS_ChannelSeconds2Bytes(mp3Str_, (float)music_time_), BASS_POS_BYTE);
                }

                //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
                //ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
                //ImGui::Checkbox("Preview Window", &show_preview_window);
                //if (ImGui::Button("Button"))  counter++;                          // Buttons return true when clicked (NB: most widgets return true when edited/activated)  
                //ImGui::SameLine();
                //ImGui::Text("counter = %d", counter);
                //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            
                char id_text[200];
                sprintf(id_text, "Keyframe ID: %%d (%d)", timeline_.NumKeyFrames() - 1);
                ImGui::SliderInt("", &edit_keyframe_id_, 0, timeline_.NumKeyFrames() - 1, id_text);
                static float keyframe_time = 0.0f;
            
                keyframe_time = timeline_.time(edit_keyframe_id_);
                ImGui::PushItemWidth(-1);
                if (ImGui::SliderFloat("KeyFrame time", &keyframe_time, 0.0f, (float)music_length_, "Keyframe Time %.2f s"))
                {
                    timeline_.SetKeyFrameTime(edit_keyframe_id_, keyframe_time);
                }
                ImGui::PopItemWidth();

                if (ImGui::Button("Previous (N)"))
                {
                    edit_keyframe_id_--;
                    if (edit_keyframe_id_ < 0) edit_keyframe_id_ = 0;
                }
                ImGui::SameLine();
                if (ImGui::Button("Next (M)"))
                {
                    edit_keyframe_id_++;
                    if (edit_keyframe_id_ >= timeline_.NumKeyFrames()) edit_keyframe_id_ = timeline_.NumKeyFrames() - 1;
                }
                ImGui::SameLine();
                if (ImGui::Button("Add Keyframe"))
                {
                    timeline_.AddKeyFrame(edit_keyframe_id_);
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete Keyframe"))
                {
                    timeline_.DeleteKeyFrame(edit_keyframe_id_);
                }
                ImGui::SameLine();
                if (ImGui::Button("Save"))
                {
                    FILE *fid = fopen("script.txt", "wb");
                    timeline_.Save(fid);
                    fclose(fid);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load"))
                {
                    FILE *fid = fopen("script.txt", "rb");
                    timeline_.Load(fid);
                    fclose(fid);
                }

                ImGui::End();
            }
            // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
            if (show_preview_window)
            {
                char error_text[MAX_ERROR_LENGTH + 1];
                GLuint texture_id;
                ImGui::Begin("Preview Window", 0 /* Always shown */,
                             0 /*ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar*/);
                if (textureManager.getTextureID(TM_OFFSCREEN_NAME, &texture_id, error_text))
                {
                    MessageBox(info->hWnd, error_text, "Get Renderbuffer", MB_OK);
                    break;
                }
                ImGui::Image((ImTextureID)texture_id, ImVec2(XRES, YRES));
                ImGui::End();
            }
            // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
    #if 0
            if (show_demo_window)
            {
                ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
                ImGui::ShowDemoWindow(&show_demo_window);
            }
    #endif
            // 4. The current keyframe image
            {
                ImGui::Begin("Keyframe Values Window");
                const float spacing = 8;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
                static float values[KF_NUM_VALUES] = { 0.5f };
                ImGui::PushID("set1");
                for (int i = 0; i < KF_NUM_VALUES; i++)
                {
                    values[i] = timeline_.value(edit_keyframe_id_, i);
                    if (i > 0 && i != KF_NUM_VALUES / 2) ImGui::SameLine();
                    ImGui::PushID(i);
                    if (ImGui::VSliderFloat("##v", ImVec2(18, 160), &values[i], 0.0f, 1.0f, ""))
                    {
                        timeline_.SetValue(edit_keyframe_id_, i, values[i]);
                    }
                    if (ImGui::IsItemActive() || ImGui::IsItemHovered()) ImGui::SetTooltip("%.3f", values[i]);
                    ImGui::PopID();
                }
                ImGui::PopID();
                ImGui::PopStyleVar();
                ImGui::End();
            }
            //  5. The interpolated keyframe image
            {
                ImGui::Begin("Current Values Window");
                const float spacing = 8;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(0.3f, 0.2f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(0.3f, 0.3f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(0.3f, 0.4f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(0.3f, 0.5f, 0.9f));
                static float values[KF_NUM_VALUES] = { 0.5f };
                for (int i = 0; i < KF_NUM_VALUES; i++)
                {
                    timeline_.GetValues((float)music_time_, values);
                    if (i > 0 && i != KF_NUM_VALUES / 2) ImGui::SameLine();
                    ImGui::VSliderFloat("##v", ImVec2(18, 160), &values[i], 0.0f, 1.0f, "");
                    if (ImGui::IsItemActive() || ImGui::IsItemHovered()) ImGui::SetTooltip("%.3f", values[i]);
                }
                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar();
                ImGui::End();
            }

            // Rendering imgui
            ImGui::EndFrame();
            ImGui::Render();
            int w = window_rect.right - window_rect.left;
            int h = window_rect.bottom - window_rect.top;
            glViewport(0, 0, w, h);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
            ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        }

		SwapBuffers(info->hDC);
	}    

    window_end(info);

    if (is_editor_)
    {
        fid = fopen("script.autosave.txt", "wb");
        timeline_.Save(fid);
        fclose(fid);
    }

#ifdef MUSIC
	// music uninit
	BASS_ChannelStop(mp3Str);
	BASS_StreamFree(mp3Str);
	BASS_Free();
#endif

    if (ReleaseGL(info)) {
        return 0;
    }

	// Un-initialize COM
	CoUninitialize();

    return( 0 );
}
