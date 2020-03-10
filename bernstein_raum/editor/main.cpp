// LiveCoding.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "main.h"
#include "Configuration.h"
#include "Error.h"
#include "glext.h"
#include "GLNames.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "Microphone.h"
#include "ProjectionRoom.h"
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
	 "glActiveTexture", "glGetUniformLocation", "glUniform1i", "glUniform1f", "glUniform3fv",
	 "glMultiTexCoord2f"
};

/*************************************************
 * The core managing units that hold the resources
 *************************************************/
ShaderManager shader_manager_;
TextureManager texture_manager_;
Microphone microphone_;
ProjectionRoom projection_room_;

/*************************************************
 * Window core variables
 *************************************************/
HINSTANCE hInst;								// Aktuelle Instanz
TCHAR szTitle[MAX_LOADSTRING];					// Titelleistentext
TCHAR szWindowClass[MAX_LOADSTRING];			// Klassenname des Hauptfensters

typedef struct
{
    //---------------
    HINSTANCE   hInstance;
    HDC         hDC[2];
    HGLRC       hRC[2];
    // First one is ImGUI, second will be fullscreen on the projector
    HWND        hWnd[2];
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

static WININFO wininfo = {  0,{0,0},{0,0},{0,0},
							{'l','c','_',0}
                            };

/*************************************************
 * OpenGL initialization
 *************************************************/
static int initGL(WININFO *win_info) {
	char errorString[MAX_ERROR_LENGTH + 1];

	// Create openGL functions
    for (int context = 0; context < 2; context++)
    {
        if (!wglMakeCurrent(win_info->hDC[context], win_info->hRC[context])) return(-1);

	    for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	    // Create and initialize the shader manager
	    if (shader_manager_.init(errorString)) {
		    MessageBox(win_info->hWnd[0], errorString, "Shader Manager Load", MB_OK);
		    return -1;
	    }

	    // Create and initialize everything needed for texture Management
	    if (texture_manager_.init(errorString)) {
		    MessageBox(win_info->hWnd[0], errorString, "Texture Manager Load", MB_OK);
		    return -1;
	    }
    }

    // imGUI initialization
    if (!wglMakeCurrent(win_info->hDC[0], win_info->hRC[0])) return(-1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplOpenGL2_Init();
    ImGui_ImplWin32_Init(win_info->hWnd[0]);
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
        RECT window_rect;
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
    
    if (info->hRC[0]) {
        wglMakeCurrent( 0, 0 );
        wglDeleteContext( info->hRC[0] );
    }
    if (info->hRC[1]) {
        wglMakeCurrent(0, 0);
        wglDeleteContext(info->hRC[1]);
    }

    if (info->hDC[0]) ReleaseDC(info->hWnd[0], info->hDC[0]);
    if (info->hDC[1]) ReleaseDC(info->hWnd[0], info->hDC[1]);
    if (info->hWnd[0]) DestroyWindow(info->hWnd[0]);
    if (info->hWnd[1]) DestroyWindow(info->hWnd[2]);

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

    for (int i = 0; i < 2; i++)
    {
        info->hWnd[i] = CreateWindowEx(dwExStyle, wc.lpszClassName, "Editor", dwStyle,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0, info->hInstance, 0 );
        if( !info->hWnd[0] )
            return( 0 );
        if( !(info->hDC[i]=GetDC(info->hWnd[i])) ) return( 0 );
        if( !(PixelFormat=ChoosePixelFormat(info->hDC[i], &pfd)) ) return( 0 );
        if( !SetPixelFormat(info->hDC[i], PixelFormat, &pfd) ) return( 0 );
        if( !(info->hRC[i]=wglCreateContext(info->hDC[i])) ) return( 0 );
        if( !wglMakeCurrent(info->hDC[i], info->hRC[i]) ) return( 0 );

        ShowWindow(info->hWnd[i], SW_SHOWDEFAULT);
        UpdateWindow(info->hWnd[i]);
    }
    
    return( 1 );
}

static void intro_do(float time, bool calibrate)
{
	char errorText[MAX_ERROR_LENGTH+1];
	GLuint textureID;

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

    glClearColor(0.7f, 0.5f , 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set the program uniforms
    GLuint programID;
    if (calibrate)
    {
        shader_manager_.getProgramID("calibration.gprg", &programID, errorText);
    }
    else
    {
        //shader_manager_.getProgramID("example.gprg", &programID, errorText);
        //shader_manager_.getProgramID("absolute_territory.gprg", &programID, errorText);
        shader_manager_.getProgramID("water_dragon.gprg", &programID, errorText);
    }
    glUseProgram(programID);

    // Set texture identifiers
	GLint texture_location;
    texture_location = glGetUniformLocation(programID, "BGTexture");
    glUniform1i(texture_location, 0);
    texture_location = glGetUniformLocation(programID, "Noise3DTexture");
	glUniform1i(texture_location, 1);

	// render to larger offscreen texture
	glActiveTexture(GL_TEXTURE0);
	texture_manager_.getTextureID("dragon2.png", &textureID, errorText);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glActiveTexture(GL_TEXTURE1);
	texture_manager_.getTextureID(TM_NOISE3D_NAME, &textureID, errorText);
	glBindTexture(GL_TEXTURE_3D, textureID);
    glActiveTexture(GL_TEXTURE0);

    GLuint loc = glGetUniformLocation(programID, "time");
    glUniform1f(loc, time);

    loc = glGetUniformLocation(programID, "center");
    if (loc >= 0)
    {
        float center[3] = {0.2f, 0.2f, 1.5f};
        center[0] = sinf(time * 0.3f);
        center[1] = sinf(time * 0.27f);
        center[2] = 2.0f * sinf(time * 0.21f) + 1.5f;
        glUniform3fv(loc, 1, center);
    }

    glColor4f(1.0f, 0.5f, 0.2f, 1.0f);
	//glRectf(-1.0, -1.0, 1.0, 1.0);

    projection_room_.RenderAll();

#if 0
    // Copy backbuffer to texture
    texture_manager_.getTextureID(TM_OFFSCREEN_NAME, &textureID, errorText);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN);

	// Copy backbuffer to front (so far no improvement)
    RECT window_rect;
    GetClientRect(hWnd, &window_rect);
	int xres = window_rect.right - window_rect.left;
	int yres = window_rect.bottom - window_rect.top;
	glViewport(0, 0, xres, yres);
	if (false) {
		shader_manager_.getProgramID("DitherTexture.gprg", &programID, errorText);
	} else {
		shader_manager_.getProgramID("SimpleTexture.gprg", &programID, errorText);
	}
#endif
	glUseProgram(programID);
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

    char error_text[MAX_ERROR_LENGTH + 1];
    if (!microphone_.Init(error_text))
    {
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
    bool show_preview_window = false;
    bool fullscreen = false;
    bool calibrate = false;
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

        if (!wglMakeCurrent(info->hDC[1], info->hRC[1])) return(0);
        RECT window_rect;
        GetClientRect(info->hWnd[1], &window_rect);
        glViewport(0, 0, window_rect.right - window_rect.left, abs(window_rect.bottom - window_rect.top));
        aspect_ratio_ = (float)(window_rect.right - window_rect.left) / (float)(abs(window_rect.bottom - window_rect.top));
        intro_do(time, calibrate);
        SwapBuffers(info->hDC[1]);

        // Start rendering the controls
        if (!wglMakeCurrent(info->hDC[0], info->hRC[0])) return(0);

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
            ImGui::Checkbox("Calibrate", &calibrate);

            bool old_fullscreen = fullscreen;
            ImGui::Checkbox("Fullscreen", &fullscreen);
            if (fullscreen && !old_fullscreen)
            {
                // TODO: Minimization again.
                SetWindowLong(info->hWnd[1], GWL_STYLE, WS_POPUP | WS_VISIBLE);
                ShowWindow(info->hWnd[1], SW_MAXIMIZE);
            }

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            // Show microphone loudness (and update it)
            float microphone_amplitude;
            char error_text[MAX_ERROR_LENGTH + 1];
            bool success = microphone_.GetAmplitude(&microphone_amplitude, error_text);
            if (success)
            {
                // Typically we would use ImVec2(-1.0f,0.0f) to use all available width, or ImVec2(width,0.0f) for a specified width. ImVec2(0.0f,0.0f) uses ItemWidth.
                ImGui::Text("Microphone amplitude %.1f", microphone_amplitude);
                // Default displaying the fraction as percentage string, but user can override it
                ImGui::ProgressBar((microphone_amplitude - 10.0f) / 20.0f, ImVec2(-1.0f, 0.0f), "");
            }
            else
            {
                ImGui::Text("Could not get microphone amplitude:\n %s", error_text);
            }
        }
        // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
        if (show_preview_window)
        {
            char error_text[MAX_ERROR_LENGTH + 1];
            GLuint texture_id;
            ImGui::Begin("Preview Window", &show_preview_window,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
            if (texture_manager_.getTextureID(TM_OFFSCREEN_NAME, &texture_id, error_text)) {
                MessageBox(info->hWnd[0], error_text, "Get Renderbuffer", MB_OK);
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

        projection_room_.ImGUIControl();

        // Rendering imgui
        ImGui::EndFrame();
        ImGui::Render();
        GetClientRect(info->hWnd[0], &window_rect);
        int w = window_rect.right - window_rect.left;
        int h = window_rect.bottom - window_rect.top;
        glViewport(0, 0, w, h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

		SwapBuffers(info->hDC[0]);
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
