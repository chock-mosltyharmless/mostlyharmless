// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "chockNgt.h"
#include "bass.h"
#include "font.h"

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
unsigned char scroller[] = "asjdhgajhsdg ajhsgd jahsg djahsgd jhasg djahsg djhasgdjhasg djhag djhagjh dag jhsdga sjhdg ajhsg djahsg djhagsj dhag hsjdg ajshdg ajshgd ajhsg djhasg djhag djhajs dhga jsdhg asd";

#define OFFSCREEN_WIDTH 1024
#define OFFSCREEN_HEIGHT 512

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

// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];

static GLuint offscreenTexture;

typedef void (*GenFP)(void); // pointer to openGL functions
static GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderPrograms[2];
GLint viewport[4];

char err[4097];

const GLchar *fragmentMainBackground = ""
 "varying vec2 vTexCoord;"
 "varying vec3 z;"
 "varying mat4 v;"
 "vec4 n(vec4 v)"
 "{"
   "return fract((v.zxwy+vec4(.735,.369,.438,.921))*vec4(9437.4,7213.5,5935.72,4951.6));"
 "}"
 "vec4 n(vec4 v,vec4 s)"
 "{"
   "return vec4(v.x*s.x-dot(v.yzw,s.yzw),v.x*s.yzw+s.x*v.yzw+cross(v.yzw,s.yzw));"
 "}"
 "mat3 s(vec4 v)"
 "{"
   "vec4 s=v.x*v,c=v.y*v,z=v.z*v;"
   "return mat3(1.)-2.*mat3(c.y+z.z,z.w-s.y,-s.z-c.w,-s.y-z.w,s.x+z.z,s.w-c.z,c.w-s.z,-c.z-s.w,s.x+c.y);"
 "}"
 "float m(vec3 v)"
 "{"
   "v=v.xzy*5.;"
   "vec3 s=v*v;"
   "float z=s.x+2.25*s.y+s.z-1.,c=s.x*s.z*v.z+.08888*s.y*s.z*v.z;"
   "return z*z*z-c;"
 "}"
 "void main()"
 "{"
   "vec4 c=v[1];"
   "vec3 f=normalize(z*vec3(1.,.6,1.)),l=vec3(0.,0.,-v[0][1]);"
   "float i=v[0][2]*4.5;"
   "l.xz=vec2(cos(i)*l.x-sin(i)*l.z,sin(i)*l.x+cos(i)*l.z);"
   "f.xz=vec2(cos(i)*f.x-sin(i)*f.z,sin(i)*f.x+cos(i)*f.z);"
   "vec3 x=l;"
   "float r=8.;"
   "vec3 w=vec3(0.);"
   "float y,e=0.,a=0.;"
   "for(int g=0;length(x)<r&&e<.9&&g<50;g++)"
     "{"
       "float b;"
       "vec4 t=normalize(vec4(cos(v[0][3]),sin(v[0][3]),sin(v[0][3]*1.3),sin(v[0][3]*2.7)));"
       "float o=1.;"
       "vec3 d=vec3(0.),h=vec3(1.,.4,.2);"
       "if(v[1][2]<.5)"
         "{"
           "b=1e+10;"
           "for(int u=0;u<12;u++)"
             "{"
               "vec4 k;"
               "float p;"
               "vec3 F,C;"
               "mat3 Z=s(t);"
               "vec4 Y=c;"
               "for(int X=0;X<4;X++)"
                 "{"
                   "Y=n(Y);"
                   "vec3 W=o*(Y.xyz*Y.xyz*Y.xyz*Y.xyz*vec3(.2)+vec3(.05));"
                   "b=min(b,length(max(abs((x-(.5*Y.wzx-vec3(.25))*Z*o-d)*s(n(normalize(Y-vec4(.5)),t)))-W,0.))-length(W)*.3);"
                 "}"
               "float W=1e+10;"
               "for(int X=0;X<2;X++)"
                 "{"
                   "Y=n(Y);"
                   "vec4 V=n(normalize(Y-vec4(.5)),t);"
                   "float U=o*(Y.x*.3+.25);"
                   "vec3 T=(.5*Y.wzx-vec3(.25))*Z*o+d;"
                   "float S=length(x-T)-U;"
                   "if(S<W)"
                     "W=S,k=V,p=U,F=T,C=Y.xyz;"
                 "}"
               "if(W>b)"
                 "{"
                   "break;"
                 "}"
               "else"
                 " t=k,o=p,d=F,h=.5*h+.5*C;"
             "}"
         "}"
       "else"
         "{"
           "float Y=.01,W=m(x);"
           "vec3 X=1./Y*(vec3(m(x+vec3(Y,0.,0.)),m(x+vec3(0.,Y,0.)),m(x+vec3(0.,0.,Y)))-vec3(W));"
           "b=W/(length(X)+Y);"
           "h=vec3(1.,.2,.2);"
         "}"
       "w+=vec3(.02,1./70.,1./90.)*3.06/exp(abs(b*5.))*v[3][3];"
       "e+=1./15./exp(abs(b*10.)+.5);"
       "a+=abs(b)*.99;"
       "if(b<0.)"
         "w=w+(1.-e)*h,e=1.f;"
       "y=max(.005*a,abs(b)*.99);"
       "x+=f*y;"
     "}"
   "float Y=normalize(f).y;"
   "w+=(1.-e)*(Y*vec3(0.,-.4,-.3)+(1.-Y)*vec3(0.,.4,.6));"
   "gl_FragColor=vec4(w-vec3(0.),1.);"
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
	vec2 pos = vTexCoord + vec2(-0.003, 0.0);\
	gl_FragColor = vec4(0.0);\
	for (int i = -5; i <= 5; i++) {\
		gl_FragColor += texture2D(t, pos)/11.0;\
		pos += vec2(0.003/5.0, 0.0);\
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

#ifdef SHADER_DEBUG
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
#endif

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
    HWND hWnd = CreateWindow( "static",0,WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,0,0);
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
	BASS_Init(-1,44100,0,hWnd,NULL);
	//mp3Str=BASS_StreamCreateFile(FALSE,"GT_muc.mp3",0,0,0);
	//BASS_ChannelPlay(mp3Str, TRUE);
	//BASS_Start();
	float fCurTime;
	GetAsyncKeyState(VK_ESCAPE);

	do
    {
		SetCursor(FALSE);

		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (!IsDialogMessage(hWnd, &msg))
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
		wglMakeCurrent(hDC,hRC);
		glClearColor(0.0f, 0.7f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// write your rendering code here!
		glMatrixMode(GL_MODELVIEW);
		parameterMatrix[0] = fCurTime; // time	
		parameterMatrix[1] = 0.0f; // no font mode
		glLoadMatrixf(parameterMatrix);

		// draw offscreen
		glGetIntegerv(GL_VIEWPORT, viewport);
		glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
		glUseProgram(shaderPrograms[0]);
		glRectf(-0.1, -0.1, 0.0, 0.0);

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
			float pos = 1.0f - fCurTime * 1.0f;
			int ch = scroller[i];
			float xp = (fontX[ch] * 48.0f + 0.5f) / fontWidth;
			float yp = (fontY[ch] * 48.0f + 0.5f) / fontHeight;
			float w = 46.5f / fontWidth;
			float h = 46.5f / fontHeight;
			glTexCoord2f(xp, yp + h);
			glVertex3f(pos + (i*1.0f)*0.2f/1920.0f*1080.0f, 0.7f, 0.5f);
			glTexCoord2f(xp, yp);
			glVertex3f(pos + (i*1.0f)*0.2f/1920.0f*1080.0f, 0.9f, 0.5f);
			glTexCoord2f(xp+w, yp);
			glVertex3f(pos + (i*1.0f+1.0f)*0.2f/1920.0f*1080.0f, 0.9f, 0.5f);
			glTexCoord2f(xp+w, yp + h);
			glVertex3f(pos + (i*1.0f+1.0f)*0.2f/1920.0f*1080.0f, 0.7f, 0.5f);
		}
		glEnd();
		glDisable(GL_BLEND);

		// swap buffers
		wglSwapLayerBuffers(hDC, WGL_SWAP_MAIN_PLANE);

		//Sleep(5);
    } while (msg.message != WM_QUIT && fCurTime < 230.0f && !GetAsyncKeyState(VK_ESCAPE));

	// music uninit
	//BASS_ChannelStop(mp3Str);
	//BASS_StreamFree(mp3Str);
	BASS_Free();

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
