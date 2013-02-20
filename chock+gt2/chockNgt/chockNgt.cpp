// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "chockNgt.h"
#include "bass.h"
#include "mathhelpers.h"
#include "Swarm.h"

LRESULT CALLBACK WindowProc (HWND, UINT, WPARAM, LPARAM);

struct TGAHeader
{
	unsigned char identSize;
	unsigned char colourmapType;
	unsigned char imageType;

	// This is a stupid hack to fool the compiler.
	// I do not know what happens if I compile it
	// under release conditions.
	unsigned char colourmapStart1;
	unsigned char colourmapStart2;
	unsigned char colourmapLength1;
	unsigned char colourmapLength2;
	unsigned char colourmapBits;

	short xStart;
	short yStart;
	short width;
	short height;
	unsigned char bits;
	unsigned char descriptor;
};

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
HSTREAM mp3Str;

void glInit()
{	
	mainDC = GetDC(mainWnd);
    if( !SetPixelFormat(mainDC,ChoosePixelFormat(mainDC,&pfd),&pfd) ) return;
    mainRC = wglCreateContext(mainDC);
    wglMakeCurrent(mainDC, mainRC);

#if 1
	// Load credits texture
	glGenTextures(1, &creditsTexture);
	glBindTexture(GL_TEXTURE_2D, creditsTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	FILE *cfpid = fopen("TEX.tga", "rb");
	if (cfpid == 0) return;
	// load header
	TGAHeader tgaHeader;
	fread(&tgaHeader, 1, sizeof(tgaHeader), cfpid);		
	// load image data
	int textureSize = 1024 * 1024 * 4;
	fread(creditsTexData, 1, textureSize, cfpid);	
	// TODO: Mip Mapping!!!!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, 1024, 1024,
					  GL_BGRA, GL_UNSIGNED_BYTE, creditsTexData);
	glDisable(GL_TEXTURE_2D);						// Enable Texture Mapping
	fclose(cfpid);
#endif
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
    mainWnd = CreateWindow ("chockngt","chockngt",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1024,600,0,0,hInstance,0);
	//mainWnd = CreateWindow("chockngt","chockngt",WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
	glInit();

    ShowWindow(mainWnd,SW_SHOW);
    UpdateWindow(mainWnd);
 
	// Initialize Swarm positions and stuff
	initSwarm();

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

	do
    {
		SetCursor(FALSE);

		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (!IsDialogMessage(mainWnd, &msg))
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
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		// draw background
		//drawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 0.4f, 1.0f, 1.0f);
		glEnable(GL_DEPTH_TEST);

		// Distance to the center object
		// TODO: Hitchcock-effect by means of gluPerspective!
		float cameraDist = 20.0f;
		float cameraComeTime = 150.0f;
		if (fCurTime > cameraComeTime)
		{
			cameraDist = 3.0f + 17.0f / (1.0f + 0.1f * ((fCurTime - cameraComeTime) * (fCurTime - cameraComeTime)));
		}

		// set up matrices
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		// TODO aspect
		//gluPerspective(25.0,  1.8, 0.1, 100.0);
		gluPerspective(500.0f / cameraDist,  1.8, 0.1, 100.0);
		//gluPerspective(25.0,  1.8, 1.1, 100.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		// Instead of this look at matrix, I will use my own...
		//gluLookAt(1., 0.5, -cameraDist,
		//		  1., 0.5, 0.0,
		//		  0.0, 1.0, 0.0);
		Matrix modelView;
		modelView.lookAt(1.0f, 0.5f, -cameraDist,
						 1.0f, 0.5f, 0.0f,
						 0.0f, 1.0f, 0.0f);

		// lighting:
		float ambient[4] = {0.3f, 0.23f, 0.2f, 1.0f};
		//float diffuse[4] = {1.8f, 1.7f, 0.8f, 1.0f};
		float diffuse[4] = {0.9f, 0.85f, 0.4f, 1.0f};
		float diffuse2[4] = {0.45f, 0.475f, 0.2f, 1.0f};
		//float diffuse2[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		float specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		float lightDir[4] = {0.7f, 0.0f, 0.7f, 0.0f};
		float allOnes[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
		glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
		//glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, allOnes);
		//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, allOnes);
		//glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, allOnes);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

		float metaAmount = 1.0f;
		int signedDistanceID = 1; // sphere
		if (fCurTime > 61.0f) signedDistanceID = 2; // metaballs
		//signedDistanceID = 0; // nothing

		int pathID = 0; // 8 moveTo points
		if (fCurTime > 80.0f)
		{
			pathID = 1; // 4 lines
			signedDistanceID = 0; // Nothing

			if (fCurTime > 130.0f)
			{
				pathID = 0; // 4 move to points
				signedDistanceID = 2; // metaballs
			}
		}

		// In reversed field
		if (fCurTime > 170.0f)
		{
			pathID = 0;
			signedDistanceID = 2;
			if (fCurTime > 190.0f)
			{
				pathID = 1;
				signedDistanceID = 0;
			}
			if (fCurTime > 210.0f)
			{
				pathID = 1;
				signedDistanceID = 2;
			}
			if (fCurTime > 217.0f)
			{
				pathID = 0;
				signedDistanceID = 1;
			}
		}

		// generate destiations
		float overshoot = 0.0f;
		if (fCurTime > 19.f)
		{
			float relTime = fCurTime - 19.f;
			overshoot = 2.0f * sin(relTime*0.17f) * (-cos(relTime * 0.75f) + 1.0f);
		}
		if (fCurTime > 200.0f)
		{
			float relTime = (fCurTime - 200.0f);
			overshoot /= relTime * relTime + 1.0f;
		}
		updateSwarmDestinations(pathID, fDeltaTime, overshoot);

		// update direction of the Triangles
		updateSwarmWithSignedDistance(signedDistanceID, fDeltaTime, metaAmount);

		// move all triangles
		moveSwarm(fDeltaTime);

		// Set how many triangles to render for each path
		int numTrisRender1 = (curTime - 11300) / 8;
		int numTrisRender2 = (curTime - 3300) / 2;
		numTrisRender1 = numTrisRender1 > NUM_TRIANGLES ? NUM_TRIANGLES : numTrisRender1;
		numTrisRender1 = numTrisRender1 < 0 ? 0 : numTrisRender1;
		numTrisRender2 = numTrisRender2 > NUM_TRIANGLES ? NUM_TRIANGLES : numTrisRender2;
		numTrisRender2 = numTrisRender2 < 0 ? 0 : numTrisRender2;
		float triBrightness = fCurTime / 30.0f + 0.2f;
		if (triBrightness > 1.0f) triBrightness = 1.0f;

		// render tris
		glBegin(GL_TRIANGLES);
		for (int i = 0; i < numTrisRender1; i++)
		{
			//glNormal3f(0.3f, 0.5f, 0.2f);
			float right[3];
			right[0] = tris.direction[i][1] * tris.normal[i][2] - tris.direction[i][2] * tris.normal[i][1];
			right[1] = tris.direction[i][2] * tris.normal[i][0] - tris.direction[i][0] * tris.normal[i][2];
			right[2] = tris.direction[i][0] * tris.normal[i][1] - tris.direction[i][1] * tris.normal[i][0];
			glNormal3fv(tris.normal[i]);
			//glVertex3f(tris.position[i][0] + 0.2f * tris.direction[i][0], tris.position[i][1] + 0.2f * tris.direction[i][1], tris.position[i][2] + 0.2f * tris.direction[i][2]);
			//glVertex3f(tris.position[i][0] + 0.2f * right[0], tris.position[i][1] + 0.2f * right[1], tris.position[i][2] + 0.2f * right[2]);
			//glVertex3f(tris.position[i][0] - 0.2f * right[0], tris.position[i][1] - 0.2f * right[0], tris.position[i][2] - 0.2f * right[0]);
			modelView.vertex3f(tris.position[i][0] + 0.2f * tris.direction[i][0], tris.position[i][1] + 0.2f * tris.direction[i][1], tris.position[i][2] + 0.2f * tris.direction[i][2]);
			modelView.vertex3f(tris.position[i][0] + 0.2f * right[0], tris.position[i][1] + 0.2f * right[1], tris.position[i][2] + 0.2f * right[2]);
			modelView.vertex3f(tris.position[i][0] - 0.2f * right[0], tris.position[i][1] - 0.2f * right[0], tris.position[i][2] - 0.2f * right[0]);
		}
		glEnd();

		glDepthMask(FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE); 
		glDisable(GL_LIGHTING);
		float beating = 1.0f - 0.25f * fabsf((float)sin(fCurTime*4.652f));
		glBegin(GL_TRIANGLES);
		for (int i = 0; i < numTrisRender2; i++)
		{
			const float colorA[3] = {0.3f, 0.5f, 1.0f};
			const float colorB[3] = {1.0f, 0.8f, 0.3f};
			const float colorC[3] = {2.0f, 0.4f, 0.1f};
			float color[3];
			float colorT = fCurTime * 0.01f;
			if (colorT > 1.0f) colorT = 1.0f;
			for (int c = 0; c < 3; c++)
			{
				color[c] = colorA[c] * (1.0f - colorT) + colorT * colorB[c];
			}
			if (fCurTime > 100.0f)
			{
				colorT = (fCurTime - 100.0f) * 0.1f - ((i*i*17)%63) * 0.1f;
				if (colorT < 0.0f) colorT = 0.0f;
				if (colorT > 1.0f) colorT = 1.0f;
				for (int c = 0; c < 3; c++)
				{
					color[c] = colorB[c] * (1.0f - colorT) + colorT * colorC[c];
				}
			}
			// Some color noise
			for (int c = 0; c < 3; c++)
			{
				float n = ((((i*i+13)*(c*c+7)+(i*(c+1))) % 100) - 50) * 0.0025f;
				color[c] += n;
			}

			//glNormal3f(0.3f, 0.5f, 0.2f);
			glColor4f(color[0], color[1], color[2], beating * 0.1f * triBrightness);
			float right[3];
			right[0] = tris.direction[i][1] * tris.normal[i][2] - tris.direction[i][2] * tris.normal[i][1];
			right[1] = tris.direction[i][2] * tris.normal[i][0] - tris.direction[i][0] * tris.normal[i][2];
			right[2] = tris.direction[i][0] * tris.normal[i][1] - tris.direction[i][1] * tris.normal[i][0];
			glNormal3fv(tris.normal[i]);
#if 0
			glVertex3f(tris.position[i][0] + 0.6f * tris.direction[i][0],
					   tris.position[i][1] + 0.6f * tris.direction[i][1],
					   tris.position[i][2] + 0.6f * tris.direction[i][2] - 0.1f);
			glVertex3f(tris.position[i][0] + 0.6f * right[0] - 0.2f * tris.direction[i][0],
					   tris.position[i][1] + 0.6f * right[1] - 0.2f * tris.direction[i][1],
					   tris.position[i][2] + 0.6f * right[2] - 0.2f * tris.direction[i][2] - 0.001f);
			glVertex3f(tris.position[i][0] - 0.6f * right[0] - 0.2f * tris.direction[i][0],
					   tris.position[i][1] - 0.6f * right[0] - 0.2f * tris.direction[i][1],
					   tris.position[i][2] - 0.6f * right[0] - 0.2f * tris.direction[i][2] - 0.001f);
#else
			modelView.vertex3f(tris.position[i][0] + 0.6f * tris.direction[i][0],
							   tris.position[i][1] + 0.6f * tris.direction[i][1],
							   tris.position[i][2] + 0.6f * tris.direction[i][2] - 0.1f);
			modelView.vertex3f(tris.position[i][0] + 0.6f * right[0] - 0.2f * tris.direction[i][0],
							   tris.position[i][1] + 0.6f * right[1] - 0.2f * tris.direction[i][1],
							   tris.position[i][2] + 0.6f * right[2] - 0.2f * tris.direction[i][2] - 0.001f);
			modelView.vertex3f(tris.position[i][0] - 0.6f * right[0] - 0.2f * tris.direction[i][0],
							   tris.position[i][1] - 0.6f * right[0] - 0.2f * tris.direction[i][1],
							   tris.position[i][2] - 0.6f * right[0] - 0.2f * tris.direction[i][2] - 0.001f);
#endif
		}
		glEnd();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glBegin(GL_TRIANGLES);
		if (fCurTime > 219.0f)
		{
			float alpha = (fCurTime - 219.0f) * 0.5f;
			alpha = alpha > 1.0f ? 1.0f : alpha;
			glColor4f(0.0f, 0.0f, 0.0f, alpha);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glVertex3f(-180.0f, 180.0f, 1.0f);
			glVertex3f(180.0f, 0.0f, 1.0f);
			glVertex3f(-180.0f, -180.0f, 1.0f);
		}
		glEnd();
		glDepthMask(TRUE);

		// draw credits
		if (fCurTime > 221.0f)
		{
			float alpha = (fCurTime - 221.0f) * 1.0f;
			alpha = alpha > 1.0f ? 1.0f : alpha;
			drawQuad(-1.3f, 0.7f, -0.1f, 0.4f, 0.0f, 0.175f, alpha);
			drawQuad(-0.7f, 1.3f, -0.5f, 0.0f, 0.175f, 0.35f, alpha);
		}

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);


		//glColor3ub(200, 100, 50);
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
		// swap buffers
		wglSwapLayerBuffers(mainDC, WGL_SWAP_MAIN_PLANE);

		//Sleep(5);
    } while (msg.message != WM_QUIT && fCurTime < 230.0f && !GetAsyncKeyState(VK_ESCAPE));

	// music uninit
	BASS_ChannelStop(mp3Str);
	BASS_StreamFree(mp3Str);
	BASS_Free();

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
