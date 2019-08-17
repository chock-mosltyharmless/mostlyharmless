// LiveCoding.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "main.h"
#include "Configuration.h"
#include "glext.h"
#include "GLNames.h"
#include "ShaderManager.h"
#include "TextureManager.h"
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

    glClearColor(0.7f, 0.5f , 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

	// Set the program uniforms
	GLuint programID;
	shaderManager.getProgramID("example.gprg", &programID, errorText);
	glUseProgram(programID);

    // Set texture identifiers
	GLint texture_location;
    texture_location = glGetUniformLocation(programID, "BGTexture");
    glUniform1i(texture_location, 0);
    texture_location = glGetUniformLocation(programID, "Noise3DTexture");
	glUniform1i(texture_location, 1);

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

    glColor4f(1.0f, 0.5f, 0.2f, 1.0f);
	glRectf(-1.0, -1.0, 1.0, 1.0);

#if 1
    for (int i = 0; i < 2; i++) {
	    // Copy backbuffer to texture
        glActiveTexture(GL_TEXTURE0);
        textureManager.getTextureID(TM_OFFSCREEN_NAME, &textureID, errorText);
	    glBindTexture(GL_TEXTURE_2D, textureID);
	    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN);

        shaderManager.getProgramID("Distance.gprg", &programID, errorText);
	    glUseProgram(programID);

        GLint uniform_location = glGetUniformLocation(programID, "aspect_ratio_step");
        glUniform1f(uniform_location, 1.0f / 255.0f);
        if (i == 0)
        {
            uniform_location = glGetUniformLocation(programID, "x_step");
            glUniform1f(uniform_location, 1.0f / X_OFFSCREEN);
            uniform_location = glGetUniformLocation(programID, "y_step");
            glUniform1f(uniform_location, 0.0f);
        }
        else
        {
            uniform_location = glGetUniformLocation(programID, "x_step");
            glUniform1f(uniform_location, 0.0f);
            uniform_location = glGetUniformLocation(programID, "y_step");
            glUniform1f(uniform_location, 1.0f / Y_OFFSCREEN);
        }

        // render to larger offscreen texture
        glColor4f(1.0f, 0.5f, 0.5f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, -1.0f);
        glEnd();
    }
#endif

    // Copy backbuffer to texture
    textureManager.getTextureID(TM_OFFSCREEN_NAME, &textureID, errorText);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN);
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
