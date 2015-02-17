// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "chockNgt.h"
//#include "bass.h"
#include "mathhelpers.h"
#include "TextureManager.h"
#include "Configuration.h"
#include "Snippets.h"

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

void glInit()
{
	mainDC = GetDC(mainWnd);
    if( !SetPixelFormat(mainDC,ChoosePixelFormat(mainDC,&pfd),&pfd) ) return;
    mainRC = wglCreateContext(mainDC);
    wglMakeCurrent(mainDC, mainRC);

	// Create and initialize everything needed for texture Management
	char errorString[MAX_ERROR_LENGTH+1];
	if (textureManager.init(errorString))
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

// Newspaper move global data
#define NUM_MOVE_NEWSPAPERS 9
#define MOVE_NEWSPAPER_SPEED_X 0.004f
#define MOVE_NEWSPAPER_SPEED_Y 0.005f
#define MOVE_NEWSPAPER_WIDTH 0.6f
#define MOVE_NEWSPAPER_HEIGHT 0.8f
float newspaperPos[NUM_MOVE_NEWSPAPERS][2];
int currentNewspaper;
char *newspaperName[NUM_MOVE_NEWSPAPERS] = 
{
	"1.tga",
	"2.tga",
	"3.tga",
	"4.tga",
	"5.tga",
	"6.tga",
	"7.tga",
	"8.tga",
	"9.tga",
};

// Function to initialize the moving of newspapers (set to left)
void initMoveNewspaper()
{
	for (int np = 0; np < NUM_MOVE_NEWSPAPERS; np++)
	{
		newspaperPos[np][0] = -1.0f - MOVE_NEWSPAPER_WIDTH;
		newspaperPos[np][1] = 0.0f - 0.5f * MOVE_NEWSPAPER_HEIGHT;
	}
	currentNewspaper = -1;
}

// Update newspaper positions and draw the things
void drawNewspaper(int deltaX, int deltaY)
{
	char errorString[MAX_ERROR_LENGTH+1];

	if (currentNewspaper >= 0 && currentNewspaper < NUM_MOVE_NEWSPAPERS)
	{
		newspaperPos[currentNewspaper][0] += MOVE_NEWSPAPER_SPEED_X * deltaX;
		if (newspaperPos[currentNewspaper][0] < -1.0f - MOVE_NEWSPAPER_WIDTH)
		{
			newspaperPos[currentNewspaper][0] = -1.0f - MOVE_NEWSPAPER_WIDTH;
		}
		if (newspaperPos[currentNewspaper][0] > 1.0f)
		{
			newspaperPos[currentNewspaper][0] = 1.0f;
		}
		if (newspaperPos[currentNewspaper][1] < -1.0f - MOVE_NEWSPAPER_HEIGHT)
		{
			newspaperPos[currentNewspaper][1] = -1.0f - MOVE_NEWSPAPER_HEIGHT;
		}
		if (newspaperPos[currentNewspaper][1] > 1.0f)
		{
			newspaperPos[currentNewspaper][1] = 1.0f;
		}
		newspaperPos[currentNewspaper][1] -= MOVE_NEWSPAPER_SPEED_Y * deltaY;
	}

	for (int np = 0; np < NUM_MOVE_NEWSPAPERS; np++)
	{
		GLuint texID;
		if (textureManager.getTextureID(newspaperName[np], &texID, errorString))
		{
			MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
			return;
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		drawQuad(newspaperPos[np][0], newspaperPos[np][0] + MOVE_NEWSPAPER_WIDTH,
			     newspaperPos[np][1], newspaperPos[np][1] + MOVE_NEWSPAPER_HEIGHT,
				 0.0f, 1.0f, 1.0f);
	}
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
    //mainWnd = CreateWindow (szAppName,szAppName,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1024,600,0,0,hInstance,0);
	mainWnd = CreateWindow("chockngt","chockngt",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1024,768,0,0,hInstance,0);
	//mainWnd = CreateWindow("chockngt","chockngt",WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
	glInit();
	initMoveNewspaper();

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


#if 1
	// Initialize test avi
	AVISTREAMINFO psi;
	PAVISTREAM pavi;
	PGETFRAME pgf;
	BITMAPINFOHEADER bmih;
	int width;  // Video width
	int height; // Video height
	int lastFrame; // Last frame in the AVI stream (number of frames, basically)
	char *pdata;  // Pointer to texture data

	HDRAWDIB hdd = DrawDibOpen(); // Used for scaling/drawing the avi to a RAM buffer
	HBITMAP hBitmap; // Bitmap that holds the texture
	HDC hdc = CreateCompatibleDC(mainDC);
	//HDC hdc = mainDC;
	unsigned char *data = 0; // Resized image after using dibdraw

	AVIFileInit(); // Opens the AVIFile Library
	if (AVIStreamOpenFromFile(&pavi, "textures/test_awkward.avi", streamtypeVIDEO, 0, OF_READ, NULL))
	{
		MessageBox(mainWnd, "Failed to open textures/test.avi", "AVI error", MB_OK);
		return -1;
	}
	AVIStreamInfo(pavi, &psi, sizeof(psi)); // Reads stream info
	width = psi.rcFrame.right - psi.rcFrame.left;
	height = psi.rcFrame.bottom - psi.rcFrame.top;
	lastFrame = AVIStreamLength(pavi);
	//mpf = AVIStreamSampleToTime(pavi, lastFrame) / lastFrame; // Clumsy frame rate computation

	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biPlanes = 1;
	bmih.biBitCount = 24;
	bmih.biWidth = width;
	bmih.biHeight = height;
	bmih.biCompression = BI_RGB;
	hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)(&bmih), DIB_RGB_COLORS, (void**)(&data), NULL, NULL);
	SelectObject(hdc, hBitmap);
	GdiFlush();

	pgf = AVIStreamGetFrameOpen(pavi, NULL);
	if (pgf == NULL)
	{
		MessageBox(mainWnd, "Failed to open frames for loading", "AVI error", MB_OK);
		return -1;
	}
#endif


	POINT newMousePos = {0, 0};
	int relMouseX, relMouseY;
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

		// Mouse stuff
#if 1
		ShowCursor(FALSE);
		GetCursorPos(&newMousePos);
		relMouseX = newMousePos.x - 320;
		relMouseY = newMousePos.y - 240;
		SetCursorPos(320, 240);
#endif

		// update timer
		long curTime = timeGetTime() - startTime;
		fCurTime = (float)curTime * 0.001f;
		long deltaTime = curTime - lastTime;
		float fDeltaTime = (float) deltaTime * 0.001f;
		lastTime = curTime;

		// render
		wglMakeCurrent(mainDC, mainRC);
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		glDisable(GL_LIGHTING);

#if 0
		// Draw the newspaper moving thing
		drawNewspaper(relMouseX, relMouseY);
#endif

#if 0
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
#endif

#if 1
		// Draw some avi texture
		static int frame = 0;
		LPBITMAPINFOHEADER lpbi;
		lpbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(pgf, frame);
		frame++;
		frame %= lastFrame;
		pdata = (char *)lpbi + lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD); // Skip header info to get to data
		// Convert data to requested bitmap format
		DrawDibDraw(hdd, hdc, 0, 0, width, height, lpbi, pdata, 0, 0, width, height, 0);
		GdiFlush();
		// create openGL texture
		glEnable(GL_TEXTURE_2D);				// Enable Texture Mapping
		GLuint aviTexID;
		textureManager.getTextureID("1.tga", &aviTexID, errorString);
		glBindTexture(GL_TEXTURE_2D, aviTexID);
		//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_BGR,
		//				  64, 64, GL_BGRA, GL_UNSIGNED_BYTE, data);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, 400, GL_BGR, GL_UNSIGNED_BYTE, data);
		drawQuad(-0.3f, 0.8f, -0.2f, 0.7, 0.0f, 1.0f, 1.0f);
#endif
		// draw background
		//drawQuad(-0.3f, 0.8f, -0.2f, 0.7f, 0.4f, 1.0f, 1.0f);
		//glEnable(GL_DEPTH_TEST);

		// set up matrices
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		// TODO aspect
		//gluPerspective(25.0,  1.8, 0.1, 100.0);
		//gluPerspective(500.0f / cameraDist,  1.8, 0.1, 100.0);
		//gluPerspective(25.0,  1.8, 1.1, 100.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		//gluLookAt(1., 0.5, -cameraDist,
		//		  1., 0.5, 0.0,
		//		  0.0, 1.0, 0.0);

		//glColor3ub(200, 100, 50);
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
		// swap buffers
		wglSwapLayerBuffers(mainDC, WGL_SWAP_MAIN_PLANE);

		//Sleep(5);
    } while (msg.message != WM_QUIT && fCurTime < 230.0f && !GetAsyncKeyState(VK_ESCAPE));

	// music uninit
#if 0
	BASS_ChannelStop(mp3Str);
	BASS_StreamFree(mp3Str);
	BASS_Free();
#endif

#if 1
	// AVI uninit
	DeleteObject(hBitmap);
	DrawDibClose(hdd);
	AVIStreamGetFrameClose(pgf);
	AVIStreamRelease(pavi);
	AVIFileExit();
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
			// move newspaper
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			currentNewspaper = wParam - '1';
			break;

		case '0':
			currentNewspaper = -1;
			break;

		default:
			currentNewspaper = -1; // just in case...
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
