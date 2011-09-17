// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "Security1.h"
#include "VideoTexture.h"
#include "CFPContainer.h"
#include "Texture.h"
#include "Parameter.h"

LRESULT CALLBACK WindowProc (HWND, UINT, WPARAM, LPARAM);

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

// I simply make a file with all the shaders...
#include "shaders.h"

// Shader stuff
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
	 "glActiveTexture", "glGetUniformLocation", "glUniform1i",
	 "glMultiTexCoord2f"
};
GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderPrograms[1];
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
// The second window for editing
HDC editDC;
HGLRC editRC;
HWND editWnd;
// The model matrix is used to send the parameters to the hardware...
// parameterMatrix[0][0] == scanTime, if > 2.0: Do not scan
static float parameterMatrix[4][4];
// Flag whether the background should be captured
static int additionalCapture = 1;
// Red box display counter:
// 0 = nothing
// 1 = make ready
// 2 = show
static int redBoxDisplay = 0;
static float redBoxX = 0.0f;
static float redBoxY = 0.0f;
static float mousePos[2];

// Chimere display flag
static bool chimereDisplay = false;

// Display stuff for cfp data
static double cfpTime = -1; // Set to 0 when stuff starts, when it's over it's set to negative.
static double boxFaceTime = -1; // Set to 0 when stuff starts, -2 when over, -1 update, but no display
static double introTime = -1; // before it all started...
const double introLength = 30000.0f; // the length until it is done introing..
static CFPContainer cfpContainer;
static CFPContainer cfpRefContainer;
static Texture assamese1;
#define NUM_STECKBRIEFE 20
static Texture steckbrief[NUM_STECKBRIEFE];
#define NUM_POINT_TEXTURES 4
static Texture loading[NUM_POINT_TEXTURES];
static Texture match[NUM_POINT_TEXTURES];
static Texture noMatch;
static int nextBoxSpecial = -1; // Set a special Box next;
static int resetFaceBox[2] = {-2, 0}; // Reset position of box face... -1 for get ready!
static bool stopScan = true; // Stop the scanning as soon as possible

// The timing speeduppers
static float boxFaceSpeedup = 1.0f;
static float cfpSpeedup = 1.0f;
static float scanSpeedup = 1.0f;
static float introSpeedup = 1.0f;

// TODO: Get rid of this shit!
char szAppName [] = TEXT("WebCam");

// The (two) video textures...
VideoTexture videoTexture[2];

void glInit()
{	
    editDC = GetDC(editWnd);
    if( !SetPixelFormat(editDC,ChoosePixelFormat(editDC,&pfd),&pfd) ) return;
    editRC = wglCreateContext(editDC);
    wglMakeCurrent(editDC, editRC);

	mainDC = GetDC(mainWnd);
    if( !SetPixelFormat(mainDC,ChoosePixelFormat(mainDC,&pfd),&pfd) ) return;
    mainRC = wglCreateContext(mainDC);
    wglMakeCurrent(mainDC, mainRC);

	wglShareLists(mainRC, editRC);

	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// init objects:	
	GLuint vMain = glCreateShader(GL_VERTEX_SHADER);
	GLuint fSobel = glCreateShader(GL_FRAGMENT_SHADER);	
	shaderPrograms[0] = glCreateProgram();
	// compile sources:
	glShaderSource(vMain, 1, &vertexMain, NULL);
	glCompileShader(vMain);
	glShaderSource(fSobel, 1, &fragmentSobel, NULL);
	glCompileShader(fSobel);

	// Check programs
	int tmp, tmp2;
	char err[4097];
	glGetShaderiv(vMain, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(vMain, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(mainWnd, err, "vMain shader error", MB_OK);
		return;
	}
	glGetShaderiv(fSobel, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(fSobel, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(mainWnd, err, "fSobel shader error", MB_OK);
		return;
	}

	// link shaders:
	glAttachShader(shaderPrograms[0], vMain);
	glAttachShader(shaderPrograms[0], fSobel);
	glLinkProgram(shaderPrograms[0]);

	// Set texture locations
	glUseProgram(shaderPrograms[0]);
	int my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[0], "Texture0");
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(my_sampler_uniform_location, 0);
	my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[0], "Texture1");
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(my_sampler_uniform_location, 1);
	my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[0], "Texture2");
	glActiveTexture(GL_TEXTURE2);
	glUniform1i(my_sampler_uniform_location, 2);
}

void glUnInit()
{
	wglDeleteContext(mainRC);
	ReleaseDC(mainWnd, mainDC);
	wglDeleteContext(editRC);
	ReleaseDC(editWnd, editDC);
}

void drawBox(float startX, float startY, float endX, float endY)
{
	glBegin(GL_LINES);
	glVertex3f(startX, startY, 0.5f);
	glVertex3f(startX, endY, 0.5f);
	glVertex3f(startX, endY, 0.5f);
	glVertex3f(endX, endY, 0.5f);
	glVertex3f(endX, endY, 0.5f);
	glVertex3f(endX, startY, 0.5f);
	glVertex3f(endX, startY, 0.5f);
	glVertex3f(startX, startY, 0.5f);
	glEnd();
}

void fillBox(float startX, float startY, float endX, float endY)
{
	glBegin(GL_QUADS);
	glVertex3f(startX, startY, 0.5f);
	glVertex3f(startX, endY, 0.5f);
	glVertex3f(endX, endY, 0.5f);
	glVertex3f(endX, startY, 0.5f);
	glEnd();
}

void drawFaceInterpol(CFPContainer* container, int faceID,
					  float startX1, float startY1, float endX1, float endY1,
					  float startX2, float startY2, float endX2, float endY2,
					  float t)
{
	float startX = (1.0f - t) * startX1 + t * startX2;
	float startY = (1.0f - t) * startY1 + t * startY2;
	float endX = (1.0f - t) * endX1 + t * endX2;
	float endY = (1.0f - t) * endY1 + t * endY2;

	parameterMatrix[0][1] = 0.0f; // enable textures
	glLoadMatrixf(parameterMatrix[0]);
	container->drawScreenAlignedQuad(faceID, startX, startY, endX, endY);
	parameterMatrix[0][1] = 1.0f; // disable textures
	glLoadMatrixf(parameterMatrix[0]);
	glColor3ub(196, 0, 0);
	drawBox(startX, startY, endX, endY);
}

// The rendering of all the CFP stuff, using inner time.
#define TIME_IN_RENDER_CFP 35000  // overall time for one run
#define TIME_OFF_RENDER_CFP 12000 // time for the loading...
#define TIME_TRANSITION_CFP 7000  // time for the transition move of faces
void renderCFPStuff(int currentTime, float rightMove)
{
	// determine which face to render and the inner time
	long innerTime = currentTime % TIME_IN_RENDER_CFP;
	long currentFace = currentTime / TIME_IN_RENDER_CFP;
	//currentFace = currentFace % cfpContainer.getNumTextures();
	int numFaces;
	numFaces = min(cfpContainer.getNumTextures(), 3);
	if (currentFace >= numFaces)
	{
		currentFace = numFaces - 1;
		innerTime = currentTime - (numFaces-1) * TIME_IN_RENDER_CFP;
	}

	glColor3ub(255, 255, 255);
	parameterMatrix[0][1] = 0.0f; // enable textures
	glLoadMatrixf(parameterMatrix[0]);
	
	// Calculate face moving positions
	float sourcePos[3][4] =
	{
		{-0.9f + rightMove, 0.1f, -0.8f + rightMove, 0.3f},
		{-0.9f + rightMove, -0.15f, -0.8f + rightMove, 0.05f},
		{-0.9f + rightMove, -0.4f, -0.8f + rightMove, -0.2f},
	};
	float destPos[3][4] =
	{
		{-0.95f + rightMove, 0.35f, -0.75f + rightMove, 0.775f},
		{-0.9f + rightMove, 0.1f, -0.8f + rightMove, 0.3f},
		{-0.9f + rightMove, -0.15f, -0.8f + rightMove, 0.05f},
	};
	float moveT = (float)innerTime / (float) TIME_TRANSITION_CFP;
	if (innerTime < TIME_TRANSITION_CFP)
		moveT = (float)(innerTime-TIME_TRANSITION_CFP/2) / (float)(TIME_TRANSITION_CFP/2);
	if (moveT < 0.0f) moveT = 0.0f;
	if (moveT > 1.0f) moveT = 1.0f;

	if (currentFace == 0)
	{
		// get source locations from videoTexture
		for (int special = 0; special < 3; special++)
		{
			int boxID;
			
			boxID = videoTexture[0].boxFaceSpecial[special];
			if (boxID >= 0)
			{
				videoTexture[0].getFaceRect(boxID, sourcePos[special], -1.0f, -1.0f, 0.0f, 1.0f);
			}
			boxID = videoTexture[1].boxFaceSpecial[special];
			if (boxID >= 0)
			{
				videoTexture[1].getFaceRect(boxID, sourcePos[special], 0.0f, -1.0f, 1.0f, 1.0f);
			}
		}
	}
	else
	{
		drawFaceInterpol(&cfpContainer, currentFace-1,
						 sourcePos[0][0], sourcePos[0][1], sourcePos[0][2], sourcePos[0][3],
						 destPos[0][0], destPos[0][1], destPos[0][2], destPos[0][3],
						 1.0f);
		// No match info
		parameterMatrix[0][1] = 0.0f; // enable textures
		glLoadMatrixf(parameterMatrix[0]);
		//int progress = (innerTime / 1500) % NUM_POINT_TEXTURES;
		if (innerTime < TIME_TRANSITION_CFP)
			noMatch.drawScreenAlignedQuad(-0.7f + rightMove, -0.5f, -0.05f + rightMove, 0.875f);
		parameterMatrix[0][1] = 1.0f; // disable textures
		glLoadMatrixf(parameterMatrix[0]);
		glColor3ub(180, 180, 180);
		drawBox(-0.7f + rightMove, -0.5f, -0.05f + rightMove, 0.875f);
	}

	// Draw down from currentFace
	drawFaceInterpol(&cfpContainer, currentFace,
					 sourcePos[0][0], sourcePos[0][1], sourcePos[0][2], sourcePos[0][3],
					 destPos[0][0], destPos[0][1], destPos[0][2], destPos[0][3],
					 moveT);
	if (currentFace + 1 < numFaces)
		drawFaceInterpol(&cfpContainer, currentFace+1,
						 sourcePos[1][0], sourcePos[1][1], sourcePos[1][2], sourcePos[1][3],
						 destPos[1][0], destPos[1][1], destPos[1][2], destPos[1][3],
						 moveT);
	if (currentFace + 2 < numFaces)
		drawFaceInterpol(&cfpContainer, currentFace+2,
						 sourcePos[2][0], sourcePos[2][1], sourcePos[2][2], sourcePos[2][3],
						 destPos[2][0], destPos[2][1], destPos[2][2], destPos[2][3],
						 moveT);
	
	// Draw biometric data
	parameterMatrix[0][1] = 1.0f; // disable textures
	glLoadMatrixf(parameterMatrix[0]);
	if (innerTime > TIME_TRANSITION_CFP)
	{
		cfpContainer.drawBiometric(currentFace, -0.95f + rightMove, 0.35f, -0.75f + rightMove, 0.775f);
	}

	// render competing biometric data		
	if (innerTime > TIME_OFF_RENDER_CFP + TIME_TRANSITION_CFP)
	{
		//int competitor = ((currentTime) / 80) % (cfpRefContainer.getNumTextures());
		double compTime = (double)currentTime / cfpSpeedup;
		int competitor = ((int)compTime / 80) % (cfpRefContainer.getNumTextures());
		glColor3ub(250, 180, 150);
		parameterMatrix[0][1] = 0.0f; // enable textures
		glLoadMatrixf(parameterMatrix[0]);

		if (currentFace == 2 && innerTime > TIME_IN_RENDER_CFP - 5000)
		{
			// Matching stuff
			parameterMatrix[0][1] = 0.0f; // enable textures
			parameterMatrix[3][3] = 1.0f; // enable color textures
			glLoadMatrixf(parameterMatrix[0]);
			int progress = ((innerTime - (TIME_IN_RENDER_CFP - 5000)) / 1500) % NUM_POINT_TEXTURES;
			match[progress].drawScreenAlignedQuad(-0.7f + rightMove, -0.5f, -0.05f + rightMove, 0.875f);
			parameterMatrix[0][1] = 1.0f; // disable textures
			parameterMatrix[3][3] = 0.0f; // disable color textures
			glLoadMatrixf(parameterMatrix[0]);
			glColor3ub(180, 180, 180);
			drawBox(-0.7f + rightMove, -0.5f, -0.05f + rightMove, 0.875f);
		}
		else
		{
			// render the database data
			int steckID = ((int)compTime / 80) % (NUM_STECKBRIEFE);
			steckbrief[steckID].drawScreenAlignedQuad(-0.7f + rightMove, -0.5f, -0.05f + rightMove, 0.875f);

			// render competitor
			cfpRefContainer.drawScreenAlignedQuad(competitor, -0.65f + rightMove, 0.35f, -0.45f + rightMove, 0.775f);
			// Draw biometric data
			parameterMatrix[0][1] = 1.0f; // disable textures
			glLoadMatrixf(parameterMatrix[0]);
			cfpRefContainer.drawBiometric(competitor, -0.65f + rightMove, 0.35f, -0.45f + rightMove, 0.775f);

			// render around
			glColor3ub(180, 180, 180);
			drawBox(-0.65f + rightMove, 0.35f, -0.45f + rightMove, 0.775f);
			drawBox(-0.7f + rightMove, -0.5f, -0.05f + rightMove, 0.875f);
			
			// render timer bar
			//glColor3ub(0, 0, 0);
			//fillBox(-0.44f + rightMove, 0.28f, -0.16f + rightMove, 0.26f);
			glColor3ub(180, 180, 180);
			drawBox(-0.65f + rightMove, -0.45f, -0.1f + rightMove, -0.42f);
			float timeProgress = (float)(innerTime - TIME_OFF_RENDER_CFP - TIME_TRANSITION_CFP) /
					(TIME_IN_RENDER_CFP - TIME_OFF_RENDER_CFP - TIME_TRANSITION_CFP);
			timeProgress = timeProgress < 0.0f ? 0.0f : timeProgress;		
			glColor3ub(230, 205, 128);
			fillBox(-0.65f + rightMove, -0.45f, -0.65f + rightMove + 0.55f * timeProgress, -0.42f);
		}
	}
	else if (innerTime > TIME_TRANSITION_CFP)
	{
		// Loading stuff
		parameterMatrix[0][1] = 0.0f; // enable textures
		glLoadMatrixf(parameterMatrix[0]);
		int progress = (innerTime / 1500) % NUM_POINT_TEXTURES;
		loading[progress].drawScreenAlignedQuad(-0.7f + rightMove, -0.5f, -0.05f + rightMove, 0.875f);
		parameterMatrix[0][1] = 1.0f; // disable textures
		glLoadMatrixf(parameterMatrix[0]);
		glColor3ub(180, 180, 180);
		drawBox(-0.7f + rightMove, -0.5f, -0.05f + rightMove, 0.875f);
	}
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

	// Initialize COM
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) exit(-1);

	// Create the window
    mainWnd = CreateWindow (szAppName,szAppName,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1300,520,0,0,hInstance,0);
	editWnd = CreateWindow (szAppName,szAppName,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1300,520,0,0,hInstance,0);
	//hwnd = CreateWindow(szAppName,szAppName,WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
	glInit();

	hr = cfpContainer.init("cfp.files");
	if (FAILED(hr))
	{
		MessageBox(mainWnd, "Failed to initialize main cfp container.", "Fatal error", MB_OK);
		exit(-1);
	}
	hr = cfpRefContainer.init("cfp.ref.files");
	if (FAILED(hr))
	{
		MessageBox(mainWnd, "Failed to initialize ref cfp container.", "Fatal error", MB_OK);
		exit(-1);
	}
	hr = assamese1.init("assamese1.tga");
	if (FAILED(hr))
	{
		MessageBox(mainWnd, "Failed to load assamese1.tga.", "Fatal error", MB_OK);
		exit(-1);
	}
	steckbrief[0].init("tex/Steckbrief1.tga");
	steckbrief[1].init("tex/Steckbrief2.tga");
	steckbrief[2].init("tex/Steckbrief3.tga");
	steckbrief[3].init("tex/Steckbrief4.tga");
	steckbrief[4].init("tex/Steckbrief5.tga");
	steckbrief[5].init("tex/Steckbrief6.tga");
	steckbrief[6].init("tex/Steckbrief7.tga");
	steckbrief[7].init("tex/Steckbrief8.tga");
	steckbrief[8].init("tex/Steckbrief9.tga");
	steckbrief[9].init("tex/Steckbrief10.tga");
	steckbrief[10].init("tex/Steckbrief11.tga");
	steckbrief[11].init("tex/Steckbrief12.tga");
	steckbrief[12].init("tex/Steckbrief13.tga");
	steckbrief[13].init("tex/Steckbrief14.tga");
	steckbrief[14].init("tex/Steckbrief15.tga");
	steckbrief[15].init("tex/Steckbrief16.tga");
	steckbrief[16].init("tex/Steckbrief17.tga");
	steckbrief[17].init("tex/Steckbrief18.tga");
	steckbrief[18].init("tex/Steckbrief19.tga");
	steckbrief[19].init("tex/Steckbrief20.tga");
	loading[0].init("tex/Loading1.tga");
	loading[1].init("tex/Loading2.tga");
	loading[2].init("tex/Loading3.tga");
	loading[3].init("tex/Loading4.tga");
	match[0].init("tex/Match1.tga");
	match[1].init("tex/Match2.tga");
	match[2].init("tex/Match3.tga");
	match[3].init("tex/Match4.tga");
	noMatch.init("tex/NoMatch.tga");

	// Initialize parameter matrix
	parameterMatrix[0][0] = 5.0f;

    ShowWindow(mainWnd,SW_SHOW);
    UpdateWindow(mainWnd);
    ShowWindow(editWnd,SW_SHOW);
    UpdateWindow(editWnd);
 
	// Initialize textures
	hr = videoTexture[0].init(1);
	if (FAILED(hr))
	{
		MessageBox(mainWnd, "Failed to initialize video texture 1.", "Critical error", MB_OK);
		exit(-1);
	}
	Sleep(300);
	hr = videoTexture[1].init(2);
	if (FAILED(hr))
	{
		MessageBox(mainWnd, "Failed to initialize video texture 2.", "Critical error", MB_OK);
		exit(-1);
	}

	long startTime = timeGetTime();
	long lastTime = 0;

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

		// update timer
		long curTime = timeGetTime() - startTime;
		long deltaTime = curTime - lastTime;
		lastTime = curTime;

		// capture video:
		if (additionalCapture == 2)
		{
			additionalCapture = 2;
		}
		videoTexture[0].captureFrame(additionalCapture);
		videoTexture[1].captureFrame(additionalCapture);
		additionalCapture = 0;
		videoTexture[0].updateFaceBoxes(boxFaceTime < -1);
		videoTexture[1].updateFaceBoxes(boxFaceTime < -1);

		// update parmeters
		if (parameterMatrix[0][0] < 5.0f)
		{
			if (parameterMatrix[0][0] < 2.0f)
			{
				parameterMatrix[0][0] += deltaTime * 0.0005f; // sobel distance
			}
			else
			{
				if (stopScan)
				{
					parameterMatrix[0][0] = 10.0f;
				}
				else
				{
					parameterMatrix[0][0] = -1.1f;
					parameterMatrix[0][3] += 0.2f * scanSpeedup; // sobel amount
					if (parameterMatrix[0][3] > 1.0f) parameterMatrix[0][3] = 1.0f;
				}
			}
		}
		parameterMatrix[0][1] = 0.0f; // enable textures
		if (chimereDisplay) parameterMatrix[0][2] = 1.0f; // display chimere
		else parameterMatrix[0][2] = 0.0f;
		//parameterMatrix[0][3] = 1.0; // sobel amount
		parameterMatrix[3][0] = mousePos[0];
		parameterMatrix[3][1] = mousePos[1];
		// Timing and movetable effect
		parameterMatrix[1][2] = (float)(curTime % 1000000) * 0.001f; // movetable time
		parameterMatrix[1][3] = 0.0f; // movetable amount

		// Update speedup depending on midi device
		introSpeedup = exp(params.getParam(2, 0.5f) * 2.0f - 1.0f);
		boxFaceSpeedup = exp(params.getParam(3, 0.5f) * 2.0f - 1.0f);
		cfpSpeedup = exp(params.getParam(4, 0.5f) * 2.0f - 1.0f);
		scanSpeedup = exp(params.getParam(5, 0.5f) * 2.0f - 1.0f);

		// two render loops
		for (int i = 0; i < 2; i++)
		{
			if (i == 0) wglMakeCurrent(editDC, editRC);
			else wglMakeCurrent(mainDC, mainRC);

			// movetabe effect only on release:
			if (i == 1)
			{
				if (introTime <= 0.0f)
				{
					if (introTime < -1.5f)
					{
						// no stuff...
						parameterMatrix[1][3] = 0.0f;
					}
					else
					{
						// total
						parameterMatrix[1][3] = 2.0f;
					}
				}
				else
				{
					parameterMatrix[1][3] = (float)(2.0f - 2.0f * (introTime / introLength));
				}
				if (introTime >= 0.0f)
				{
					introTime += introSpeedup * deltaTime;
					if (introTime > introLength) introTime = -2.0f;
				}
			}

			// overall opengl data
			glLineWidth(2.0f);
			//glColor3ub(200, 100, 50);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Set the parameters for the current frame
			parameterMatrix[0][1] = 0.0f; // enable textures
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(parameterMatrix[0]);
			glColor3ub(255, 255, 255);
		
			// Set the sobel shader
			glUseProgram(shaderPrograms[0]);

			if (cfpTime >= 0)
			{
				glColor3ub(128, 128, 128);
			}
			
			// capture frame (in external wrapper); update BG in first second
			//videoTexture[0].captureFrame(curTime < 1000);
			// render loop (screen aligned quad)
			// Set parameters for bounding box of second scan
			parameterMatrix[2][0] = -0.8f;
			parameterMatrix[2][1] = -0.8f;
			parameterMatrix[2][2] = -0.2f;
			parameterMatrix[2][3] = -0.6f;
			parameterMatrix[1][1] = 1.0f; // scanline
			parameterMatrix[0][1] = 0.0f; // enable textures
			glLoadMatrixf(parameterMatrix[0]);
			videoTexture[0].drawScreenSpaceQuad(0.0, -1.0, -1.0, 1.0);
			parameterMatrix[1][1] = 0.0f; // no scanline
			glLoadMatrixf(parameterMatrix[0]);
			if (cfpTime >= 0) renderCFPStuff((int)cfpTime, 0.0f);

			// capture frame (in external wrapper); update BG in first second
			//videoTexture[1].captureFrame(curTime < 1000);
			// render loop (screen aligned quad)
			parameterMatrix[2][0] = 0.2f;
			parameterMatrix[2][1] = -0.8f;
			parameterMatrix[2][2] = 0.8f;
			parameterMatrix[2][3] = -0.6f;
			parameterMatrix[1][1] = 1.0f; // scanline
			parameterMatrix[0][1] = 0.0f; // enable textures
			glLoadMatrixf(parameterMatrix[0]);
			videoTexture[1].drawScreenSpaceQuad(1.0, -1.0, 0.0, 1.0);
			parameterMatrix[1][1] = 0.0f; // no scanline
			glLoadMatrixf(parameterMatrix[0]);
			if (cfpTime >= 0) renderCFPStuff((int)cfpTime, 1.0f);

			// disable texture, draw stuff
			parameterMatrix[0][1] = 1.0f; // disable textures
			parameterMatrix[1][1] = 0.0f; // no scanline
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(parameterMatrix[0]);

			// draw some box
			if (redBoxDisplay == 2 && cfpTime < 0)
			{
				glColor3ub(255, 20, 10);
				drawBox(redBoxX - 0.015f, redBoxY - 0.04f, redBoxX + 0.015f, redBoxY + 0.04f);
			}

			// box face rendering
			if (cfpTime < 0)
			{
				// edit window: always render
				if (i == 0 || boxFaceTime == -2)
				{
					// Draw faces
					if (i == 0 && resetFaceBox[0] == 0) videoTexture[0].drawFaceBoxes(-1.0, -1.0, 0.0, 1.0, 10000000, boxFaceTime < -1, resetFaceBox[1]);
					else videoTexture[0].drawFaceBoxes(-1.0, -1.0, 0.0, 1.0, 10000000, boxFaceTime < -1);
					
					if (i == 0 && resetFaceBox[0] == 1) videoTexture[1].drawFaceBoxes(0.0, -1.0, 1.0, 1.0, 10000000, boxFaceTime < -1, resetFaceBox[1]);
					else videoTexture[1].drawFaceBoxes(0.0, -1.0, 1.0, 1.0, 10000000, boxFaceTime < -1);
				}
				else // render window, only render in boxFaceTime
				{
					videoTexture[0].drawFaceBoxes(-1.0, -1.0, 0.0, 1.0, (int)boxFaceTime, boxFaceTime < -1);
					videoTexture[1].drawFaceBoxes(0.0, -1.0, 1.0, 1.0, (int)boxFaceTime, boxFaceTime < -1);
				}
			}

			// Do the cfp updating if applicable
			if (cfpTime >= 0)
			{
				cfpTime += deltaTime * (double)cfpSpeedup;
			}

			// update box face time
			if (boxFaceTime >= 0)
			{
				boxFaceTime += deltaTime * (double)boxFaceSpeedup;
			}

			// This only in jo mode
			if (i == 0)
			{
				// Draw face box special face if in jo mode
				if (nextBoxSpecial >= 0)
				{
					parameterMatrix[0][1] = 0.0f; // enable textures
					glMatrixMode(GL_MODELVIEW);
					glLoadMatrixf(parameterMatrix[0]);
					cfpContainer.drawScreenAlignedQuad(nextBoxSpecial, -1.0f, 0.575f, -0.8f, 1.0f);
				}
			}

			// swap buffers
			Sleep(5);
			if (i == 0) wglSwapLayerBuffers(mainDC, WGL_SWAP_MAIN_PLANE);
			else wglSwapLayerBuffers(editDC, WGL_SWAP_MAIN_PLANE);
			Sleep(5);
		}

		Sleep(20);
    } while (msg.message != WM_QUIT);

	// Un-initialize videos
	// Does not work when alt-F4 is pressed?
	videoTexture[0].deinit();
	videoTexture[1].deinit();

	glUnInit();

	// Un-initialize COM
	CoUninitialize();

    return msg.wParam;
}

//Main Window Procedure WindowProc
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
	RECT rect;
	int width, height;
	int x, y;
	float xPos, yPos;

    switch (message)                  /* handle the messages */
    {

	case WM_CREATE:
        break;
 
    case WM_DESTROY:
        PostQuitMessage(0);   /* send a WM_QUIT to the message queue */
        break;

	case WM_MOUSEMOVE:
		GetClientRect(hwnd, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		mousePos[0] = (float)x * 2.0f / (float)width - 1.0f;
		mousePos[1] = 1.0f - (float)y * 2.0f / (float)height;
		break;

	case WM_LBUTTONDOWN:
		GetClientRect(hwnd, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		xPos = (float)x * 2.0f / (float)width - 1.0f;
		yPos = 1.0f - (float)y * 2.0f / (float)height;
		if (redBoxDisplay == 1)
		{
			redBoxX = (float)x * 2.0f / (float)width - 1.0f;
			redBoxY = 1.0f - (float)y * 2.0f / (float)height;
			redBoxDisplay = 2;
		}
		
		if (resetFaceBox[0] == 0)
		{
			videoTexture[0].setFaceRect(resetFaceBox[1], xPos + 1.0f, 0.5f * yPos + 0.5f);
			resetFaceBox[0] = -2;
		}
		if (resetFaceBox[0] == 1)
		{
			videoTexture[1].setFaceRect(resetFaceBox[1], xPos, 0.5f * yPos + 0.5f);
			resetFaceBox[0] = -2;
		}
		if (resetFaceBox[0] == -1)
		{
			if (xPos < 0.0f)
			{
				resetFaceBox[0] = 0;
				resetFaceBox[1] = videoTexture[0].getNearestFace(xPos + 1.0f, 0.5f * yPos + 0.5f);
			}
			else
			{
				resetFaceBox[0] = 1;
				resetFaceBox[1] = videoTexture[1].getNearestFace(xPos, 0.5f * yPos + 0.5f);
			}
		}
		break;

	case WM_RBUTTONDOWN:
		GetClientRect(hwnd, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		xPos = (float)x * 2.0f / (float)width - 1.0f;
		yPos = 1.0f - (float)y * 2.0f / (float)height;
		if (xPos < 0.0f)
		{
			videoTexture[0].addFace(xPos + 1.0f, 0.5f * yPos + 0.5f, nextBoxSpecial);
			if (nextBoxSpecial >= 0)
			{
				videoTexture[1].boxFaceSpecial[nextBoxSpecial] = -1;
				nextBoxSpecial = -1;
			}
		}
		else
		{
			videoTexture[1].addFace(xPos, 0.5f * yPos + 0.5f, nextBoxSpecial);
			if (nextBoxSpecial >= 0)
			{
				videoTexture[0].boxFaceSpecial[nextBoxSpecial] = -1;
				nextBoxSpecial = -1;
			}
		}
		break;

	case WM_KEYUP:
		switch (wParam)
		{
		case 'v':
		case 'V':
			chimereDisplay = false;
			break;
		}
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case 't':
		case 'T':
			if(FAILED(cfpContainer.init("cfp.files")))
			{
				MessageBox(mainWnd, "Failed to initialize main cfp container.", "Fatal error", MB_OK);
				exit(-1);
			}
			break;
		
		case 'q':
		case 'Q':
			redBoxDisplay = 1;
			stopScan = true;
			break;
		case 'w':
		case 'W':
			redBoxDisplay = 0;
			break;
		
#if 0
		case 'b':
		case 'B':
			additionalCapture = 1;
			break;
		case 'n':
		case 'N':
			additionalCapture = 2;
			break;
		case 'v':
		case 'V':
			chimereDisplay = true;
			break;
#else
		case 'b':
		case 'B':
			resetFaceBox[0] = -1;
			break;
		case 'n':
		case 'N':
			resetFaceBox[0] = -2;
			break;
#endif

		case '1':
			nextBoxSpecial = 0;
			break;
		case '2':
			nextBoxSpecial = 1;
			break;
		case '3':
			nextBoxSpecial = 2;
			break;
		case '0':
			nextBoxSpecial = -1;
			break;

		case 'a':
		case 'A':
			if (boxFaceTime == -2 && cfpTime < 0)
			{
				parameterMatrix[0][0] = -1.1f; // sobel time
				parameterMatrix[0][3] = 0.0f; // sobel amount
				parameterMatrix[1][0] = 0.0f; // do the sobel
				stopScan = false;
			}
			break;
		case 's':
		case 'S':
			stopScan = true;
			break;

		case 'd':
		case 'D':
			if (boxFaceTime == -2 && cfpTime < 0)
			{
				parameterMatrix[0][0] = -1.1f; // sobel time
				parameterMatrix[0][3] = 0.0f; // sobel amount
				parameterMatrix[1][0] = 1.0f; // do no sobel
				stopScan = false;
			}
			break;
		case 'f':
		case 'F':
			stopScan = true;
			break;

		case 'z':
		case 'Z':
			// before all intro...
			boxFaceTime = -1;
			cfpTime = -1;
			introTime = -1;
			stopScan = true;
			break;
		case 'u':
		case 'U':
			// go to start, intro runs
			introTime = 0;
			boxFaceTime = -1;
			cfpTime = -1;
			stopScan = true;
			break;
		case 'i':
		case 'I':
			introTime = -2;
			boxFaceTime = 0; // box Face time is starts here
			cfpTime = -1;
			stopScan = true;
			break;
		case 'o':
		case 'O':
			introTime = -2;
			cfpTime = 0;
			boxFaceTime = -2; // box Face time is already over. No more!
			stopScan = true;
			break;
		case 'p':
		case 'P':
			introTime = -2;
			cfpTime = -1;
			boxFaceTime = -2; // box Face time is completely over.
			break;

		case 'm':
		case 'M':
			SetWindowLong(hwnd, GWL_STYLE, WS_POPUP|WS_VISIBLE);
			ShowWindow(hwnd, SW_MAXIMIZE);
			GetClientRect(hwnd, &rect);
			if (hwnd == mainWnd) wglMakeCurrent(mainDC, mainRC);
			else wglMakeCurrent(editDC, editRC);
			glViewport(0, 0, rect.right-rect.left, abs(rect.bottom - rect.top)); //NEW
			break;

		case 'x':
		case 'X':
			videoTexture[0].removeLastFace();
			break;
		case 'c':
		case 'C':
			videoTexture[1].removeLastFace();
			break;

		default:
			break;
		}
 
    default:              /* for messages that we don't deal with */
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
