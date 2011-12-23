// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "chockNgt.h"
#include "bass.h"

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

// TODO: Get rid of this shit!
char szAppName [] = TEXT("WebCam");

// music data
HSTREAM mp3Str;

// Triangle data
#define NUM_TRIANGLES 4000
float triPositions[NUM_TRIANGLES][3];
float triDirections[NUM_TRIANGLES][3];
float triNormals[NUM_TRIANGLES][3];
//float triDirections[NUM_TRIANGLES][4]; // on the fly?

void glInit()
{	
	mainDC = GetDC(mainWnd);
    if( !SetPixelFormat(mainDC,ChoosePixelFormat(mainDC,&pfd),&pfd) ) return;
    mainRC = wglCreateContext(mainDC);
    wglMakeCurrent(mainDC, mainRC);
}

void glUnInit()
{
	wglDeleteContext(mainRC);
	ReleaseDC(mainWnd, mainDC);
}

float frand()
{
	return (float)rand()/(float)RAND_MAX;
}

void normalize(float *vector, int dimension)
{
	float length = 0.0f;
	for (int i = 0; i < dimension; i++)
	{
		length += vector[i] * vector[i];
	}
	length = sqrtf(length);

	if (length > 0.0001f)
	{
		for (int i = 0; i < dimension; i++)
		{
			vector[i] = vector[i] / length;
		}
	}
	else
	{
		vector[0] = 1.0f;
		for (int i = 1; i < dimension; i++)
		{
			vector[i] = 0.0f;
		}
	}
}

// dot product
float dot(float *a, float *b, int dimension)
{
	float result = 0.0f;
	for (int i = 0; i < dimension; i++)
	{
		result += a[i] * b[i];
	}
	return result;
}

// change vector so that it is normal again.
// Both must be normalized.
// TODO: will fail, if they go the same way.
void reNormal(float *vec, float *normal, int dimension)
{
	float normalizer = dot(vec, normal, dimension);

	for (int i = 0; i < dimension; i++)
	{
		vec[i] -= normalizer * normal[i];
	}
	normalize(vec, dimension);
}

// only for unit length vectors!
// source and result may be the same.
void slerp(float *source, float *dest, float *result, int dimension, float t)
{
	// fake!
	for (int i = 0; i < dimension; i++)
	{
		result[i] = (1.0f - t) * source[i] + t * dest[i];
	}

	normalize(result, dimension);
}

// get signed distance value
float getSDSphere(float *position)
{
	float length = 0.0f;

	length = sqrtf(position[0] * position[0] +
			       position[1] * position[1] +
				   position[2] * position[2]);
	length -= 3.0;
	return length;
}

// get Normal. I may need to use the signed distance for that?
void getNormalSphere(float *position, float *normal)
{
	normal[0] = position[0];
	normal[1] = position[1];
	normal[2] = position[2];

	normalize(normal, 3);
}

// update center positions of the metaballs
float metaCenters[3][3];
void updateSD(float time)
{
	static float randomValues[3][3][2];
	static bool randomValuesAreMade = false;
	if (!randomValuesAreMade)
	{
		randomValuesAreMade = true;
		for (int i = 0; i < 3*3*2; i++)
		{
			randomValues[0][0][i] = frand() * 1.0f;
		}
	}
	for (int i = 0; i < 3*3; i++)
	{
		metaCenters[0][i] = 2.5f * sin(randomValues[0][i][0]*time + 3.0f * randomValues[0][i][1]);
	}
}

// three metaballs SD
float getSD(float *position)
{
	float value = 0.0f;

	for (int ball = 0; ball < 3; ball++)
	{
		position[0] -= metaCenters[ball][0];
		position[1] -= metaCenters[ball][1];
		position[2] -= metaCenters[ball][2];

		value += 1.0f / (position[0] * position[0] +
						 position[1] * position[1] +
						 position[2] * position[2] + 0.01f);
		
		position[0] += metaCenters[ball][0];
		position[1] += metaCenters[ball][1];
		position[2] += metaCenters[ball][2];
	}

	value = sqrt(1.0f / value) - 2.0f;

	return value;
}

void getNormal(float *position, float *normal)
{
	normal[0] = 0.0f;
	normal[1] = 0.0f;
	normal[2] = 0.0f;

	for (int ball = 0; ball < 3; ball++)
	{
		position[0] -= metaCenters[ball][0];
		position[1] -= metaCenters[ball][1];
		position[2] -= metaCenters[ball][2];

		float value = 1.0f / (position[0] * position[0] +
						      position[1] * position[1] +
						      position[2] * position[2] + 0.01f);
		value = value * value;
		normal[0] += value * position[0];
		normal[1] += value * position[1];
		normal[2] += value * position[2];
		
		position[0] += metaCenters[ball][0];
		position[1] += metaCenters[ball][1];
		position[2] += metaCenters[ball][2];
	}

	normalize(normal, 3);
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
    wc.lpszClassName = szAppName;

    RegisterClass (&wc);

	// Create the window
    //mainWnd = CreateWindow (szAppName,szAppName,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1024,600,0,0,hInstance,0);
	mainWnd = CreateWindow(szAppName,szAppName,WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
	glInit();

    ShowWindow(mainWnd,SW_SHOW);
    UpdateWindow(mainWnd);
 
	// Initialize triangle positions
	for (int i = 0; i < NUM_TRIANGLES; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			triPositions[i][j] = (frand() - 0.5f) * 10.0f;
			triDirections[i][j] = (frand() - 0.5f);
			triNormals[i][j] = (frand() - 0.5f);
		}
		normalize(triDirections[i], 3);
		normalize(triNormals[i], 3);
		reNormal(triNormals[i], triDirections[i], 3);
	}

	long startTime = timeGetTime();
	long lastTime = 0;

	// start music playback
	BASS_Init(-1,44100,0,mainWnd,NULL);
	mp3Str=BASS_StreamCreateFile(FALSE,"GT_muc.mp3",0,0,0);
	BASS_ChannelPlay(mp3Str, TRUE);
	BASS_Start();
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
		glEnable(GL_DEPTH_TEST);

		// set up matrices
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		// TODO aspect
		gluPerspective(25.0,  1.8, 0.1, 100.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(0.0, 0.0, -20.0,
				  0.0, 0.0, 0.0,
				  0.0, 1.0, 0.0);

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

		/*glBegin(GL_TRIANGLES);
		//glColor3ub(200, 100, 0);
		glNormal3f(sin((float)0.001f * curTime), 0.0f, cos((float)0.001f * curTime));
		glVertex3f(-1.0, 1.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, -1.0, 0.0);
		//glColor3ub(200, 0, 200);
		glVertex3f(0.0, 1.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, -1.0, 0.0);
		glEnd();*/

		// update metaball positions
		updateSD(fCurTime);

		// generate destiations
#define NUM_MOVE_POINTS 3
		float moveToPoint[NUM_MOVE_POINTS][3];
		static float randomValues[NUM_MOVE_POINTS][3][4];
		static bool randomValuesAreMade = false;
		float slowMotion = 1.0f;
		if (fCurTime < 22.5f) slowMotion = fCurTime - 21.5f;
		if (fCurTime < 22.0f) slowMotion = 0.5f;
		float metaAmount = 0.0f;
		if (fCurTime > 96.5f) metaAmount = (fCurTime - 96.5f) * 0.1f;
		if (metaAmount > 1.0f) metaAmount = 1.0f;
		metaAmount *= metaAmount;
		float pointAmount = 1.0f;
		if (fCurTime > 214.0f) pointAmount = 0.0f;

		if (!randomValuesAreMade)
		{
			randomValuesAreMade = true;
			for (int i = 0; i < NUM_MOVE_POINTS * 3 * 4; i++)
			{
				randomValues[0][0][i] = frand() * 1.0f;
			}
		}
		for (int i = 0; i < NUM_MOVE_POINTS; i++)
		{
			moveToPoint[i][0] = 6.0f * sin(fCurTime * randomValues[i][0][0] + randomValues[i][0][1]) +
				5.0f * sin(fCurTime * randomValues[i][0][2] + randomValues[i][0][3]);
			moveToPoint[i][1] = 5.5f * sin(fCurTime * randomValues[i][1][0] + randomValues[i][1][1]) +
				6.5f * sin(fCurTime * randomValues[i][1][2] + randomValues[i][1][3]);
			moveToPoint[i][2] = 4.5f * sin(fCurTime * randomValues[i][2][0] + randomValues[i][2][1]) +
				5.5f * sin(fCurTime * randomValues[i][2][2] + randomValues[i][2][3]);
		}
		int destinationMultiplier = (curTime / 20000) + 1;

		// move all triangles
		for (int i = 0; i < NUM_TRIANGLES; i++)
		{
			int mPoint = ((i * destinationMultiplier) % 113 + (i % 31)) % NUM_MOVE_POINTS;
			
			// update direction based on destination point
			float destDirection[3];
			destDirection[0] = moveToPoint[mPoint][0] - triPositions[i][0] + 40.0f * (frand() - 0.5f);
			destDirection[1] = moveToPoint[mPoint][1] - triPositions[i][1] + 40.0f * (frand() - 0.5f);
			destDirection[2] = moveToPoint[mPoint][2] - triPositions[i][2] + 40.0f * (frand() - 0.5f);
			normalize(destDirection, 3);
			slerp(triDirections[i], destDirection, triDirections[i], 3, 1.8f * fDeltaTime * slowMotion * pointAmount);
			reNormal(triNormals[i], triDirections[i], 3);

			// update normal based on signed distance
			float signedDist = getSD(triPositions[i]);
			getNormal(triPositions[i], destDirection);
			float t = 1.0f - fabsf(signedDist);
			t = t < 0.0f ? 0.0f : t;
			slerp(triNormals[i], destDirection, triNormals[i], 3, t * metaAmount);
			reNormal(triDirections[i], triNormals[i], 3);

			for (int j = 0; j < 3; j++)
			{
				//triPositions[i][j] += (moveToPoint[mPoint][j] - triPositions[i][j]) * 0.1f * fDeltaTime;
				
				// update position based on fly direction
				triPositions[i][j] += triDirections[i][j] * fDeltaTime * 3.0f * slowMotion;
			}
		}

		// Set how many triangles to render for each path
		int numTrisRender1 = (curTime - 11000) / 5;
		int numTrisRender2 = (curTime-3300);
		numTrisRender1 = numTrisRender1 > NUM_TRIANGLES ? NUM_TRIANGLES : numTrisRender1;
		numTrisRender1 = numTrisRender1 < 0 ? 0 : numTrisRender1;
		numTrisRender2 = numTrisRender2 > NUM_TRIANGLES ? NUM_TRIANGLES : numTrisRender2;
		numTrisRender2 = numTrisRender2 < 0 ? 0 : numTrisRender2;

		// render tris
		glBegin(GL_TRIANGLES);
		for (int i = 0; i < numTrisRender1; i++)
		{
			//glNormal3f(0.3f, 0.5f, 0.2f);
			float right[3];
			right[0] = triDirections[i][1] * triNormals[i][2] - triDirections[i][2] * triNormals[i][1];
			right[1] = triDirections[i][2] * triNormals[i][0] - triDirections[i][0] * triNormals[i][2];
			right[2] = triDirections[i][0] * triNormals[i][1] - triDirections[i][1] * triNormals[i][0];
			glNormal3fv(triNormals[i]);
			glVertex3f(triPositions[i][0] + 0.2f * triDirections[i][0], triPositions[i][1] + 0.2f * triDirections[i][1], triPositions[i][2] + 0.2f * triDirections[i][2]);
			glVertex3f(triPositions[i][0] + 0.2f * right[0], triPositions[i][1] + 0.2f * right[1], triPositions[i][2] + 0.2f * right[2]);
			glVertex3f(triPositions[i][0] - 0.2f * right[0], triPositions[i][1] - 0.2f * right[0], triPositions[i][2] - 0.2f * right[0]);
		}
		glEnd();

		glDepthMask(FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE); 
		glDisable(GL_LIGHTING);
		float beating = 1.0f - 0.25f * fabsf((float)sin(fCurTime*4.652f));
		glColor4f(1.0f, 0.5f, 0.3f, beating * 0.1f);
		glBegin(GL_TRIANGLES);
		for (int i = 0; i < numTrisRender2; i++)
		{
			//glNormal3f(0.3f, 0.5f, 0.2f);
			float right[3];
			right[0] = triDirections[i][1] * triNormals[i][2] - triDirections[i][2] * triNormals[i][1];
			right[1] = triDirections[i][2] * triNormals[i][0] - triDirections[i][0] * triNormals[i][2];
			right[2] = triDirections[i][0] * triNormals[i][1] - triDirections[i][1] * triNormals[i][0];
			glNormal3fv(triNormals[i]);
			glVertex3f(triPositions[i][0] + 0.6f * triDirections[i][0],
					   triPositions[i][1] + 0.6f * triDirections[i][1],
					   triPositions[i][2] + 0.6f * triDirections[i][2] - 0.1f);
			glVertex3f(triPositions[i][0] + 0.6f * right[0] - 0.2f * triDirections[i][0],
					   triPositions[i][1] + 0.6f * right[1] - 0.2f * triDirections[i][1],
					   triPositions[i][2] + 0.6f * right[2] - 0.2f * triDirections[i][2] - 0.001f);
			glVertex3f(triPositions[i][0] - 0.6f * right[0] - 0.2f * triDirections[i][0],
					   triPositions[i][1] - 0.6f * right[0] - 0.2f * triDirections[i][1],
					   triPositions[i][2] - 0.6f * right[0] - 0.2f * triDirections[i][2] - 0.001f);
		}
		glEnd();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
		glDisable(GL_DEPTH_TEST);
		glBegin(GL_TRIANGLES);
		if (fCurTime > 225.0f)
		{
			float alpha = (fCurTime - 225.0f) * 0.3f;
			alpha = alpha > 1.0f ? 1.0f : alpha;
			glColor4f(0.0f, 0.0f, 0.0f, alpha);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glVertex3f(-20.0f, 20.0f, 1.0f);
			glVertex3f(20.0f, 0.0f, 1.0f);
			glVertex3f(-20.0f, -20.0f, 1.0f);
		}
		glEnd();
		glDisable(GL_BLEND);
		glDepthMask(TRUE);

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
