// Camera_capture.cpp : Definiert den Einstiegspunkt für die Anwendung.
//
#include "stdafx.h"
#include "chockNgt.h"
//#include "bass.h"
#include "mathhelpers.h"
#include "TextureManager.h"
#include "Audio.h"
#include "Configuration.h"
#include "Zimmer.h"
#include "Prolog.h"
#include "Karaoke.h"
#include "Cafe.h"
#include "Smartphones.h"
#include "Car.h"

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

float fCurTime = 0.0f;

bool mod_key_ = false;
// Used to avoid double-pressing the same key
float last_key_press_time_[65536] = { 0.0f };

TextureManager textureManager;
Audio audio_;
Zimmer zimmer_;
Prolog prolog_;
Karaoke karaoke_;
Cafe cafe_;
Smartphones smartphones_;
Car car_;
enum SETTING {
    PROLOG,
    ZIMMER,
    KARAOKE,
    CAFE,
    BAHNHOF,
    PROBE,
    SMARTPHONES,
    CAR
};
int next_scene_id_ = 0;  // Go to this after the current scene is finished
bool end_current_scene_ = false;  // Wait for current scene to end...
SETTING scene_to_show_ = PROLOG;  // What is rendered

bool music_is_playing = false;
bool noise_is_playing = false;

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
    if (audio_.Init(errorString)) {
        MessageBox(mainWnd, errorString, "Audio Initialization", MB_OK);
        return;
    }
}

void glUnInit()
{
	wglDeleteContext(mainRC);
	ReleaseDC(mainWnd, mainDC);
}

void DrawQuadColor(float startX, float endX, float startY, float endY,
                   float startU, float endU, float startV, float endV,
                   float red, float green, float blue, float alpha) {
    // set up matrices
    /*
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    */
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    glColor4f(red, green, blue, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(startU, endV);
    glVertex3f(startX, endY, 0.99f);
    glTexCoord2f(endU, endV);
    glVertex3f(endX, endY, 0.99f);
    glTexCoord2f(endU, startV);
    glVertex3f(endX, startY, 0.99f);
    glTexCoord2f(startU, startV);
    glVertex3f(startX, startY, 0.99f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void DrawQuadColor(float startX, float endX, float startY, float endY, float red, float green, float blue, float alpha) {
    DrawQuadColor(startX, endX, startY, endY, 0.0f, 1.0f, 0.0f, 1.0f, red, green, blue, alpha);
}

void DrawQuad(float startX, float endX, float startY, float endY, float startU, float endU, float startV, float endV, float alpha) {
    DrawQuadColor(startX, endX, startY, endY, startU, endU, startV, endV, 1.0f, 1.0f, 1.0f, alpha);
}
void DrawQuad(float startX, float endX, float startY, float endY, float startV, float endV, float alpha) {
    DrawQuad(startX, endX, startY, endY, 0.0f, 1.0f, startV, endV, alpha);
}
void DrawQuad(float startX, float endX, float startY, float endY, float alpha) {
    DrawQuad(startX, endX, startY, endY, 0.0f, 1.0f, 0.0f, 1.0f, alpha);
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
    wc.lpszClassName = "hotparticle";

    RegisterClass (&wc);

	// Create the window
#ifdef FULLSCREEN
    mainWnd = CreateWindow(wc.lpszClassName,"hot particle",WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,hInstance,0);
#else
	mainWnd = CreateWindow(wc.lpszClassName,"hot particle",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,XRES,YRES,0,0,hInstance,0);
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
	GetAsyncKeyState(VK_ESCAPE);

    char error_string[MAX_ERROR_LENGTH+1];

    const float kTransformationMatrix[4][4] = {
        {1.2f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    };

	do
    {
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			//if (!IsDialogMessage(mainWnd, &msg))
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
        //GLuint tex_id;

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(kTransformationMatrix[0]);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        int return_value = 1;  // Assume we are finished
        switch (scene_to_show_) {
        case PROLOG:
            return_value = prolog_.Draw(fCurTime);
            break;
        case ZIMMER:
        case PROBE:
            return_value = zimmer_.Draw(fCurTime);
            break;
        case KARAOKE:
            return_value = karaoke_.Draw(fCurTime);
            break;
        case CAFE:
            return_value = cafe_.Draw(fCurTime);
            break;
        case SMARTPHONES:
            return_value = smartphones_.Draw(fCurTime);
            break;
        case CAR:
            return_value = car_.Draw(fCurTime);
            break;
        default:
            break;
        }

        // Check for error
        if (return_value < 0) return -1;

        // Check for end of scene to switch it
        if (end_current_scene_) {
            if (return_value == 1) {
                zimmer_.UpdateTime(fCurTime);
                prolog_.UpdateTime(fCurTime);
                karaoke_.UpdateTime(fCurTime);
                cafe_.UpdateTime(fCurTime);
                car_.UpdateTime(fCurTime);
                smartphones_.UpdateTime(fCurTime);
                // It has ended, start a new one
                audio_.StopSound(0, 36.0f, error_string);
                end_current_scene_ = false;
                switch(next_scene_id_) {
                case 0:
                    prolog_.ToBeginning();
                    scene_to_show_ = PROLOG;
                    break;
                case 1:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(MAERZ_11);
                    break;
                case 2:
                    karaoke_.ToBeginning();
                    scene_to_show_ = KARAOKE;
                    karaoke_.StartScene(TRENNUNG);
                    break;
                case 3:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(APRIL_09);
                    break;
                case 4:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(APRIL_16);
                    break;
                case 5:  // Bahnhof
                    karaoke_.ToBeginning();
                    scene_to_show_ = KARAOKE;
                    karaoke_.StartScene(BAHNHOF_BAR);
                    break;
                case 6:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(APRIL_17);
                    break;
                case 7:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(APRIL_21);
                    break;
                case 8:
                    car_.ToBeginning();
                    scene_to_show_ = CAR;
                    car_.StartScene(BEGRUSSUNG);
                    break;
                case 9:
                    car_.ToBeginning();
                    scene_to_show_ = CAR;
                    car_.StartScene(TOMOBE);
                    break;
                case 10:
                    car_.ToBeginning();
                    scene_to_show_ = CAR;
                    car_.StartScene(SIEVERT);
                    break;
                case 11:  // deleted scene
                case 12:
                    car_.ToBeginning();
                    scene_to_show_ = CAR;
                    car_.StartScene(TAMURA);
                    break;
                case 13:
                    car_.ToBeginning();
                    scene_to_show_ = CAR;
                    car_.StartScene(KATSURAO13);
                    break;
                case 14:
                    car_.ToBeginning();
                    scene_to_show_ = CAR;
                    car_.StartScene(KATSURAO14);
                    break;
                case 15:  // Minamisoma is smartphones only
                    smartphones_.ToBeginning();
                    smartphones_.StartScene(SM_MINAMISOMA);
                    scene_to_show_ = SMARTPHONES;
                    break;
                case 16:
                    car_.ToBeginning();
                    scene_to_show_ = CAR;
                    car_.StartScene(ABSCHIED);
                    break;
                case 17:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(MAI_10);
                    break;
                case 18:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(JUNI_01);
                    break;
                case 19:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(JUNI_04);
                    break;
                case 20:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(JUNI_05);
                    break;
                case 21:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(JUNI_12);
                    break;
                case 22:
                    karaoke_.ToBeginning();
                    scene_to_show_ = KARAOKE;
                    karaoke_.StartScene(MITARBEITER);
                    break;
                case 23:
                    karaoke_.ToBeginning();
                    scene_to_show_ = KARAOKE;
                    karaoke_.StartScene(SEKUHARA);
                    break;
                case 24:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(JULI_29);
                    break;
                case 25:  // CAFE
                    cafe_.ToBeginning();
                    scene_to_show_ = CAFE;
                    cafe_.StartScene(0);
                    break;
                case 26:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(AUGUST_15);
                    break;
                case 27:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(PROBERAUM);
                    break;
                case 28:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(MAERZ_11_END);
                    break;
                case 29:
                    zimmer_.ToBeginning();
                    scene_to_show_ = ZIMMER;
                    zimmer_.StartScene(UNKNOWN);
                    break;
                default:
                    break;  // Ignore that scene, it's not implemented
                }
            }
        }

#if 0
        // Draw video background
        float videoTime = fCurTime;
        if (textureManager.getVideoID("Kenshiro20_vonoben.wmv", &texID, errorString, videoTime) < 0) {
            MessageBox(mainWnd, errorString, "Texture manager get video ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, texID);
        DrawQuad(-0.5f, 0.5f, 0.7f, -0.7f, 0.0f, 1.0f, 1.0f);

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
			DrawQuad(-0.5f, 0.5f, -0.5f, 0.91f, 0.0f, 1.0f, alpha);

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
			DrawQuad(-0.5f, 0.5f, -0.5f, 0.91f, 0.0f, 1.0f, alpha*0.75f);

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
			DrawQuad(-0.5f, 0.5f, -0.5f, 0.91f, 0.0f, 1.0f, alpha*0.75f);

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
					DrawQuad(centerX - width, centerX + width,
							 centerY - width * ASPECT_RATIO, centerY + width * ASPECT_RATIO,
							 0.0f, 1.0f, amount);
				}
			}


			glDisable(GL_BLEND);
		}
#endif

		// draw background
		//DrawQuad(-0.3f, 0.8f, -0.2f, 0.7f, 0.4f, 1.0f, 1.0f);
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

void EndCurrentScene(bool switch_to_next_scene) {
    zimmer_.EndKenchiro();
    zimmer_.EndScene();
    prolog_.EndVideo();
    prolog_.EndLight();
    karaoke_.EndKenchiro();
    karaoke_.EndScene();
    cafe_.EndScene();
    cafe_.EndVideo();
    // What do I do with car and smartphones???
    car_.EndScene();
    smartphones_.EndScene();
    char error_string[MAX_ERROR_LENGTH+1];
    audio_.StopSound(0, 36.0f, error_string);    
    end_current_scene_ = switch_to_next_scene;
    audio_.StopSound(2, 18.0f, error_string);
    audio_.StopSound(3, 18.0f, error_string);
    music_is_playing = false;
    noise_is_playing = false;
}

//Main Window Procedure WindowProc
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    char error_string[MAX_ERROR_LENGTH + 1];
    int cur_mod_key = mod_key_;

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
    case WM_SYSKEYDOWN:
        mod_key_ = false;
        if (fCurTime - last_key_press_time_[wParam] > 0.5f) {
            last_key_press_time_[wParam] = fCurTime;
            switch (wParam)
            {
            case VK_F1:
                mod_key_ = true;
                break;

            case '1':
                if (cur_mod_key) { EndCurrentScene(true); next_scene_id_ = 0; }
                break;
            case '2':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 1;
                }
                break;
            case '3':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 2;
                }
                break;
            case '4':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 3;
                }
                break;
            case '5':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 4;
                }
                break;
            case '6':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 5;
                }
                break;
            case '7':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 6;
                }
                break;
            case '8':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 7;
                }
                break;
            case '9':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 8;
                }
                break;
            case '0':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 9;
                }
                break;
            case 'Q':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 10;
                }
                else {
                    EndCurrentScene(true);
                    next_scene_id_--;
                    if (next_scene_id_ == 11) next_scene_id_--;  // Kawauchi no longer exists
                    if (next_scene_id_ == 23) next_scene_id_--;
                    if (next_scene_id_ < 0) next_scene_id_ = 0;
                }
                break;
            case 'W':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 11;
                }
                break;
            case 'E':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 12;
                }
                break;
            case 'R':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 13;
                }
                break;
            case 'T':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 14;
                }
                break;
            case 'Y':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 15;
                }
                break;
            case 'U':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 16;
                }
                break;
            case 'I':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 17;
                }
                break;
            case 'O':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 18;
                }
                break;
            case 'P':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 19;
                }
                break;
            case 'A':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 20;
                }
                break;
            case 'S':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 21;
                }
                break;
            case 'D':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 22;
                }
                break;
            case 'F':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 23;
                }
                break;
            case 'G':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 24;
                }
                break;
            case 'H':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 25;
                }
                break;
            case 'J':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 26;
                }
                break;
            case 'K':
                if (cur_mod_key) {
                    EndCurrentScene(true);
                    next_scene_id_ = 27;
                }
                break;
            case 'L':
                if (cur_mod_key) { EndCurrentScene(true); next_scene_id_ = 28; }
                break;
            case 'Z':
                if (cur_mod_key) { EndCurrentScene(true); next_scene_id_ = 29; }
                else {
                    EndCurrentScene(false);
                }
                break;

            case VK_RETURN:
                EndCurrentScene(true);
                next_scene_id_++;
                if (next_scene_id_ == 11) next_scene_id_++;  // Kawauchi no longer exists
                if (next_scene_id_ == 23) next_scene_id_++;  // Second kneipe deleted
                if (next_scene_id_ > 29) next_scene_id_ = 29;
                break;

            case 'V':
                zimmer_.GoToErdbeben();
                zimmer_.NextPanya();
                if (scene_to_show_ == CAR) {
                    audio_.PlaySound("panya_klingelton.wav", 1, false, -1, error_string);
                }
                break;

            case 'M':
                if (scene_to_show_ == SMARTPHONES) smartphones_.TakeNextPicture();
                if (scene_to_show_ == PROLOG) {
                    prolog_.StartVideo();
                    audio_.PlaySound("Fukushima-Fahrt_small.wav", 0, false, -1, error_string);
                }
                if (scene_to_show_ == ZIMMER) zimmer_.StartKenchiro();
                if (scene_to_show_ == PROBE) zimmer_.StartKenchiro();
                if (scene_to_show_ == KARAOKE) karaoke_.StartKenchiro();
                if (scene_to_show_ == CAFE) cafe_.StartVideo();
                break;
                //case 'C':
                    //EndCurrentScene(false);
                    //break;

            case VK_UP:
                audio_.PlaySound("punch.wav", 0, false, -1, error_string);
                break;
            case VK_DOWN:
                audio_.PlaySound("01_Donner.wav", 0, false, -1, error_string);
                break;
            case VK_LEFT:
                if (!music_is_playing) {
                    audio_.PlaySound("musik_bearb.wav", 2, false, -1, error_string);
                    audio_.StopSound(3, 18.0f, error_string);
                    noise_is_playing = false;
                    music_is_playing = true;
                }
                else {
                    audio_.StopSound(2, 18.0f, error_string);
                    music_is_playing = false;
                }
                break;
            case VK_RIGHT:
                if (!noise_is_playing) {
                    audio_.StopSound(2, 18.0f, error_string);
                    music_is_playing = false;
                    noise_is_playing = true;
                    switch (scene_to_show_) {
                    case CAFE:
                        audio_.PlaySound("cafe.wav", 3, true, 24.0f, error_string, 0.3f);
                        break;
                    case KARAOKE:
                        audio_.PlaySound("kneipe.wav", 3, true, 24.0f, error_string, 0.2f);
                        break;
                    case CAR:
                        audio_.PlaySound("fahrt.wav", 3, true, 24.0f, error_string, 0.04f);
                        break;
                    }
                }
                else {
                    audio_.StopSound(3, 36.0f, error_string);
                    noise_is_playing = false;
                }

            default:
                break;
            }
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
