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
#define SHOW_FALLING_SNIPPETS 3
#define SHOW_INTRO 4
#define SHOW_VIDEO_1 11
#define SHOW_VIDEO_2 12
#define SHOW_VIDEO_3 13
#define SHOW_VIDEO_4 14
#define SHOW_VIDEO_5 15
#define SHOW_VIDEO_6 16
#define SHOW_VIDEO_7 17

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
float videoStartTime = 0.0f;
long startTime;
bool notYetDetached = true;

// An indicator what is currently done
// 0: Move articles
// 1: Falling snippets
int whatIsShown = -1;
bool showBlue = false;
float fadeInTime; // Put to 0 to start fading in...

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
	mainWnd = CreateWindow("chockngt","chockngt",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1024,768,0,0,hInstance,0);
	//mainWnd = CreateWindow("chockngt","chockngt",WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
	
	RECT windowRect;
	GetWindowRect(mainWnd, &windowRect);
	X_OFFSCREEN = windowRect.right - windowRect.left;
	Y_OFFSCREEN = windowRect.bottom - windowRect.top;

	glInit();

    ShowWindow(mainWnd,SW_SHOW);
    UpdateWindow(mainWnd);
 
	startTime = timeGetTime();
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

	fadeInTime = 1.0f; // fully faded in.
	do
    {
		float fadeOut = 1.0f; // fully faded in.
		float redenner = 0.0f; // How much red in there

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

		fadeInTime += fDeltaTime * 3.0f;
		if (fadeInTime > 1.0f) fadeInTime = 1.0f;

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

		if (whatIsShown == SHOW_VIDEO_1)
		{
			movingPapers.update(fDeltaTime, true);
			GLuint texID;
			int retVal = textureManager.getVideoID("01.avi", &texID, errorString, (int)((fCurTime - videoStartTime) * 29.98f));
			if (retVal < 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get video ID", MB_OK);
				return -1;
			}
			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, true, texID);
		}
		if (whatIsShown == SHOW_VIDEO_2)
		{
			movingPapers.update(fDeltaTime, true);
			GLuint texID;
			int retVal = textureManager.getVideoID("02.avi", &texID, errorString, (int)((fCurTime - videoStartTime) * 29.98f));
			if (retVal < 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get video ID", MB_OK);
				return -1;
			}
			if (retVal > 0) showBlue = false;
			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, true, texID);
		}
		if (whatIsShown == SHOW_VIDEO_3)
		{
			movingPapers.update(fDeltaTime, true);
			GLuint texID;
			int retVal = textureManager.getVideoID("03.avi", &texID, errorString, (int)((fCurTime - videoStartTime) * 29.98f));
			if (retVal < 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get video ID", MB_OK);
				return -1;
			}
			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, true, texID);
		}
		if (whatIsShown == SHOW_VIDEO_4)
		{
			movingPapers.update(fDeltaTime, true);
			GLuint texID;
			int retVal = textureManager.getVideoID("04.avi", &texID, errorString, (int)((fCurTime - videoStartTime) * 29.98f));
			if (retVal < 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get video ID", MB_OK);
				return -1;
			}
			if (retVal > 0) showBlue = false;
			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, true, texID);
		}
		if (whatIsShown == SHOW_VIDEO_5)
		{
			movingPapers.update(fDeltaTime, true);
			GLuint texID;
			int retVal = textureManager.getVideoID("05.avi", &texID, errorString, (int)((fCurTime - videoStartTime) * 29.98f));
			if (retVal < 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get video ID", MB_OK);
				return -1;
			}
			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, true, texID);
		}
		if (whatIsShown == SHOW_VIDEO_6)
		{
			movingPapers.update(fDeltaTime, true);
			GLuint texID;
			int retVal = textureManager.getVideoID("06.avi", &texID, errorString, (int)((fCurTime - videoStartTime) * 29.98f));
			if (retVal < 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get video ID", MB_OK);
				return -1;
			}
			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, true, texID);
		}
		if (whatIsShown == SHOW_VIDEO_7)
		{
			movingPapers.update(fDeltaTime, true);
			GLuint texID;
			int retVal = textureManager.getVideoID("05.avi", &texID, errorString, (int)((fCurTime - videoStartTime) * 29.98f));
			if (fCurTime - videoStartTime > 43.0f && notYetDetached)
			{
				movingPapers.startDetaching(4);
				notYetDetached = false;
			}
			if (retVal < 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get video ID", MB_OK);
				return -1;
			}
			// Draw the newspaper moving thing
			movingPapers.draw(mainWnd, &textureManager, true, texID);

			// Fade everything out at the end
			if (fCurTime - videoStartTime > 3.0f*60.0f + 45.0f)
			{
				fadeOut = (3.0f*60.0f + 45.0f - (fCurTime - videoStartTime)) / 10.0f + 1.0f;
				if (fadeOut < 0.0f) fadeOut = 0.0f;
			}
			if (fCurTime - videoStartTime > 1.2f*60.0f)
			{
				redenner = ((fCurTime - videoStartTime) - 1.2f * 60.0f) / (2.0f*60.0f);
				if (redenner < 0.0f) redenner = 0.0f;
				if (redenner > 1.0f) redenner = 1.0f;
			}
		}

		if (whatIsShown == SHOW_FALLING_SNIPPETS)
		{
			// Draw the snippet stuff
			GLuint texID;
			if (textureManager.getTextureID("n6.tga", &texID, errorString))
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
		screenBorders.drawBorders(&textureManager, mainWnd, showBlue, fadeInTime * fadeOut, redenner);

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
	long curTime;

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
			whatIsShown = SHOW_INTRO;
			movingPapers.init(false);
			showBlue = false;
			fadeInTime = 0.0f;
			break;
		case 'p':
		case 'P':
			PlaySound("textures/intro.wav", NULL, SND_ASYNC);
			whatIsShown = SHOW_INTRO;
			movingPapers.startDetaching(1);
			fadeInTime = 1.0f;
			break;
		case 'q':
		case 'Q':
			PlaySound("textures/silence.wav", NULL, SND_ASYNC);
			whatIsShown = SHOW_MOVING_PAPERS;
			movingPapers.init(true); // Fade it all in at the same time
			showBlue = true;
			fadeInTime = 0.0f;
			break;
		case 'w':
		case 'W':
			PlaySound("textures/silence.wav", NULL, SND_ASYNC);
			whatIsShown = SHOW_MOVING_PAPERS;
			movingPapers.init(false); // Move in from left
			showBlue = true;
			fadeInTime = 1.0f;
			break;
		case 'l':
		case 'L':
			movingPapers.startDetaching(2);
			break;
		case 's':
		case 'S':
			movingPapers.stopFeeding();
			snippets.stopFalling();
			break;
		case '1':
			PlaySound("textures/01.wav", NULL, SND_ASYNC);
			curTime = timeGetTime() - startTime;
			videoStartTime = (float)curTime * 0.001f;
			whatIsShown = SHOW_VIDEO_1;
			movingPapers.init(false);
			showBlue = true;
			fadeInTime = 1.0f;
			break;
		case '2':
			PlaySound("textures/02.wav", NULL, SND_ASYNC);
			curTime = timeGetTime() - startTime;
			videoStartTime = (float)curTime * 0.001f;
			whatIsShown = SHOW_VIDEO_2;
			movingPapers.init(false);
			showBlue = true;
			fadeInTime = 1.0f;
			break;
		case '3':
			PlaySound("textures/03.wav", NULL, SND_ASYNC);
			curTime = timeGetTime() - startTime;
			videoStartTime = (float)curTime * 0.001f;
			whatIsShown = SHOW_VIDEO_3;
			movingPapers.init(false);
			showBlue = true;
			fadeInTime = 1.0f;
			break;
		case '4':
			PlaySound("textures/04.wav", NULL, SND_ASYNC);
			curTime = timeGetTime() - startTime;
			videoStartTime = (float)curTime * 0.001f;
			whatIsShown = SHOW_VIDEO_4;
			movingPapers.init(false);
			showBlue = true;
			fadeInTime = 1.0f;
			break;
		case '5':
			PlaySound("textures/05.wav", NULL, SND_ASYNC);
			curTime = timeGetTime() - startTime;
			videoStartTime = (float)curTime * 0.001f;
			whatIsShown = SHOW_VIDEO_5;
			movingPapers.init(false);
			showBlue = true;
			fadeInTime = 1.0f;
			break;
		case '6':
			PlaySound("textures/06.wav", NULL, SND_ASYNC);
			curTime = timeGetTime() - startTime;
			videoStartTime = (float)curTime * 0.001f;
			whatIsShown = SHOW_VIDEO_6;
			movingPapers.init(false);
			showBlue = true;
			fadeInTime = 1.0f;
			break;
		case '7':
			PlaySound("textures/05.wav", NULL, SND_ASYNC);
			curTime = timeGetTime() - startTime;
			videoStartTime = (float)curTime * 0.001f;
			whatIsShown = SHOW_VIDEO_7;
			notYetDetached = true;
			movingPapers.init(false);
			showBlue = true;
			break;
		case 't':
		case 'T':
		case 'y':
		case 'Y':
		case 'z':
		case 'Z':
			PlaySound("textures/silence.wav", NULL, SND_ASYNC);
			whatIsShown = SHOW_FALLING_SNIPPETS;
			snippets.init();
			showBlue = false;
			break;
		case '9': // Soft reset (blue stays)
			PlaySound("textures/silence.wav", NULL, SND_ASYNC);
			whatIsShown = -1;
			break;
		case '0': // Hard reset (blue away)
			PlaySound("textures/silence.wav", NULL, SND_ASYNC);
			whatIsShown = -1;
			showBlue = 0;
			break;
		case 'b':
		case 'B':
			showBlue = !showBlue;
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
