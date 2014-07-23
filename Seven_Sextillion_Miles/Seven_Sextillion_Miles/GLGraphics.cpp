#include "StdAfx.h"
#include "GLGraphics.h"
#include "wglext.h"
#include "global.h"

GLGraphics::GLGraphics(void)
{
}


GLGraphics::~GLGraphics(void)
{
}

void GLGraphics::drawSprite(int xOrigin, int yOrigin, float xPos, float yPos,
							float width, float height, float rotation)
{
	// I could save those in the class if I really wanted:
	float xScale = 1.0f;
	float yScale = 1.0f;

	if (viewportWidth < viewportHeight)
	{
		yScale = (float)viewportWidth / (float)viewportHeight;
	}
	else
	{
		xScale = (float)viewportHeight / (float)viewportWidth;
	}

	float centerX, centerY;

	switch(xOrigin)
	{
	case -1:
		centerX = xPos * xScale - 1.0f;
		break;
	case 0:
		centerX = xPos * xScale;
		break;
	case 1:
		centerX = 1.0f - xPos * xScale;
		break;
	default:
		centerX = 0.0f;
		break;
	}

	switch(yOrigin)
	{
	case -1:
		centerY = yPos * yScale - 1.0f;
		break;
	case 0:
		centerY = yPos * yScale;
		break;
	case 1:
		centerY = 1.0f - yPos * yScale;
		break;
	default:
		centerY = 0.0f;
		break;
	}

	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 2; x++)
		{
			float xMod = (float)x;
			float yMod = (float)y;
			if (y == 1) xMod = 1.0f - xMod;

			float xd = ((float)xMod - 0.5f) * width * 2.0f;
			float yd = ((float)yMod - 0.5f) * height * 2.0f;

			glTexCoord2d(xMod, 1.0f - yMod);
			glVertex2f(centerX + xScale * (cos(rotation)*xd - sin(rotation) * yd),
				       centerY + yScale * (sin(rotation)*xd + cos(rotation) * yd));
		}
	}
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
	wc.lpszClassName	= "SevenSextillionMiles";				// Set The Class Name

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "Failed To Register The Window Class.", "OpenGL ERROR",
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
						      "SevenSextillionMiles",				// Class Name
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
		MessageBox(NULL, "Window Creation Error.", "OpenGL ERROR",
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
		MessageBox(NULL, "Can't Create A GL Device Context.", "OpenGL ERROR",
				   MB_OK | MB_ICONEXCLAMATION);
		return GL_GET_DC_FAILED;
	}

	GLuint pixelFormat;
	if (!(pixelFormat = ChoosePixelFormat(hDC, &pfd)))
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Can't Find A Suitable PixelFormat.", "OpenGL ERROR",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_CHOOSE_PIXEL_FORMAT_FAILED;
	}

	if(!SetPixelFormat(hDC, pixelFormat, &pfd))
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Can't Set The PixelFormat.", "OpenGL ERROR",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_SET_PIXEL_FORMAT_FAILED;
	}

	if (!(hRC = wglCreateContext(hDC)))
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Can't Create A GL Rendering Context.", "OpenGL ERROR",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_CREATE_CONTEXT_FAILED;
	}

	if(!wglMakeCurrent(hDC, hRC))
	{
		//KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Can't Activate The GL Rendering Context.", "OpenGL ERROR",
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
	char errorString[MAX_ERROR_LENGTH + 1];
	glEnable(GL_TEXTURE_2D);
	if (textures.init(errorString) != 0)
	{
		MessageBox(NULL, errorString, "Texture Load error",
			       MB_OK | MB_ICONEXCLAMATION);
		return GL_LOAD_TEXTURES_FAILED;
	}

	return 0;
}

void GLGraphics::handleError(const char *errorString)
{
	MessageBox(NULL, errorString, "ERROR",
			    MB_OK | MB_ICONEXCLAMATION);
}

int GLGraphics::resize(int width, int height)
{
	glViewport(0, 0, width, height);				// Reset The Current Viewport
	viewportWidth = width;
	viewportHeight = height;

	return 0;
}

int GLGraphics::clear(void)
{
	glClear(GL_COLOR_BUFFER_BIT);

	// Standard transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	return 0;
}

int GLGraphics::swap(void)
{
	SwapBuffers(hDC);

	return 0;
}
