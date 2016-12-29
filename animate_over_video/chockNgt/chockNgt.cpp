// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//
#include "stdafx.h"
#include "chockNgt.h"
//#include "bass.h"
#include "TextureManager.h"
#include "Configuration.h"
#include "TextDisplay.h"
#include "Frame.h"

#include <vector>

int X_OFFSCREEN = XRES;
int Y_OFFSCREEN = YRES;

// The frame that is currently edited
int current_frame_ = 0;
bool show_shadow_ = false;  // Show some previous frames
bool show_video_ = true;  // Show the background video (instead of white?)
bool convert_to_gravity_ = false;  // Nilsify it

// Debug: There is just one global frame that can be edited.
std::vector<Frame>frames_;

// For textDisplay. May have to adjust somehow?
float aspectRatio = 1.2f;

// Zooming stuff
float zoom_amount_ = 1.0f;
float zoom_scroll_[2] = { 0.0f, 0.0f };
float last_mouse_pos_[2] = { 0.0f, 0.0f };

TextDisplay text_display_;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

static const PIXELFORMATDESCRIPTOR pfd =
{
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
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
// main window stuff
HDC mainDC;
HGLRC mainRC;
HWND mainWnd;
WNDCLASS mainWC;
HINSTANCE mainInstance;
const char *className = "hotparticle";

float fCurTime = 0.0f;

TextureManager textureManager;

int Save(const char *filename, char *error_string, bool fancy) {
    FILE *file;

    fopen_s(&file, filename, "wb");
    if (NULL == file) {
        snprintf(error_string, MAX_ERROR_LENGTH, "Could not open file %s", filename);
        return -1;
    }

    int num_elements = frames_.size();
    fwrite(&num_elements, sizeof(num_elements), 1, file);

    for (int i = 0; i < num_elements; i++) {
        int ret_val = 0;
        if (!fancy) {
            ret_val = frames_[i].Save(file, error_string);
        } else {
            ret_val = frames_[i].Export(file, error_string);
        }
        if (ret_val < 0) {
            fclose(file);
            return ret_val;
        }
    }

    fclose(file);
    return 0;
}

int Load(const char *filename, char *error_string) {
    FILE *file;

    fopen_s(&file, filename, "rb");
    if (NULL == file) {
        snprintf(error_string, MAX_ERROR_LENGTH, "Could not open file %s", filename);
        return -1;
    }

    int num_elements;
    fread(&num_elements, sizeof(num_elements), 1, file);

    for (int i = 0; i < num_elements; i++) {
        frames_.push_back(Frame());
        int ret_val = frames_[i].Load(file, error_string);
        if (ret_val < 0) {
            fclose(file);
            return ret_val;
        }
    }

    fclose(file);
    return 0;
}

static void window_end()
{
    if (mainRC) {
        wglMakeCurrent(0, 0);
        wglDeleteContext(mainRC);
    }

    if (mainDC) ReleaseDC(mainWnd, mainDC);
    if (mainWnd) DestroyWindow(mainWnd);

    UnregisterClass(className, mainWC.hInstance);

#ifdef FULLSCREEN
    ChangeDisplaySettings(0, 0);
#endif
}

static int window_init(bool use_custom_pixel_format = false, int custom_pixel_format = 0)
{
    unsigned int	PixelFormat;
    DWORD			dwExStyle, dwStyle;
    RECT			rec;

    ZeroMemory(&mainWC, sizeof(WNDCLASS));
    mainWC.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    mainWC.lpfnWndProc = WindowProc;
    mainWC.hInstance = mainInstance;
    mainWC.lpszClassName = className;

    if (!RegisterClass(&mainWC))
        return(0);

#ifdef FULLSCREEN
    DEVMODE			dmScreenSettings;
    dmScreenSettings.dmSize = sizeof(DEVMODE);
    dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    dmScreenSettings.dmBitsPerPel = 24;
    dmScreenSettings.dmPelsWidth = XRES;
    dmScreenSettings.dmPelsHeight = YRES;
    if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        return(0);
    dwExStyle = WS_EX_APPWINDOW;
    dwStyle = WS_VISIBLE | WS_POPUP;// | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
#else
    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
#endif

    rec.left = 0;
    rec.top = 0;
    rec.right = XRES;
    rec.bottom = YRES;
    AdjustWindowRect(&rec, dwStyle, 0);
#if 0
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = XRES;
    windowRect.bottom = YRES;
#endif

    mainWnd = CreateWindowEx(dwExStyle, mainWC.lpszClassName, "animate over video", dwStyle,
        (GetSystemMetrics(SM_CXSCREEN) - rec.right + rec.left) >> 1,
        (GetSystemMetrics(SM_CYSCREEN) - rec.bottom + rec.top) >> 1,
        rec.right - rec.left, rec.bottom - rec.top, 0, 0, mainInstance, 0);
    if (!mainWnd)
        return(0);

    if (!(mainDC = GetDC(mainWnd)))
        return(0);

    if (!use_custom_pixel_format) {
        if (!(PixelFormat = ChoosePixelFormat(mainDC, &pfd)))
            return(0);

        if (!SetPixelFormat(mainDC, PixelFormat, &pfd))
            return(0);
    }
    else {
        if (!SetPixelFormat(mainDC, custom_pixel_format, &pfd))
            return(0);
    }

    if (!(mainRC = wglCreateContext(mainDC)))
        return(0);

    if (!wglMakeCurrent(mainDC, mainRC))
        return(0);

    return(1);
}

void glInit()
{
    mainDC = GetDC(mainWnd);

    // NEHE MULTISAMPLE
    bool    arbMultisampleSupported = false;
    int arbMultisampleFormat = 0;
    // Get Our Pixel Format
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    HDC hDC = mainDC;
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
    }
    else {
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
        window_end();
        // Remove all messages...
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE));
        window_init(true, arbMultisampleFormat);
    }
    glEnable(GL_MULTISAMPLE_ARB);

	// Create and initialize everything needed for texture Management
	char errorString[MAX_ERROR_LENGTH+1];
	if (textureManager.init(errorString, mainDC))
	{
		MessageBox(mainWnd, errorString, "Texture Manager Load", MB_OK);
		return;
	}

    text_display_.init(&textureManager, errorString);

    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
}

void glUnInit()
{
	wglDeleteContext(mainRC);
	ReleaseDC(mainWnd, mainDC);
}

void DrawQuadColor(float startX, float endX, float startY, float endY,
                   float startU, float endU, float startV, float endV,
                   float red, float green, float blue, float alpha) {
    glColor4f(red, green, blue, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(startU, endV);
    glVertex3f(startX, endY, 0.99f);
    glTexCoord2f(endU, endV);
    glVertex3f(endX, endY, 0.99f);
    glTexCoord2f(endU, startV);
    glVertex3f(endX, startY, 0.99f);
    glTexCoord2f(startU, startV);
    glVertex3f(startX, startY, 0.99f);
    glEnd();
}

void DrawQuadColor(float startX, float endX, float startY, float endY, float red, float green, float blue, float alpha) {
    DrawQuadColor(startX, endX, startY, endY, 0.0f, 1.0f, 0.0f, 1.0f, red, green, blue, alpha);
}

void DrawQuad(float startX, float endX, float startY, float endY, float startU, float endU, float startV, float endV, float alpha) {
    DrawQuadColor(startX, endX, startY, endY, startU, endU, startV, endV, 1.0f, 1.0f, 1.0f, alpha);
}
void DrawQuad(float startX, float endX, float startY, float endY, float startV, float endV, float alpha) {
    DrawQuad(startX, endX, startY, endY, 0.0f, 1.0f, startV, endV, alpha);
}
void DrawQuad(float startX, float endX, float startY, float endY, float alpha) {
    DrawQuad(startX, endX, startY, endY, 0.0f, 1.0f, 0.0f, 1.0f, alpha);
}

//WinMain -- Main Window
int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow )
{
    MSG msg;
	msg.message = WM_CREATE;

    mainInstance = hInstance;

#if 0
    mainWC.style = CS_HREDRAW|CS_VREDRAW;
    mainWC.lpfnWndProc = WindowProc;
    mainWC.cbClsExtra = 0;
    mainWC.cbWndExtra = 0;
    mainWC.hInstance = hInstance;
    mainWC.hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
    mainWC.hCursor = LoadCursor (NULL, IDC_ARROW);
    mainWC.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
    mainWC.lpszMenuName = NULL;
    mainWC.lpszClassName = className;

    RegisterClass (&mainWC);

	// Create the window
#ifdef FULLSCREEN
    mainWnd = CreateWindow(mainWC.lpszClassName,"hot particle",WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
#else
	mainWnd = CreateWindow(mainWC.lpszClassName,"hot particle",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,XRES,YRES,0,0,hInstance,0);
#endif

#else
    if (!window_init()) {
        window_end();
        MessageBox(0, "window_init()!", "error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }
#endif
	
	RECT windowRect;
	GetClientRect(mainWnd, &windowRect);
	X_OFFSCREEN = windowRect.right - windowRect.left;
	Y_OFFSCREEN = windowRect.bottom - windowRect.top;

	glInit();

    // Initialize first frame
    frames_.push_back(Frame());

    ShowWindow(mainWnd,SW_SHOW);
    UpdateWindow(mainWnd);
 
	int startTime = timeGetTime();
	long lastTime = 0;

	// start music playback
	GetAsyncKeyState(VK_ESCAPE);

    const float kTransformationMatrixLeft[4][4] = {
        {0.5f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {-0.5f, 0.0f, 0.0f, 1.0f},
    };
    const float kTransformationMatrixRight[4][4] = {
        { 0.5f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { +0.5f, 0.0f, 0.0f, 1.0f },
    };

	do
    {
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			//if (!IsDialogMessage(mainWnd, &msg))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}
		if (msg.message == WM_QUIT) break; // early exit on quit

		// update timer
		long curTime = timeGetTime() - startTime;
		fCurTime = (float)curTime * 0.001f;
		long deltaTime = curTime - lastTime;
		float fDeltaTime = (float) deltaTime * 0.001f;
		lastTime = curTime;

		// render
		wglMakeCurrent(mainDC, mainRC);
		
		// Set the stuff to render to "rendertarget"
		//glViewport(0, 0, X_OFFSCREEN, Y_OFFSCREEN);

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
        //GLuint tex_id;

        glMatrixMode(GL_MODELVIEW);
        float transformation_matrix[4][4];
        for (int i = 0; i < 4 * 4; i++) {
            transformation_matrix[0][i] = kTransformationMatrixLeft[0][i];
        }
        transformation_matrix[0][0] *= zoom_amount_;
        transformation_matrix[1][1] *= zoom_amount_;
        transformation_matrix[3][0] += 0.5f * zoom_scroll_[0];
        transformation_matrix[3][1] += zoom_scroll_[1];
        glLoadMatrixf(transformation_matrix[0]);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        int return_value = 1;  // Assume we are finished

        // Check for error
        if (return_value < 0) return -1;

        // Draw video background
        float videoTime = fCurTime;
        GLuint texID;
        char errorString[MAX_ERROR_LENGTH + 1];

        if (show_video_) {
            if (textureManager.GetVideoFrameID("dance.wmv", &texID, errorString, current_frame_) < 0) {
                MessageBox(mainWnd, errorString, "Texture manager get video ID", MB_OK);
                return -1;
            }
            glBindTexture(GL_TEXTURE_2D, texID);
            DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f);
        }

        // DEBUG: Draw a frame
        for (int draw_frame = current_frame_ - 1; draw_frame <= current_frame_; draw_frame++) {
            if (draw_frame >= 0 && (draw_frame == current_frame_ || show_shadow_)) {
                float alpha = 1.0f - 0.3f * (current_frame_ - draw_frame);
                return_value = frames_[draw_frame].Draw(&textureManager, errorString, alpha);
                if (return_value < 0) {
                    MessageBox(mainWnd, errorString, "Could not draw frame", MB_OK);
                    return -1;
                }
            }
        }

        // Load unzoomed transformation matrix for frame number
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(kTransformationMatrixLeft[0]);
        // Show the frame number
        char frame_number_text[1024];
        sprintf_s(frame_number_text, sizeof(frame_number_text), "%d", current_frame_);
        text_display_.ShowText(0.0f, 0.0f, frame_number_text);

        // Draw the right frame without the video
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(kTransformationMatrixRight[0]);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        if (textureManager.getTextureID("black.png", &texID, errorString) < 0) {
            MessageBox(mainWnd, errorString, "Texture manager get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, texID);
        DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f);
        return_value = frames_[current_frame_].DrawFancy(&textureManager, errorString);
        if (return_value < 0) {
            MessageBox(mainWnd, errorString, "Could not fancy draw frame", MB_OK);
            return -1;
        }

        if (convert_to_gravity_) {
            // Get Backbuffer to image buffer
            GLuint texture_id;
            char error_text[MAX_ERROR_LENGTH + 1];
            unsigned char *pixels = new unsigned char[X_OFFSCREEN * Y_OFFSCREEN * 4];
            memset(pixels, 0xfe, X_OFFSCREEN * Y_OFFSCREEN * 4);
            textureManager.getTextureID(TM_OFFSCREEN_NAME, &texture_id, error_text);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);

            // Get an array of non-black pixels
            int num_set_pixels = 0;
            const int kMaxSetPixels = 1000000;
            int *pixel_pos_x = new int[kMaxSetPixels];
            int *pixel_pos_y = new int[kMaxSetPixels];
            float *pixel_intensity = new float[kMaxSetPixels];
            for (int y = 0; y < Y_OFFSCREEN; y++) {
                for (int x = X_OFFSCREEN / 2 + 1; x < X_OFFSCREEN; x++) {
                    int index = (y * X_OFFSCREEN + x) * 4;
                    if (pixels[index] > 0 && num_set_pixels < kMaxSetPixels) {
                        pixel_pos_x[num_set_pixels] = x;
                        pixel_pos_y[num_set_pixels] = y;
                        pixel_intensity[num_set_pixels] = pixels[index] / 255.0f;
                        num_set_pixels++;
                    }
                }
            }

            // TODO: Covert it to a direct approach with a pre-computed bitmap for the gravity computation
            // I might even think about using the GPU?
#if 0
            // Calculate gravity based on nearby pixels
            for (int y = 0; y < Y_OFFSCREEN; y++) {
                for (int x = X_OFFSCREEN / 2 + 1; x < X_OFFSCREEN; x++) {
                    float gravity_x = 0.0f;
                    float gravity_y = 0.0f;
                    for (int attractor = 0; attractor < num_set_pixels; attractor++) {
                        float distance = 0.0f;
                        distance += (x - pixel_pos_x[attractor]) * (x - pixel_pos_x[attractor]);
                        distance += (y - pixel_pos_y[attractor]) * (y - pixel_pos_y[attractor]);
                        distance = sqrtf(distance);
                        distance += 1.0f;  // strange normalization for inside pixel
                        float adjust = 1.0f / (distance * distance * distance);
                        float amount_x = (pixel_pos_x[attractor] - x) * adjust;
                        float amount_y = (pixel_pos_y[attractor] - y) * adjust;
                        float weight = pixel_intensity[attractor];
                        gravity_x += amount_x * weight;
                        gravity_y += amount_y * weight;
                    }

                    gravity_x *= 1.0f;
                    gravity_y *= 1.0f;
                    if (gravity_x < -1.0f) gravity_x = -1.0f;
                    if (gravity_y < -1.0f) gravity_y = -1.0f;
                    if (gravity_x > 1.0f) gravity_x = 1.0f;
                    if (gravity_y > 1.0f) gravity_y = 1.0f;
                    int index = (y * X_OFFSCREEN + x) * 4;
                    pixels[index] = (int)((gravity_x + 1.0f) * 0.5f * 255.0f);
                    pixels[index + 1] = (int)((gravity_y + 1.0f) * 0.5f * 255.0f);
                    pixels[index + 2] = 0;  // No for testing..
                }
            }
#else
            float (*gravity)[2] = new float[X_OFFSCREEN / 2 * Y_OFFSCREEN][2];
            const int radius = 80;
            for (int i = 0; i < X_OFFSCREEN / 2 * Y_OFFSCREEN; i++) {
                gravity[i][0] = 0.0f;
                gravity[i][1] = 0.0f;
            }
            for (int i = 0; i < num_set_pixels; i++) {
                for (int dy = -radius; dy <= radius; dy++) {
                    for (int dx = -radius; dx <= radius; dx++) {
                        if ((dy + pixel_pos_y[i] >= 0) &&
                            (dy + pixel_pos_y[i] < Y_OFFSCREEN) &&
                            (dx + pixel_pos_x[i] >= X_OFFSCREEN / 2) &&
                            (dx + pixel_pos_x[i] < X_OFFSCREEN)) {
                            float distance = 0.0f;
                            distance += dx * dx + dy * dy;
                            distance = sqrtf(distance);
                            distance += 1.0f;  // strange normalization for inside pixel
                            float adjust = 1.0f / (distance * distance * distance);
                            float amount_x = dx * adjust;
                            float amount_y = dy * adjust;
                            float weight = pixel_intensity[i];
                            int index = (pixel_pos_y[i] + dy) * X_OFFSCREEN / 2 + pixel_pos_x[i] + dx - X_OFFSCREEN / 2;
                            gravity[index][0] += amount_x * weight;
                            gravity[index][1] += amount_y * weight;
                        }
                    }
                }
            }
            // Visualize gravity
            for (int y = 0; y < Y_OFFSCREEN; y++) {
                for (int x = 0; x < X_OFFSCREEN / 2; x++) {
                    int index = y * X_OFFSCREEN / 2 + x;
                    int r = (int)(gravity[index][0] * 256.0f + 128.0f);
                    if (r < 0) r = 0;
                    if (r > 255) r = 255;
                    int g = (int)(gravity[index][1] * 256.0f + 128.0f);
                    if (g < 0) g = 0;
                    if (g > 255) g = 255;
                    int dest_index = 4 * (y * X_OFFSCREEN + x + X_OFFSCREEN / 2);
                    pixels[dest_index + 2] = r;
                    pixels[dest_index + 1] = g;
                    pixels[dest_index + 0] = 128;
                }
            }
            delete[] gravity;
#endif

            delete[] pixel_pos_x;
            delete[] pixel_pos_y;
            delete[] pixel_intensity;

            // Draw the pixels back
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
            delete[] pixels;
        }

		// swap buffers
		wglSwapLayerBuffers(mainDC, WGL_SWAP_MAIN_PLANE);

		//Sleep(5);
    //} while (msg.message != WM_QUIT && !GetAsyncKeyState(VK_ESCAPE));
    } while (msg.message != WM_QUIT);

	glUnInit();
	
    return msg.wParam;
}

//Main Window Procedure WindowProc
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    RECT windowRect;
    GetClientRect(mainWnd, &windowRect);
    float width = (float)(windowRect.right - windowRect.left);
    float height = (float)(windowRect.bottom - windowRect.top);
    float xp = 2.0f * float(x) / width * 2.0f - 1.0f;
    bool left_side = true;
    if (xp > 1.0f) {
        xp -= 2.0f;
        left_side = false;
    }
    float yp = 1.0f - float(y) / height * 2.0f;

    // Apply inverse zoom
    if (left_side) {
        xp /= zoom_amount_;
        yp /= zoom_amount_;
        xp -= zoom_scroll_[0] / zoom_amount_;
        yp -= zoom_scroll_[1] / zoom_amount_;
    }

    char error_string[MAX_ERROR_LENGTH + 1];

    switch (message)                  /* handle the messages */
    {
    case WM_DESTROY:
        PostQuitMessage(0);   /* send a WM_QUIT to the message queue */
        break;
 
    case WM_LBUTTONDOWN:
        last_mouse_pos_[0] = xp;
        last_mouse_pos_[1] = yp;
        frames_[current_frame_].StartNewLine();
        frames_[current_frame_].AddLineNode(xp, yp, false);
        break;

    case WM_LBUTTONUP:
        last_mouse_pos_[0] = xp;
        last_mouse_pos_[1] = yp;
        frames_[current_frame_].AddLineNode(xp, yp, true);
        break;

    case WM_MOUSEMOVE:
        last_mouse_pos_[0] = xp;
        last_mouse_pos_[1] = yp;
        if (wParam & MK_LBUTTON) {
            frames_[current_frame_].AddLineNode(xp, yp, false);
        }
        break;

    case WM_KEYDOWN:
        switch (wParam) {
        case 'o':
        case 'O':
        case 'z':
        case 'Z':
            current_frame_--;
            if (current_frame_ < 0) current_frame_ = 0;
            while (current_frame_ >= (int)frames_.size()) {
                frames_.push_back(Frame());
            }
            break;
        case 'p':
        case 'P':
        case 'x':
        case 'X':
            current_frame_++;
            while (current_frame_ >= (int)frames_.size()) {
                frames_.push_back(Frame());
            }
            break;
        case 'c':  // c: Delete last line of frame
        case 'C':
            frames_[current_frame_].DeleteLastLine();
            break;
        case 'm':
        case 'M':
            show_shadow_ = !show_shadow_;
            break;
        case 'n':
        case 'N':
            show_video_ = !show_video_;
            break;
        case 't':
        case 'T':
            convert_to_gravity_ = !convert_to_gravity_;
            break;
        case 's':
        case 'S':
            if (Save("savefile.frames", error_string, false) < 0) {
                MessageBox(mainWnd, error_string, "Save", MB_OK);
            }
            break;
        case 'e':
        case 'E':
            if (Save("export.frames", error_string, true) < 0) {
                MessageBox(mainWnd, error_string, "Export", MB_OK);
            }
            break;
        case'l':
        case'L':
            if (Load("savefile.frames", error_string) < 0) {
                MessageBox(mainWnd, error_string, "Load", MB_OK);
            }
            break;
        
        case 'q':
        case 'Q':
            zoom_amount_ *= 3.0f / 4.0f;
            zoom_scroll_[0] = -last_mouse_pos_[0] * zoom_amount_;
            zoom_scroll_[1] = -last_mouse_pos_[1] * zoom_amount_;
            if (zoom_amount_ < 1.0f) {
                zoom_amount_ = 1.0f;
                zoom_scroll_[0] = 0.0f;
                zoom_scroll_[1] = 0.0f;
            }
            break;
        case 'w':
        case 'W':
            zoom_amount_ *= 4.0f / 3.0f;
            if (zoom_amount_ > 4.0f) zoom_amount_ = 4.0f;
            zoom_scroll_[0] = -last_mouse_pos_[0] * zoom_amount_;
            zoom_scroll_[1] = -last_mouse_pos_[1] * zoom_amount_;
            break;
        }        

    default:              /* for messages that we don't deal with */
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
