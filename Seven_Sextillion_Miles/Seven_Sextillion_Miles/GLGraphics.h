#pragma once

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Teile der Windows-Header nicht einbinden.
#include <windows.h>
#include <GL/GL.h>

// Global strings (better in a resource file?)
#define GL_WINDOW_TITLE "Seven Sextillion Miles"

// Global error values
#define GL_REGISTER_CLASS_FAILED -1
#define GL_CREATE_WINDOW_FAILED -2
#define GL_GET_DC_FAILED -3
#define GL_CHOOSE_PIXEL_FORMAT_FAILED -4
#define GL_SET_PIXEL_FORMAT_FAILED -5
#define GL_CREATE_CONTEXT_FAILED -6
#define GL_MAKE_CURRENT_FAILED -7

class GLGraphics
{
public:
	GLGraphics(void);
	virtual ~GLGraphics(void);

public:
	// So far there is no resizing and no fullscreen.
	// So far no sensible return codes (except negative for error...)
	// The Graphics class also creates the window that is drawn to.
	// Problem: Window management is windows dependent -> no problem?
	int init(int width, int height, WNDPROC WndProc);

	// Resize the window. May return some negative error
	int resize(int width, int height);

	// Swap double buffer screens
	// May return negative error values
	int swap(void);

	// Clears screen with black. May return negative error values
	int clear(void);

private:
	bool wglExtensionSupported(const char *extension_name);

	HDC hDC;		// Private GDI Device Context
	HGLRC hRC;		// Permanent Rendering Context
	HWND hWnd;		// Holds Our Window Handle
};

