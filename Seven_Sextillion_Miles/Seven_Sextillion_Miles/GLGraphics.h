#pragma once

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Teile der Windows-Header nicht einbinden.
#include <windows.h>
#include <GL/GL.h>
#include "GLTextures.h"

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
#define GL_LOAD_TEXTURES_FAILED -8

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

	// Stop rendering (quads) and swap double buffer screens
	// May return negative error values
	int swap(void);

	// Clears screen with black. May return negative error values
	// This also initializes rendering (glBegin(GL_QUADS));
	int clear(void);

	/// Call this to begin the rendering process of one "state"
	/// E.g. one texture/material/rendering type thingie
	void beginRendering(void) { glBegin(GL_QUADS); }

	/// Call this at the end of a rendering process (see beginRendering)
	void endRendering(void) { glEnd(); }

	// Get the texture ID from a named texture
	// That might either be a .png or any of the special textures
	// Returns 0 if successful, -1 otherwise
	// The error string must hold space for at least MAX_ERROR_LENGTH
	// characters.
	int setTexture(const char *name, char *errorString) {
		unsigned int texID;
		if (textures.getTextureID(name, &texID, errorString) == 0) {
			glBindTexture(GL_TEXTURE_2D, texID);
			return 0;
		}
		else return -1;
	}

	// xOrigin is -1 for left, 0 for center, 1 for right
	// yOrigin is -1 for bottom, 0 for center, 1 for top
	// For a negative origin, the position direction is inverted
	// ?Pos, width and height are relative to the shorter length of the viewport.
	void drawSprite(int xOrigin, int yOrigin, float xPos, float yPos,
					float width, float height, float rotation);

	// Display an error and wait for user to acknowledge it
	void handleError(const char *errorString);

private:
	bool wglExtensionSupported(const char *extension_name);

	HDC hDC;		// Private GDI Device Context
	HGLRC hRC;		// Permanent Rendering Context
	HWND hWnd;		// Holds Our Window Handle

	// Size of the screen viewport
	int viewportWidth;
	int viewportHeight;

	// Texture manager
	GLTextures textures;
};

