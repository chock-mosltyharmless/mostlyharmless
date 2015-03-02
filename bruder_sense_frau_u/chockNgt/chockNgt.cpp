// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//
#include "stdafx.h"
#include "chockNgt.h"
//#include "bass.h"
#include "mathhelpers.h"
#include "TextureManager.h"
#include "Configuration.h"
#include "Snippets.h"
#include "ScreenBorders.h"
#include "MovingPapers.h"

int X_OFFSCREEN = 512;
int Y_OFFSCREEN = 256;

#define SHOW_MOVING_PAPERS 1
#define SHOW_VIDEO 2
#define SHOW_FALLING_SNIPPETS 3
#define SHOW_INTRO 4


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

// music data
//HSTREAM mp3Str;

TextureManager textureManager;
Snippets snippets;
ScreenBorders screenBorders;
MovingPapers movingPapers;

// An indicator what is currently done
// 0: Move articles
// 1: Falling snippets
int whatIsShown = -1;

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
    wc.lpszClassName = "chockngt";

    RegisterClass (&wc);

	// Create the window
	//mainWnd = CreateWindow("chockngt","chockngt",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1024,768,0,0,hInstance,0);
	mainWnd = CreateWindow("chockngt","chockngt",WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
	
	RECT windowRect;
	GetWindowRect(mainWnd, &windowRect);
	X_OFFSCREEN = windowRect.right - windowRect.left;
	Y_OFFSCREEN = windowRect.bottom - windowRect.top;

	glInit();

    ShowWindow(mainWnd,SW_SHOW);
    UpdateWindow(mainWnd);
 
	long startTime = timeGetTime();
	long lastTime = 0;

	// start music playback
#if 0
	BASS_Init(-1,44100,0,mainWnd,NULL);
	mp3Str=BASS_StreamCreateFile(FALSE,"GT_muc.mp3",0,0,0);
	BASS_ChannelPlay(mp3Str, TRUE);
	BASS_Start();
#endif
	float fCurTime;
	GetAsyncKeyState(VK_ESCAPE);

	snippets.init();

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

		if (whatIsShown == SHOW_INTRO)
		{
			movingPapers.update(fDeltaTime, true);

			GLuint texID;
			int retVal = textureManager.getTextureID("intro.tga", &texID, errorString);
			if (retVal != 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
				return -1;
			}

			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, true, texID);
		}

		if (whatIsShown == SHOW_MOVING_PAPERS)
		{
			movingPapers.update(fDeltaTime, false);
			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, false, 0);
		}

		if (whatIsShown == SHOW_VIDEO)
		{
			movingPapers.update(fDeltaTime, true);

			GLuint texID;
			int retVal = textureManager.getVideoID("2-old.avi", &texID, errorString, (int)(fCurTime * 30.0f));
			if (retVal != 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get video ID", MB_OK);
				return -1;
			}

			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, true, texID);
		}

		if (whatIsShown == SHOW_FALLING_SNIPPETS)
		{
			// Draw the snippet stuff
			GLuint texID;
			if (textureManager.getTextureID("1.tga", &texID, errorString))
			{
				MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
				return -1;
			}
			glBindTexture(GL_TEXTURE_2D, texID);
			snippets.update(fDeltaTime);
			snippets.draw();
		}

		// draw background
		//drawQuad(-0.3f, 0.8f, -0.2f, 0.7f, 0.4f, 1.0f, 1.0f);
		//glEnable(GL_DEPTH_TEST);

		// Draw the black borders around the Schraenke
		screenBorders.drawBorders(&textureManager, mainWnd);

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
		case '1':
			whatIsShown = SHOW_INTRO;
			movingPapers.init();
			break;
		case 'l':
		case 'L':
			movingPapers.startDetaching();
			break;
		case 's':
		case 'S':
			movingPapers.stopFeeding();
			break;
		case '2':
			whatIsShown = SHOW_MOVING_PAPERS;
			movingPapers.init();
			break;
		case '3':
			whatIsShown = SHOW_VIDEO;
			movingPapers.init();
			break;
		case '4':
			whatIsShown = SHOW_FALLING_SNIPPETS;
			snippets.init();
			break;
		case '0':
			whatIsShown = -1;
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
