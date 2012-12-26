// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "chockNgt.h"
#include "bassmod.h"
#include "font.h"

LRESULT CALLBACK WindowProc (HWND, UINT, WPARAM, LPARAM);

HWND hWnd;

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
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0,
    PFD_MAIN_PLANE,
    0, 0, 0, 0
    };
// main window stuff
static GLuint fontTexture;
const int fontWidth = 960;
const int fontHeight = 144;
const int numValues = 48227;
unsigned char fontCompressed[4][fontHeight][fontWidth];
unsigned char font[fontHeight][fontWidth][4];
int fontX[256];
int fontY[256];
const char scroller[] = \
"        oooh, not another of those epic realtime demos you might say. i say, bring them on, good things are still to come! if it isn\'t for the circus that\'s driving you mad, maybe we\'re more successful with this nice little feature presentation. the key to success in getting insane is to stare on the cube and let the demo run for at least 10 minutes. or you could try to read all this babbling and get the infamous everything-moves-to-the-right-after-reading-a-scroller-for-too-long-syndrome. whatever suits your needs... let\'s shout out some greetings to our allies from nuance, cosine, genesis project, mostly harmless, squoquo, squopoo, jumalauta, toxie. finally we managed to put it all together. thanks guys! so hopefully you\'re able to read this text, because, in the end, the world might already have ended to turn... to fill up space here are some credits, music by dalezy / local conversions by others, code by widdy - wii and gba, nitro - vectrex, chock - pc 4k, chokes on dick - pc demo, hopper - atari xl, yoda - wild. scolltext by hopper        we finally reached the 1kb mark, so let\'s bring it all to a good end and as the mayans said - it will all repeat itself, so here you go...  *        ";

#define OFFSCREEN_WIDTH 512
#define OFFSCREEN_HEIGHT 256

#define glCreateShader ((PFNGLCREATESHADERPROC)glFP[0])
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)glFP[1])
#define glShaderSource ((PFNGLSHADERSOURCEPROC)glFP[2])
#define glCompileShader ((PFNGLCOMPILESHADERPROC)glFP[3])
#define glAttachShader ((PFNGLATTACHSHADERPROC)glFP[4])
#define glLinkProgram ((PFNGLLINKPROGRAMPROC)glFP[5])
#define glUseProgram ((PFNGLUSEPROGRAMPROC)glFP[6])
#define glTexImage3D ((PFNGLTEXIMAGE3DPROC)glFP[7])
#define glGetShaderiv ((PFNGLGETSHADERIVPROC)glFP[8])
#define glGetShaderInfoLog ((PFNGLGETSHADERINFOLOGPROC)glFP[9])

#define NUM_GL_NAMES 10
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
};

// TODO: Check implementation from somewhere else. Esp. %65535? Numeric recipies.
static unsigned long seed;
float frand()
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	//return (seed >> 8) % 65535;
	return (float)((seed>>8)%65535) * (1.0f/65536.0f);
}

// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];

static GLuint offscreenTexture;

typedef void (*GenFP)(void); // pointer to openGL functions
static GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderPrograms[2];
GLint viewport[4];

char err[4097];

// Name of the 32x32x32 noise texture
#define FLOAT_TEXTURE
#define NOISE_TEXTURE_SIZE 16 // try smaller?
static GLuint noiseTexture;
#ifdef FLOAT_TEXTURE
static float noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];
#else
static unsigned char noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];
#endif

const GLchar *fragmentMainBackground = ""
"uniform sampler3D Texture0;"
"varying vec3 z;"
"varying mat4 v;"
"varying vec2 vTexCoord;"
"vec3 noise(vec3 pos, int iterations, float reduction)"
"{"
"float n=1.;"
"pos*=2.;"
"vec3 result=vec3(0.);"
"for(;iterations>0;iterations--)"
"{"
"result += texture3D(Texture0,pos).xyz*n;"
"n*=reduction;"
"pos*=1.93;"
"}"
"return result;"
"}"
"vec3 color(vec3 pos, vec3 noiseStuff, vec3 colMult)"
"{"
   "float dist = length(pos);"
   "float color = smoothstep(0.3, 1.2, dist);"
   "float brightness = smoothstep(-1.2, 0.0, -color);"
   "float stuffi = noiseStuff.g * noiseStuff.b;"
   "colMult -= stuffi * (8.1, 18.5, 13.2);"
   "return abs(stuffi) * vec3(0.7, 0.4, 0.2) + mix(vec3(1.0)-colMult, colMult, color) * brightness + 0.2 * (1.0-brightness);"
"}"
"void main(void)"
"{  "
   "float fTime0_X = v[0][0];"
   "vec3 tvNoise =  noise(z*0.05 + vec3(0.,0.,fTime0_X*0.01), 8, 0.65);"
   "vec3 rayDir = normalize(z * vec3(1.0, 0.6, 1.0));"
   "vec3 fRayDir = rayDir;"
   "float sw1 = floor(fTime0_X);"
   "float sw2 = fTime0_X - sw1;"
   "float swoosher = sw1 + pow(sw2, 0.6);"
   "vec3 camPos = vec3(1.5*sin(swoosher * 0.7), -0.5+cos(swoosher * 1.3), -7.0 + 3.0 * sin(swoosher*0.5));"
   "float alpha = swoosher * 2.37;"
   "camPos.yz = vec2(cos(alpha)*camPos.y - sin(alpha)*camPos.z,"
                    "sin(alpha)*camPos.y + cos(alpha)*camPos.z);"
   "rayDir.yz = vec2(cos(alpha)*rayDir.y - sin(alpha)*rayDir.z,"
                    "sin(alpha)*rayDir.y + cos(alpha)*rayDir.z);"
   "alpha = swoosher * 2.17;"
   "camPos.xz = vec2(cos(alpha)*camPos.x - sin(alpha)*camPos.z,"
                    "sin(alpha)*camPos.x + cos(alpha)*camPos.z);"
   "rayDir.xz = vec2(cos(alpha)*rayDir.x - sin(alpha)*rayDir.z,"
                    "sin(alpha)*rayDir.x + cos(alpha)*rayDir.z);"
   "vec3 rayPos = camPos;"
   "vec3 fRayPos = vec3(sin(fTime0_X * 0.9), 2., cos(fTime0_X * 0.7));"
   "float sceneSize = 20.0;"
   "vec3 totalColor = vec3(0.);"
   "float stepSize;"
   "float totalDensity = 0.0;"
   "float coneSize = 0.003;   "
   "while(length(rayPos)<sceneSize && totalDensity < 0.95)"
   "{"
      "vec3 tmpPos = rayPos;   "
      "vec3 noiseData = noise(tmpPos*0.015+ vec3(0.0, fTime0_X*0.003, 0.0), 5, 0.5);"
      "float firstNoise = noiseData.r;"
      "firstNoise = clamp(0.4 - abs(firstNoise), 0.0, 0.4);"
      "firstNoise *= firstNoise * 2.;"
      "vec3 noiseData2 = noise(noiseData*0.2 + rayPos*0.1 - vec3(0.0, fTime0_X*0.017, 0.0), 5, 0.8);"
      "float noiseVal = noiseData2.r * 0.2;"
      "vec3 absPos = abs(tmpPos);"
      "float sphere = max(absPos.x, max(absPos.y, absPos.z)) - 1. - firstNoise;"
      "float sphere2 = abs(length(tmpPos) - 1.0 - 1.*firstNoise) + 0.1;"
      "float implicitVal;"
      "vec3 colMult;"
      "colMult = vec3(0.8, 0.3, 0.1);"
      "float outer = sphere2-1.8*firstNoise;"
      "outer = max(outer, min(outer, 0.) + 1.*noiseVal);      "
      "implicitVal = min(sphere, outer);"
      "float floorDist = fRayPos.y - 0.4f * fRayPos.z + 1.5f;"
      "totalColor += vec3(1./50., 1./70., 1./90.) * 0.75;"
      "totalDensity += 2./(300. * exp(2.0*max(implicitVal, 0.0)));"
      "if (implicitVal < 0.0)"
      "{"
         "float localDensity = clamp(0.0 - max(120.*noiseVal,0.1)*implicitVal, 0.0, 1.0);"
         "totalColor = totalColor + (1.-totalDensity) * color(rayPos*0.4, noiseData2, colMult) * localDensity;"
         "totalDensity = totalDensity + (1.-totalDensity) * localDensity;"
      "}"
      "if (floorDist < 0.)"
      "{"
         "float localDensity = 1.0;"
#if 0
         "vec3 col = vec3(0.0, 0.0, 0.0);"
		 "vec3 baseRayPos = 0.5*fRayPos - floor(0.5*fRayPos) - 0.5;"
         "if (baseRayPos.x * baseRayPos.z < 0.0)"
         "{"
             "col = vec3(0.3, 0.5, 0.7);"
         "}"
#else
         "float amount = 0.0;"
		 "vec3 baseRayPos = 0.5*fRayPos - floor(0.5*fRayPos) - 0.5 + 0.1 * noise(0.4 * fRayPos, 5, 0.5).xyz;"
		 "float dist = 2.0 * max(abs(abs(baseRayPos.x) - 0.25), abs(abs(baseRayPos.z) - 0.25));"
		 "amount = dist;"
         "if (baseRayPos.x * baseRayPos.z < 0.0)"
         "{"
             "amount = 1.-amount;"
         "}"
		 "amount = smoothstep(0.45, 0.55, amount);"
		 "vec3 col = mix(vec3(0.0, 0.0, 0.0), vec3(0.3, 0.5, 0.7), amount);"
#endif
         "totalColor = totalColor + (1.-totalDensity) * col * localDensity;"
         "totalDensity = totalDensity + (1.-totalDensity) * localDensity;          "
      "}"
      "stepSize = min(implicitVal, floorDist) * 0.8;"
      "stepSize = 0.01 + smoothstep(0.0, 2.0, stepSize) * 2.;"
      "stepSize = max(coneSize, stepSize);"
      "coneSize += stepSize * 0.003;"
      "rayPos += rayDir * stepSize;"
      "fRayPos += fRayDir * stepSize;"
   "}   "
   "float grad = normalize(rayPos).y;"
   "totalColor += (1.-totalDensity) * (grad * vec3(0.0,0.1,0.2) + (1.-grad)*vec3(0.0,0.2,0.3));"
   "gl_FragColor = vec4(totalColor-vec3(0.1), 1.0);"
"}";

const GLchar *fragmentOffscreenCopy="\
uniform sampler2D t;\
varying vec3 z;\
varying mat4 v;\
varying vec2 vTexCoord;\
void main(void)\
{\
  if (v[0][1] < 0.5) {\
    vec2 n=vec2(fract(sin(dot(z.xy+v[0][0],vec2(12.9898,78.233)))*43758.5453));\
	gl_FragColor=texture2D(t,.5*z.xy+.5+.0007*n)+n.x*.02;\
  } else {\
	vec2 pos = vTexCoord + vec2(-0.000, 0.0);\
	gl_FragColor = vec4(0.0);\
	for (int i = -0; i <= 0; i++) {\
		gl_FragColor += texture2D(t, pos)/1.0;\
		pos += vec2(0.000/1.0, 0.0);\
	}\
  }\
}";

const GLchar *vertexMainObject="\
varying vec3 z;\
varying mat4 v;\
varying vec2 vTexCoord;\
void main(void)\
{\
vTexCoord = vec2(gl_MultiTexCoord0);\
v=gl_ModelViewMatrix;\
z=vec3(gl_Vertex.xy,.99);\
gl_Position=vec4(z,1.);\
}";

// Initialize X and Y positions of the letters in the font
void initFont()
{
	for (int i = 0; i < 256; i++)
	{
		switch (i)
		{
		case 'a': fontX[i] = 1; fontY[i] = 0; break;
		case 'b': fontX[i] = 2; fontY[i] = 0; break;
		case 'c': fontX[i] = 3; fontY[i] = 0; break;
		case 'd': fontX[i] = 4; fontY[i] = 0; break;
		case 'e': fontX[i] = 5; fontY[i] = 0; break;
		case 'f': fontX[i] = 6; fontY[i] = 0; break;
		case 'g': fontX[i] = 7; fontY[i] = 0; break;
		case 'h': fontX[i] = 8; fontY[i] = 0; break;
		case 'i': fontX[i] = 9; fontY[i] = 0; break;
		case 'j': fontX[i] = 10; fontY[i] = 0; break;
		case 'k': fontX[i] = 11; fontY[i] = 0; break;
		case 'l': fontX[i] = 12; fontY[i] = 0; break;
		case 'm': fontX[i] = 13; fontY[i] = 0; break;
		case 'n': fontX[i] = 14; fontY[i] = 0; break;
		case 'o': fontX[i] = 15; fontY[i] = 0; break;
		case 'p': fontX[i] = 16; fontY[i] = 0; break;
		case 'q': fontX[i] = 17; fontY[i] = 0; break;
		case 'r': fontX[i] = 18; fontY[i] = 0; break;
		case 's': fontX[i] = 19; fontY[i] = 0; break;
		case 't': fontX[i] = 0; fontY[i] = 1; break;
		case 'u': fontX[i] = 1; fontY[i] = 1; break;
		case 'v': fontX[i] = 2; fontY[i] = 1; break;
		case 'w': fontX[i] = 3; fontY[i] = 1; break;
		case 'x': fontX[i] = 4; fontY[i] = 1; break;
		case 'y': fontX[i] = 5; fontY[i] = 1; break;
		case 'z': fontX[i] = 6; fontY[i] = 1; break;
		case '0': fontX[i] = 7; fontY[i] = 1; break;
		case '1': fontX[i] = 8; fontY[i] = 1; break;
		case '2': fontX[i] = 9; fontY[i] = 1; break;
		case '3': fontX[i] = 10; fontY[i] = 1; break;
		case '4': fontX[i] = 11; fontY[i] = 1; break;
		case '5': fontX[i] = 12; fontY[i] = 1; break;
		case '6': fontX[i] = 13; fontY[i] = 1; break;
		case '7': fontX[i] = 14; fontY[i] = 1; break;
		case '8': fontX[i] = 15; fontY[i] = 1; break;
		case '9': fontX[i] = 16; fontY[i] = 1; break;
		case '.': fontX[i] = 17; fontY[i] = 1; break;
		case ',': fontX[i] = 18; fontY[i] = 1; break;
		case '!': fontX[i] = 19; fontY[i] = 1; break;
		case '/': fontX[i] = 0; fontY[i] = 2; break;
		case '?': fontX[i] = 1; fontY[i] = 2; break;
		case '-': fontX[i] = 2; fontY[i] = 2; break;
		case '\'': fontX[i] = 3; fontY[i] = 2; break;
		default: fontX[i] = 0; fontY[i] = 0; break;
		}
	}
}

void glInit()
{	
	initFont();

	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop...

	// create noise Texture
#ifdef FLOAT_TEXTURE
	for (int i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData[i] = frand() - 0.5f;
	}
#else
	for (int i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData[i] = (unsigned char)rand();
	}
#endif

	// init objects:
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);	
	GLuint fOffscreenCopy = glCreateShader(GL_FRAGMENT_SHADER);
	shaderPrograms[0] = glCreateProgram();
	shaderPrograms[1] = glCreateProgram();
	// compile sources:
	glShaderSource(vMainObject, 1, &vertexMainObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(fMainBackground, 1, &fragmentMainBackground, NULL);
	glCompileShader(fMainBackground);
	glShaderSource(fOffscreenCopy, 1, &fragmentOffscreenCopy, NULL);
	glCompileShader(fOffscreenCopy);

	// Check programs
	int tmp, tmp2;
	glGetShaderiv(vMainObject, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(vMainObject, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "vMainObject shader error", MB_OK);
		return;
	}
	glGetShaderiv(fMainBackground, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(fMainBackground, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "fMainBackground shader error", MB_OK);
		return;
	}
	glGetShaderiv(fOffscreenCopy, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(fOffscreenCopy, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "fOffscreeCopy shader error", MB_OK);
		return;
	}

	// link shaders:
	glAttachShader(shaderPrograms[0], vMainObject);
	glAttachShader(shaderPrograms[0], fMainBackground);
	glLinkProgram(shaderPrograms[0]);
	glAttachShader(shaderPrograms[1], vMainObject);
	glAttachShader(shaderPrograms[1], fOffscreenCopy);
	glLinkProgram(shaderPrograms[1]);

	// Create a rendertarget texture
	glGenTextures(1, &offscreenTexture);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT, 0,
		         GL_RGBA, GL_UNSIGNED_BYTE, 0);

#if 1
	// RLE uncompress
	int pos = 0;
	for (int k = 0; k < numValues; k++)
	{
		for (int i = 0; i < fontLength[k]; i++)
		{
			fontCompressed[0][0][pos+fontLength[k]-i-1] = fontValues[k];
		}
		pos += fontLength[k];
	}

	// uncompress font
	for (int color = 0; color < 4; color++)
	{
		for (int y = 0; y < fontHeight; y++)
		{
			for (int x = 0; x < fontWidth; x++)
			{
				font[y][x][color] = fontCompressed[color][y][x];
			}
		}
	}

	// Set texture.
	glEnable(GL_TEXTURE_3D); // automatic?
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
#ifdef FLOAT_TEXTURE
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F,
				 NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
				 0, GL_RGBA, GL_FLOAT, noiseData);
#else
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8,
				 NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
				 0, GL_RGBA, GL_UNSIGNED_BYTE, noiseData);
#endif

	// Load font texture
	glGenTextures(1, &fontTexture);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// TODO: Mip Mapping!!!!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, fontWidth, fontHeight,
					  GL_BGRA, GL_UNSIGNED_BYTE, font);
	glDisable(GL_TEXTURE_2D);						// Enable Texture Mapping
#endif
}

void glUnInit()
{
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
    //mainWnd = CreateWindow (szAppName,szAppName,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1024,600,0,0,hInstance,0);
	//mainWnd = CreateWindow("epic","epic",WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
    hWnd = CreateWindow( "static",0,WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,0,0);
    HDC hDC = GetDC(hWnd);
    // initalize opengl
    if( !SetPixelFormat(hDC,ChoosePixelFormat(hDC,&pfd),&pfd) ) return -1;
    HGLRC hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC,hRC);
	glInit();

    ShowWindow(hWnd,SW_SHOW);
    UpdateWindow(hWnd);
 
	long startTime = timeGetTime();
	long lastTime = 0;

	// start music playback
	/* setup output - default device, 44100hz, stereo, 16 bits */
	BASSMOD_Init(-1,44100,BASS_DEVICE_NOSYNC);
	BASSMOD_MusicLoad(FALSE,"dalezy - trollmannen_s krypt..mod",0,0,BASS_MUSIC_LOOP);
	BASSMOD_MusicPlay();

	float fCurTime;
	GetAsyncKeyState(VK_ESCAPE);

	do
    {

		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (!IsDialogMessage(hWnd, &msg))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}
		if (msg.message == WM_QUIT) break; // early exit on quit

		SetCursor(FALSE);

		// update timer
		long curTime = timeGetTime() - startTime;
		fCurTime = (float)curTime * 0.001f;
		long deltaTime = curTime - lastTime;
		float fDeltaTime = (float) deltaTime * 0.001f;
		lastTime = curTime;

		// render
		wglMakeCurrent(hDC,hRC);
		glClearColor(0.0f, 0.7f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// write your rendering code here!
		glMatrixMode(GL_MODELVIEW);
		parameterMatrix[0] = fCurTime; // time	
		parameterMatrix[1] = 0.0f; // no font mode
		glLoadMatrixf(parameterMatrix);

		// draw offscreen
		glBindTexture(GL_TEXTURE_3D, noiseTexture);
		glGetIntegerv(GL_VIEWPORT, viewport);
		glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
		glUseProgram(shaderPrograms[0]);
		glRectf(-1.0, -1.0, 1.0, 1.0);

		//// copy to front
		glViewport(0, 0, viewport[2], viewport[3]);
		glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
		glBindTexture(GL_TEXTURE_2D, offscreenTexture);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
		glUseProgram(shaderPrograms[1]);	
		glRectf(-1.0, -1.0, 1.0, 1.0);
		glDisable(GL_TEXTURE_2D);						// Enable Texture Mapping

		// draw font
		parameterMatrix[1] = 1.0f; // font mode
		glLoadMatrixf(parameterMatrix);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, fontTexture);
		glBegin(GL_QUADS);
		for (int i = 0; i < sizeof(scroller)-1; i++)
		{
			float pos = 15.0f - fCurTime * 10.0f;
			int ch = scroller[i];
			float xp = (fontX[ch] * 48.0f + 1.0f) / fontWidth;
			float yp = (fontY[ch] * 48.0f + 1.0f) / fontHeight;
			float w = 45.5f / fontWidth;
			float h = 45.5f / fontHeight;
			// repeat scroller
			while (pos < -1.0f * sizeof(scroller)) pos += 1.0f * sizeof(scroller);
			glTexCoord2f(xp, yp + h);
			glVertex3f((pos + i*1.0f)*0.2f/1920.0f*1080.0f, 0.7f, 0.5f);
			glTexCoord2f(xp, yp);
			glVertex3f((pos + i*1.0f)*0.2f/1920.0f*1080.0f, 0.9f, 0.5f);
			glTexCoord2f(xp+w, yp);
			glVertex3f((pos + i*1.0f+1.0f)*0.2f/1920.0f*1080.0f, 0.9f, 0.5f);
			glTexCoord2f(xp+w, yp + h);
			glVertex3f((pos + i*1.0f+1.0f)*0.2f/1920.0f*1080.0f, 0.7f, 0.5f);
		}
		glEnd();
		glDisable(GL_BLEND);

		// swap buffers
		wglSwapLayerBuffers(hDC, WGL_SWAP_MAIN_PLANE);

		//Sleep(5);
    } while (msg.message != WM_QUIT && fCurTime < 230.0f && !GetAsyncKeyState(VK_ESCAPE));

	// music uninit
	BASSMOD_MusicStop();
	BASSMOD_MusicFree(); // free the current mod

	wglDeleteContext(hRC);
	ReleaseDC(hWnd, hDC);
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
