// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//
#include "stdafx.h"
#include "chockNgt.h"
//#include "bass.h"
#include "mathhelpers.h"
#include "TextureManager.h"
#include "Configuration.h"

int X_OFFSCREEN = 512;
int Y_OFFSCREEN = 256;

LRESULT CALLBACK WindowProc (HWND, UINT, WPARAM, LPARAM);

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

static const PIXELFORMATDESCRIPTOR pfd =
    {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32,
    0, 0, 0, 0, 0, 0, 8, 0,
    0, 0, 0, 0, 0,
    32, 0, 0,
    PFD_MAIN_PLANE,
    0, 0, 0, 0
    };
// main window stuff
HDC mainDC;
HGLRC mainRC;
HWND mainWnd;
static GLuint creditsTexture;
static int *creditsTexData[1024*1024];

TextureManager textureManager;

void glInit()
{
	mainDC = GetDC(mainWnd);
    if( !SetPixelFormat(mainDC,ChoosePixelFormat(mainDC,&pfd),&pfd) ) return;
    mainRC = wglCreateContext(mainDC);
    wglMakeCurrent(mainDC, mainRC);

	// Create and initialize everything needed for texture Management
	char errorString[MAX_ERROR_LENGTH+1];
	if (textureManager.init(errorString, mainDC))
	{
		MessageBox(mainWnd, errorString, "Texture Manager Load", MB_OK);
		return;
	}
}

void glUnInit()
{
	wglDeleteContext(mainRC);
	ReleaseDC(mainWnd, mainDC);
}


void drawQuad(float startX, float endX, float startY, float endY, float startV, float endV, float alpha)
{
		// set up matrices
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glEnable(GL_TEXTURE_2D);
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);

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
	glDisable(GL_TEXTURE_2D);
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
}

//WinMain -- Main Window
int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow )
{
	char errorString[MAX_ERROR_LENGTH + 1];

    MSG msg;
	msg.message = WM_CREATE;

    WNDCLASS wc;
    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "hotparticle";

    RegisterClass (&wc);

	// Create the window
#ifdef FULLSCREEN
    //mainWnd = CreateWindow(wc.lpszClassName,"hot particle",WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
#else
	mainWnd = CreateWindow(wc.lpszClassName,"hot particle",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1024,768,0,0,hInstance,0);
#endif
	
	RECT windowRect;
	GetWindowRect(mainWnd, &windowRect);
	X_OFFSCREEN = windowRect.right - windowRect.left;
	Y_OFFSCREEN = windowRect.bottom - windowRect.top;

	glInit();

    ShowWindow(mainWnd,SW_SHOW);
    UpdateWindow(mainWnd);
 
	int startTime = timeGetTime();
	long lastTime = 0;

	// start music playback
	float fCurTime;
	GetAsyncKeyState(VK_ESCAPE);

	do
    {
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (!IsDialogMessage(mainWnd, &msg))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}
		if (msg.message == WM_QUIT) break; // early exit on quit

		ShowCursor(FALSE);

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

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
        GLuint tex_id;

        // Draw example room
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        if (textureManager.getTextureID("zimmer_layer_1.tga", &tex_id, errorString)) {
            MessageBox(mainWnd, errorString, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        drawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
        if (textureManager.getTextureID("zimmer_layer_2.tga", &tex_id, errorString)) {
            MessageBox(mainWnd, errorString, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        drawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        if (textureManager.getTextureID("zimmer_layer_3.tga", &tex_id, errorString)) {
            MessageBox(mainWnd, errorString, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        drawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
        if (textureManager.getTextureID("vignette.tga", &tex_id, errorString)) {
            MessageBox(mainWnd, errorString, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        drawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f);

#if 0
        // Draw video background
        float videoTime = fCurTime;
        if (textureManager.getVideoID("Kenshiro20_vonoben.wmv", &texID, errorString, videoTime) < 0) {
            MessageBox(mainWnd, errorString, "Texture manager get video ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, texID);
        drawQuad(-0.5f, 0.5f, 0.7f, -0.7f, 0.0f, 1.0f, 1.0f);

		//if (whatIsShown == SHOW_ENDING)
		{
            float end_start_time = 1.0f;
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			// Draw icon
			if (textureManager.getTextureID("icon.tga", &texID, errorString))
			{
				MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
				return -1;
			}
			glBindTexture(GL_TEXTURE_2D, texID);
			float alpha;
			if (fCurTime - end_start_time < 0.4f) alpha = 0.0f;
			else alpha = 1.0f;
			drawQuad(-0.5f, 0.5f, -0.5f, 0.91f, 0.0f, 1.0f, alpha);

			// Draw first highlight
			if (textureManager.getTextureID("icon_highlight1.tga", &texID, errorString))
			{
				MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
				return -1;
			}
			glBindTexture(GL_TEXTURE_2D, texID);
			if (fCurTime - end_start_time < 0.3f) alpha = (fCurTime - end_start_time) / 0.3f;
			else alpha = 1.1f - (fCurTime - end_start_time - 0.3f) * 0.5f;
			if (alpha < 0.0f) alpha = 0.0f;
			if (alpha > 1.0f) alpha = 1.0f;
			alpha = 0.5f - 0.5f * (float)cos(alpha * 3.14159);
			drawQuad(-0.5f, 0.5f, -0.5f, 0.91f, 0.0f, 1.0f, alpha*0.75f);

			// Draw second highlight
			if (textureManager.getTextureID("icon_highlight2.tga", &texID, errorString))
			{
				MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
				return -1;
			}
			glBindTexture(GL_TEXTURE_2D, texID);
			if (fCurTime - end_start_time < 0.4f) alpha = (fCurTime - end_start_time - 0.3f) / 0.1f;
			else alpha = 1.2f - (fCurTime - end_start_time - 0.4f) * 1.2f;
			if (alpha < 0.0f) alpha = 0.0f;
			if (alpha > 1.0f) alpha = 1.0f;
			alpha = 0.5f - 0.5f * (float)cos(alpha * 3.14159);
			alpha *= 0.75f;
			drawQuad(-0.5f, 0.5f, -0.5f, 0.91f, 0.0f, 1.0f, alpha*0.75f);

			// draw some sparkles
			if (textureManager.getTextureID("sparkle.tga", &texID, errorString))
			{
				MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
				return -1;
			}
			glBindTexture(GL_TEXTURE_2D, texID);
			float sparkleTime = (fCurTime - end_start_time - 0.4f) * 0.4f;
			for (int i = 0; i < 16; i++)
			{
				float sparkleDuration = 1.3f + 0.4f * sinf(i*2.4f+2.3f);
				if (sparkleTime > 0.0f && sparkleTime < sparkleDuration)
				{
					float amount = sqrtf(sinf((sparkleTime / sparkleDuration * 3.1415f)));
					float iconDistance = 0.5f;
					float ASPECT_RATIO = 240.0f / 170.0f;
					float centerX = -0.3f + iconDistance * (0.55f + 0.35f * sinf(i*2.1f + 7.3f));
					centerX += (0.7f+0.15f*sinf(i*1.4f+8.3f)) * iconDistance / sparkleDuration * sparkleTime -
							   0.1f * sparkleTime*sparkleTime/sparkleDuration/sparkleDuration;
					float centerY = 0.5f + iconDistance * ASPECT_RATIO * (0.8f + 0.3f * sinf(i*4.6f + 2.9f) - 1.0f);
					centerY += (0.5f+0.2f*sinf(i*6.8f+3.0f)) * iconDistance / sparkleDuration * sparkleTime * ASPECT_RATIO -
							   0.4f * sparkleTime*sparkleTime/sparkleDuration/sparkleDuration;
					float width = iconDistance * 0.25f;
					drawQuad(centerX - width, centerX + width,
							 centerY - width * ASPECT_RATIO, centerY + width * ASPECT_RATIO,
							 0.0f, 1.0f, amount);
				}
			}


			glDisable(GL_BLEND);
		}
#endif

		// draw background
		//drawQuad(-0.3f, 0.8f, -0.2f, 0.7f, 0.4f, 1.0f, 1.0f);
		//glEnable(GL_DEPTH_TEST);

		// Draw the black borders around the Schraenke
		//screenBorders.drawBorders(&textureManager, mainWnd, showBlue, fadeInTime * fadeOut, redenner);

		// swap buffers
		wglSwapLayerBuffers(mainDC, WGL_SWAP_MAIN_PLANE);

		//Sleep(5);
    } while (msg.message != WM_QUIT && !GetAsyncKeyState(VK_ESCAPE));

	// music uninit
#if 0
	BASS_ChannelStop(mp3Str);
	BASS_StreamFree(mp3Str);
	BASS_Free();
#endif

	glUnInit();
	
    return msg.wParam;
}

//Main Window Procedure WindowProc
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

	switch (message)                  /* handle the messages */
    {

    case WM_SYSCOMMAND:
      switch (wParam)
      {
        case SC_SCREENSAVE:  
          return 0;
        case SC_MONITORPOWER:
          return 0;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
      }
      break;

	case WM_KEYDOWN:
		switch(wParam)
		{
			// change what is shown
		case 'o':
		case 'O':
			PlaySound("textures/silence.wav", NULL, SND_ASYNC);
			break;
		default:
			break;
		}
		break;

	case WM_CREATE:
        break;
 
    case WM_DESTROY:
        PostQuitMessage(0);   /* send a WM_QUIT to the message queue */
        break;
 
    default:              /* for messages that we don't deal with */
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
