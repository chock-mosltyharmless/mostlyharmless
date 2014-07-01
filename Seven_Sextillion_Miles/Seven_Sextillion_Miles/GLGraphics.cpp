#include "StdAfx.h"
#include "GLGraphics.h"
#include "wglext.h"

GLGraphics::GLGraphics(void)
{
}


GLGraphics::~GLGraphics(void)
{
}

bool GLGraphics::wglExtensionSupported(const char *extension_name)
{
    // this is pointer to function which returns pointer to string with list of all wgl extensions
    PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;

    // determine pointer to wglGetExtensionsStringEXT function
    _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");

    if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
    {
        // string was not found
        return false;
    }

    // extension is supported
    return true;
}

int GLGraphics::init(int width, int height, WNDPROC WndProc)
{
	WNDCLASS wc;						// Windows Class Structure
	HINSTANCE hInstance =  GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC)WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= L"SevenSextillionMiles";				// Set The Class Name

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, L"Failed To Register The Window Class.", L"OpenGL ERROR",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_REGISTER_CLASS_FAILED;
	}

	DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD dwStyle = WS_OVERLAPPEDWINDOW;

	RECT windowRect; // Desired size of the window (no fullscreen)
	windowRect.left=(long)0;
	windowRect.right=(long)width;
	windowRect.top=(long)0;
	windowRect.bottom=(long)height;
	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(dwExStyle,
						      L"SevenSextillionMiles",				// Class Name
							  GL_WINDOW_TITLE,					    // Window Title
							  dwStyle |							    // Defined Window Style
							  WS_CLIPSIBLINGS |	WS_CLIPCHILDREN,	// Required Window Style
							  CW_USEDEFAULT, CW_USEDEFAULT,			// Window Position
							  windowRect.right-windowRect.left,		// Calculate Window Width
							  windowRect.bottom-windowRect.top,		// Calculate Window Height
							  NULL,									// No Parent Window
							  NULL,									// No Menu
							  hInstance,							// Instance
						      NULL)))								// Dont Pass Anything To WM_CREATE
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, L"Window Creation Error.", L"OpenGL ERROR",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_CREATE_WINDOW_FAILED;
	}

	static PIXELFORMATDESCRIPTOR pfd =				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,											// Version Number
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,								// Request An RGBA Format
		24,											// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		0,											// 16Bit Z-Buffer (no Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	if (!(hDC = GetDC(hWnd)))
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, L"Can't Create A GL Device Context.", L"OpenGL ERROR",
				   MB_OK | MB_ICONEXCLAMATION);
		return GL_GET_DC_FAILED;
	}

	GLuint pixelFormat;
	if (!(pixelFormat = ChoosePixelFormat(hDC, &pfd)))
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, L"Can't Find A Suitable PixelFormat.", L"OpenGL ERROR",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_CHOOSE_PIXEL_FORMAT_FAILED;
	}

	if(!SetPixelFormat(hDC, pixelFormat, &pfd))
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, L"Can't Set The PixelFormat.", L"OpenGL ERROR",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_SET_PIXEL_FORMAT_FAILED;
	}

	if (!(hRC = wglCreateContext(hDC)))
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, L"Can't Create A GL Rendering Context.", L"OpenGL ERROR",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_CREATE_CONTEXT_FAILED;
	}

	if(!wglMakeCurrent(hDC, hRC))
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, L"Can't Activate The GL Rendering Context.", L"OpenGL ERROR",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_MAKE_CURRENT_FAILED;
	}

	// Try to change the swap control
	PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT = NULL;
	PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT = NULL;

	if (wglExtensionSupported("WGL_EXT_swap_control"))
	{
		// Extension is supported, init pointers.
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		// this is another function from WGL_EXT_swap_control extension
		wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

		// Set VSync to 1
		wglSwapIntervalEXT(1);
	}

	ShowWindow(hWnd, SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	resize(width, height);							// Set Up Our Perspective GL Screen

	return 0;
}

int GLGraphics::resize(int width, int height)
{
	glViewport(0, 0, width, height);				// Reset The Current Viewport

	return 0;
}

int GLGraphics::clear(void)
{
	glClear(GL_COLOR_BUFFER_BIT);

	return 0;
}

int GLGraphics::swap(void)
{
	SwapBuffers(hDC);

	return 0;
}
