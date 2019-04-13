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
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_opengl2.h"
#include "../imgui/imgui_impl_win32.h"

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
#define NUM_PARTICLES_PER_DIM 128
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

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
    dwStyle = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU |
        WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX;

    rec.left   = 0;
    rec.top    = 0;
    rec.right  = XRES;
    rec.bottom = YRES;
    AdjustWindowRect( &rec, dwStyle, 0 );
    RECT window_rect;
	window_rect.left = 0;
	window_rect.top = 0;
	window_rect.right = XRES;
	window_rect.bottom = YRES;

    info->hWnd = CreateWindowEx(dwExStyle, wc.lpszClassName, "Editor", dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
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
    
    ShowWindow(info->hWnd, SW_SHOWDEFAULT);
    UpdateWindow(info->hWnd);

    return( 1 );
}

static void intro_do(float time)
{
	char errorText[MAX_ERROR_LENGTH+1];
	GLuint textureID;

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

    //glClearColor(0.7f, 0.5f , 0.3f, 1.0f);
    glClearColor(0.35f, 0.25f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  
    glBindBuffer(GL_ARRAY_BUFFER, vboID[0]); // Bind our Vertex Buffer Object  

	// Set the program uniforms
	GLuint programID;
	//shaderManager.getProgramID("example.gprg", &programID, errorText);
    shaderManager.getProgramID("main.gprg", &programID, errorText);
	glUseProgram(programID);

    // Set texture identifiers
	GLint texture_location;
    //texture_location = glGetUniformLocation(programID, "BGTexture");
    //glUniform1i(texture_location, 0);
    //texture_location = glGetUniformLocation(programID, "Noise3DTexture");
	//glUniform1i(texture_location, 1);

    // Set the render matrix
    float parameterMatrix[4][4];
    //parameterMatrix[0][0] = itime / 44100.0f;

    // Set parameters to other locations
    parameterMatrix[0][1] = params.getParam(2, 0.5f);
    parameterMatrix[0][2] = params.getParam(3, 0.5f);
    parameterMatrix[0][3] = params.getParam(4, 0.5f);

    parameterMatrix[1][0] = params.getParam(5, 0.5f);
    parameterMatrix[1][1] = params.getParam(6, 0.5f);
    parameterMatrix[1][2] = params.getParam(8, 0.5f);
    parameterMatrix[1][3] = params.getParam(9, 0.5f);

    parameterMatrix[2][0] = params.getParam(12, 0.5f);
    parameterMatrix[2][1] = params.getParam(13, 0.5f);
    parameterMatrix[2][2] = params.getParam(14, 0.5f);
    parameterMatrix[2][3] = params.getParam(15, 0.5f);

    parameterMatrix[3][0] = params.getParam(16, 0.5f);
    parameterMatrix[3][1] = params.getParam(17, 0.5f);
    parameterMatrix[3][2] = params.getParam(18, 0.5f);
    parameterMatrix[3][3] = params.getParam(19, 0.5f);

    int location = glGetUniformLocation(programID, "r");
    glUniformMatrix4fv(location, 1, GL_FALSE, &(parameterMatrix[0][0]));

    location = glGetUniformLocation(programID, "R");
    parameterMatrix[0][1] = params.getParam(20, 0.5f);
    parameterMatrix[0][2] = params.getParam(21, 0.5f);
    parameterMatrix[0][3] = params.getParam(22, 0.5f);
    glUniformMatrix4fv(location, 1, GL_FALSE, &(parameterMatrix[0][0]));

	// render to larger offscreen texture
	glActiveTexture(GL_TEXTURE0);
	textureManager.getTextureID("background.png", &textureID, errorText);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glActiveTexture(GL_TEXTURE1);
	textureManager.getTextureID(TM_NOISE3D_NAME, &textureID, errorText);
	glBindTexture(GL_TEXTURE_3D, textureID);
    glActiveTexture(GL_TEXTURE0);

    glViewport(0, 0, X_OFFSCREEN, Y_OFFSCREEN);

    GLuint loc = glGetUniformLocation(programID, "time");
    glUniform1f(loc, time);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_POINTS, 0, TOTAL_NUM_PARTICLES);
    //glColor4f(1.0f, 0.5f, 0.2f, 1.0f);
	//glRectf(-1.0, -1.0, 1.0, 1.0);

	// Copy backbuffer to texture
    textureManager.getTextureID(TM_OFFSCREEN_NAME, &textureID, errorText);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN);

	// Copy backbuffer to front (so far no improvement)
	int xres = window_rect.right - window_rect.left;
	int yres = window_rect.bottom - window_rect.top;
	glViewport(0, 0, xres, yres);
	if (false) {
		shaderManager.getProgramID("DitherTexture.gprg", &programID, errorText);
	} else {
		shaderManager.getProgramID("SimpleTexture.gprg", &programID, errorText);
	}
	glUseProgram(programID);

    glBindVertexArray(0); // Bind our Vertex Array Object so we can use it  
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Bind our Vertex Buffer Object  
}

int WINAPI WinMain( HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    WININFO     *info = &wininfo;

    info->hInstance = GetModuleHandle( 0 );

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

    INT64 last_ticks, ticks_per_second;
    QueryPerformanceCounter((LARGE_INTEGER *)&last_ticks);
    QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_second);
    float time = 0.0f;
    bool done = false;
    
    while (!done) {
        INT64 ticks;
        QueryPerformanceCounter((LARGE_INTEGER *)&ticks);
        QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_second);
        time += (float)((INT64)ticks - (INT64)last_ticks) / (float)ticks_per_second;
        last_ticks = ticks;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) done = true;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        intro_do(time);

        // Start the ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show a simple window.
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::Text("Current time: %f", time);
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
            ImGui::Checkbox("Preview Window", &show_preview_window);

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }
        // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
        if (show_preview_window)
        {
            char error_text[MAX_ERROR_LENGTH + 1];
            GLuint texture_id;
            ImGui::Begin("Preview Window", &show_preview_window,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
            if (textureManager.getTextureID(TM_OFFSCREEN_NAME, &texture_id, error_text)) {
                MessageBox(info->hWnd, error_text, "Get Renderbuffer", MB_OK);
                break;
            }
            ImGui::Image((ImTextureID)texture_id, ImVec2(XRES, YRES));
            ImGui::End();
        }
        // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
        if (show_demo_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
            ImGui::ShowDemoWindow(&show_demo_window);
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

		SwapBuffers(info->hDC);
	}    

    window_end(info);

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
