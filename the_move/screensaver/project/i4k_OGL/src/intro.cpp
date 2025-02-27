//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"
#include <stdlib.h>

#include "config.h"
#include "intro.h"
#include "Parameter.h"
#include "GLNames.h"
#include "TextureManager.h"
#include "FlowIcon.h"

float frand();

float screenLeft;
float screenRight;
float screenBottom;
float screenTop;

extern int realXRes;
extern int realYRes;
extern DWORD demoStartTime;
extern bool isScreenSaverRunning;
extern DWORD screenSaverStartTime;
extern bool isAlarmRinging;
extern DWORD alarmStartTime;
extern int screenSaverID;
extern DWORD itemDeleteStartTime;
extern bool isEndScene;
extern DWORD creditStartTime;
extern int creditID;
extern DWORD endSceneStartTime;
extern DWORD iconsMoveStartTime;

float iconsXDelta = 0.0f;

bool areCreditsShown = false;

const int numIconsX = 5;
const int numIconsY = 4;
const float iconDistance = 2.0f / 5.0f;
const float iconBorderWidth = 0.08f;
bool iconBlackout[numIconsY][numIconsX] =
{
	{false, false, false, false},
	{false, false, false, false},
	{false, false, false, false},
	{false, false, false, false},
};

DWORD cursorLastActiveTime;

DWORD arrowStartTime = 0;
float arrowX = 0;
float arrowY = 0;
extern bool isMusicPlaying;
DWORD musicPlayStartTime = 0;

float mouseXPos = 0.0f;
float mouseYPos = 0.0f;

const float mouseCursorWidth = 0.063f;
const float mouseCursorHeight = 0.1f;

//const float FOV = 45;
const float FOV = 40;

// Flickering of the screen
extern bool isFlickering;
extern DWORD flickerStartTime;
DWORD nextFlickerTime = 0;
float flickerAmount = 0.0f;
float flickerPosition = 0.5f;

/* Number of names of the used shaders */
#define NUM_SHADERS 10
const GLchar shaderFileName[NUM_SHADERS][128] =
{
	"shaders/verystart.shader",
	"shaders/showTex.shader",
	"shaders/cloudstuff.1.shader",
	"shaders/gras20.shader",
	"shaders/gras10.shader",
	"shaders/gras12.2.shader",
	"shaders/ball8.shader",
	"shaders/Background.shader",
	"shaders/simpleTex.shader",
	"shaders/fragmentOffscreenCopy.shader"
};
#define OTONE_SHADER 7
#define SIMPLE_TEX_SHADER 8
#define ENGAWA_ICON_HIGHLIGHT_SHADER 1
/* Location where the loaded shader is stored */
#define MAX_SHADER_LENGTH 200000
GLchar fragmentMainBackground[MAX_SHADER_LENGTH];

// Constant shader code that is used for all the effects.
const GLchar *vertexMainObject="\
#version 120\n\
varying vec3 objectPosition;\
varying mat4 parameters;\
varying vec4 color;\
\
void main(void)\
{\
   parameters = gl_ModelViewMatrix;\
   gl_TexCoord[0] = gl_MultiTexCoord0;\n\
   color = gl_Color;\n\
   objectPosition = vec3(gl_Vertex.x, gl_Vertex.y, 1.0);\
   gl_Position = gl_ProjectionMatrix * vec4(gl_Vertex.x, gl_Vertex.y, gl_Vertex.z, 1.0) ;\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

TextureManager textureManager;

const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
	 "glDeleteProgram", "glDeleteShader",
	 "glActiveTexture", "glGetUniformLocation", "glUniform1i", "glUniform1f",
	 "glMultiTexCoord2f"
};

// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];
static int noiseTmp[4];

typedef void (*GenFP)(void); // pointer to openGL functions
GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderPrograms[NUM_SHADERS];
//static GLuint shaderCopyProgram;
#define shaderCopyProgram (shaderPrograms[9])

// mouse stuff
bool isSubMenu = false;
int subMenuIndex;
float subMenuAlpha = 0.0f;

// The Icons globally here
#define NUM_ICONS 15
FlowIcon icon[NUM_ICONS];
FlowIcon engawaIcon[2]; // The second one is for the highlight...
FlowIcon deadIcon[3];
const int NUM_BOXES = 20;
const float boxPosition[NUM_BOXES][2] =
{
	{-1.0f, 1.0f}, {-0.6f, 1.0f}, {-0.2f, 1.0f}, {.2f, 1.0f}, {.6f, 1.0f},
	{-1.0f, 0.5f}, {-0.6f, 0.5f}, {-0.2f, 0.5f}, {.2f, 0.5f}, {.6f, 0.5f},
	{-1.0f, 0.f}, {-0.6f, 0.f}, {-0.2f, 0.f}, {.2f, 0.f}, {.6f, 0.f},
	{-1.0f, -0.5f}, {-0.6f, -0.5f}, {-0.2f, -0.5f}, {.2f, -0.5f}, {.6f, -0.5f},
};

char iconFileName[NUM_ICONS][1024] = 
{
	"garbage_icon.tga",
	"fernseher_icon.tga",
	"music_icon.tga",
	"alarm_icon.tga",
	"weather_icon.tga",
	"wanne_icon.tga",
	"lampe_icon.tga",
	"fridge_icon.tga",
	"teppich_icon.tga",
	"komode_icon.tga",
	"garderobe_icon.tga",
	"sofa_icon.tga",
	"tisch_icon.tga",
	"chair_icon.tga",
	"plant_icon.tga",
};
const int iconBoxID[NUM_ICONS] =
{
	15, 6, 7, 8, 9,
	10, 11, 12, 13, 14,
	5, 16, 17, 18, 19
};
const int deadIconBoxID[3] =
{
	3, 2, 1
};

// icon subcategories
const int MAX_X_SUBCATEGORIES = 2;
const int MAX_Y_SUBCATEGORIES = 5;
const float SUB_ICON_DISTANCE = 0.14f;
const float SUB_ICON_BORDER_WIDTH = 0.01f;

struct SubCategory
{
	int width, height; // Number of stuff in there...
	const char *texName[MAX_X_SUBCATEGORIES][MAX_Y_SUBCATEGORIES];
	float xPos, yPos; // Filled in later...
	FlowIcon icons[MAX_X_SUBCATEGORIES][MAX_Y_SUBCATEGORIES]; // Created later...
};
SubCategory subCategories[NUM_ICONS] = 
{
	// schluessel.tga
	// tee.tga

	// M�lleimer
	{0, 0, },
	// Television
	{2, 5, {{"cracker.tga", "yen.tga", "zwiebel.tga", "affe.tga", "gluehbirne.tga"},
	        {"fisch.tga", "Zucker.tga", "breze.tga", "platte.tga", "magic.tga"}}},
	// Music
	{0, 0, },
	// Alarm
	{0, 0, },
	// Weather
	{2, 5, {{"feuerloescher.tga", "schirm.tga", "banane.tga", "thermometer.tga", "herz.tga"},
	        {"marmelade.tga", "kuerbis.tga", "pinsel.tga", "salz.tga", "schloss.tga"}}},
	// Dusche
	{2, 5, {{"bett.tga", "birne.tga", "sanitaetskasten.tga", "stift.tga", "euro.tga"},
	        {"schluessel.tga", "tee.tga", "hut.tga","kohl.tga", "spritze.tga"}}},
	// Lampe
	{1, 5, {{"Tischdecke01.tga", "trauben.tga", "croissant.tga", "ananas.tga", "erdbeere.tga"}}},
	// Freezer
	{2, 5, {{"bein.tga", "eis.tga", "bier.tga", "eingelegtes.tga", "Kaese.tga"},
	        {"Milch.tga", "pfeffer.tga", "pilz.tga", "Kavier.tga", "zitrone.tga"}}},
	// Teppich
	{1, 3, {{"werkzeug.tga", "karotte.tga", "Waesche.tga"}}},
	// Komode
	{2, 5, {{"apfel.tga", "tischdecke02.tga", "radiergummi.tga", "ordner.tga", "medizin.tga"},
	        {"Streichholz.tga", "schere.tga", "waescheleine.tga", "vase.tga", "Hausschuhe.tga"}}},
	// Garderobe
	{2, 5, {{"besteck.tga", "topf.tga", "Zeitung.tga", "brokkoli.tga", "weinglas.tga"},
	        {"aubergine.tga", "kanne.tga", "tischdecke03.tga", "hamburger.tga", "tasse.tga"}}},
	// Sofa
	{1, 5, {{"kaffee.tga", "gift.tga", "buch.tga", "kinderwagen.tga", "eingemachtes.tga"}}},
	// Tisch
	{0, 0}, // Done directly!!!
	// Chair
	{0, 0}, // Done directly!!!
	// Flower:
	{1, 5, {{"sushi.tga", "bonbon.tga", "Eimer.tga", "cake.tga", "eingelegtes.tga"}}}
};

// -------------------------------------------------------------------
//        ALL the stuff that I need to represent scenes
// -------------------------------------------------------------------

const int NUM_POWER_LINES = 14;
const int NUM_POWER_SCENES = 1000;
const float POWER_LINE_WIDTH = 0.005f;
const float MAST_LINE_WIDTH = 0.07f;
const float MAST_DISTANCE = 10.0f;
const int NUM_CORE_POWER_SCENES = 5;

const float coreMastLine[2][3] =
{
	{-1.0f, 2.52f, 0.0f}, {-1.0f, 0.5f, 0.0f}
};
const float MAST_STUFF_TOP = 0.15f;
const float MAST_STUFF_HEIGHT = 0.3f;
const float MAST_STUFF_WIDTH = 0.5f;

const float corePowerLines[NUM_CORE_POWER_SCENES][NUM_POWER_LINES][3] =
{
	{ // STANDARD
		// Top two lines
		{-1.05f, 2.50f, -0.0f},
		{-0.95f, 2.50f, -0.0f},
		// Left top
		{-1.25f, 2.25f, -0.0f},
		{-1.3f, 2.15f, -0.0f},
		{-1.35f, 2.25f, -0.0f},
		// Left bottom
		{-1.35f, 2.1f, -0.0f},
		{-1.3f, 2.0f, -0.0f},
		{-1.35f, 2.1f, -0.0f},
		// Right top
		{-0.75f, 2.25f, -0.0f},
		{-0.7f, 2.15f, -0.0f},
		{-0.65f, 2.25f, -0.0f},
		// Right bottom
		{-0.75f, 2.1f, -0.0f},
		{-0.7f, 2.0f, -0.0f},
		{-0.65f, 2.1f, -0.0f},
	},
	{ // LEFT_TOP_LEFT
		// Top two lines
		{-1.05f, 2.50f, -0.0f},
		{-0.95f, 2.50f, -0.0f},
		// Left top
		{-10.25f, 2.25f, -0.0f},
		{-10.3f, 2.15f, -0.0f},
		{-1.35f, 2.25f, -0.0f},
		// Left bottom
		{-1.35f, 2.1f, -0.0f},
		{-1.3f, 2.0f, -0.0f},
		{-1.35f, 2.1f, -0.0f},
		// Right top
		{-0.75f, 2.25f, -0.0f},
		{-0.7f, 2.15f, -0.0f},
		{-0.65f, 2.25f, -0.0f},
		// Right bottom
		{-0.75f, 2.1f, -0.0f},
		{-0.7f, 2.0f, -0.0f},
		{-0.65f, 2.1f, -0.0f},
	},
	{ // RIGHT TOP RIGHT
		// Top two lines
		{-1.05f, 2.50f, -0.0f},
		{-0.95f, 2.50f, -0.0f},
		// Left top
		{-10.25f, 2.25f, -0.0f},
		{-10.3f, 2.15f, -0.0f},
		{-10.35f, 2.25f, -0.0f},
		// Left bottom
		{-1.35f, 2.1f, -0.0f},
		{-1.3f, 2.0f, -0.0f},
		{-1.35f, 2.1f, -0.0f},
		// Right top
		{10.75f, 2.25f, -0.0f},
		{10.7f, 2.15f, -0.0f},
		{10.65f, 2.25f, -0.0f},
		// Right bottom
		{-0.75f, 2.1f, -0.0f},
		{-0.7f, 2.0f, -0.0f},
		{-0.65f, 2.1f, -0.0f},
	},
	{ // LEFT_BOTTOM_DOWN
		// Top two lines
		{-1.05f, 2.50f, -0.0f},
		{-0.95f, 2.50f, -0.0f},
		// Left top
		{-1.25f, 2.25f, -0.0f},
		{-1.3f, 2.15f, -0.0f},
		{-1.35f, 2.25f, -0.0f},
		// Left bottom
		{-1.35f, 0.1f, 3.0f},
		{-1.3f, 0.0f, 3.0f},
		{-1.35f, 2.1f, -0.0f},
		// Right top
		{-0.75f, 2.25f, -0.0f},
		{-0.7f, 2.15f, -0.0f},
		{-0.65f, 2.25f, -0.0f},
		// Right bottom
		{-0.75f, 2.1f, -0.0f},
		{-0.7f, 2.0f, -0.0f},
		{-0.65f, 2.1f, -0.0f},
	},
	{ // RIGHT_BOTTOM_DOWN
		// Top two lines
		{-1.05f, 2.50f, -0.0f},
		{-0.95f, 2.50f, -0.0f},
		// Left top
		{-1.25f, 2.25f, -0.0f},
		{-1.3f, 2.15f, -0.0f},
		{-1.35f, 2.25f, -0.0f},
		// Left bottom
		{-1.35f, 2.1f, -0.0f},
		{-1.3f, 2.0f, -0.0f},
		{-1.35f, 2.1f, -0.0f},
		// Right top
		{-0.75f, 2.25f, -0.0f},
		{-0.7f, 2.15f, -0.0f},
		{-0.65f, 2.25f, -0.0f},
		// Right bottom
		{-0.75f, 0.1f, 3.0f},
		{-0.7f, 0.0f, 3.0f},
		{-0.65f, 2.1f, -0.0f},
	},
};

const int NUM_MAST_STUFF_TEXTURES = 5;
const char *mastStuffTextures[NUM_MAST_STUFF_TEXTURES] =
{
	"brown_combined_1.tga",
	"brown_combined_2.tga",
	"brown_combined_3.tga",
	"brown_combined_4.tga",
	"brown_combined_5.tga"
};

float powerLines[NUM_POWER_SCENES][NUM_POWER_LINES][3];
//float mastLines[NUM_POWER_SCENES][2][3];
float mastDisplacement[NUM_POWER_SCENES][3];
float mastSide[NUM_POWER_SCENES];
const char *mastTextures[NUM_POWER_SCENES];
int mastStuffID[NUM_POWER_SCENES][4];

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

unsigned int seed = 0;
float frand()
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	//return (seed >> 8) % 65535;
	return (float)((seed>>8)%65535) * (1.0f/65536.0f);
}

void compileShaders(void)
{
	const GLchar *shaderPointer = fragmentMainBackground;
	//const char *shaderFileName = "shaders/ball8.shader";

	// delete objects
	if (shaderPrograms[0])
	{
		for (int i = 0; i < NUM_SHADERS; i++)
		{
			glDeleteProgram(shaderPrograms[i]);
			shaderPrograms[i] = 0;
		}
		glDeleteProgram(shaderCopyProgram);
		
		shaderCopyProgram = 0;
	}

	/* Generate general programs */
	int tmp, tmp2;
	char err[4097];
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);	
	glShaderSource(vMainObject, 1, &vertexMainObject, NULL);
	glCompileShader(vMainObject);
	// Check programs
	glGetShaderiv(vMainObject, GL_COMPILE_STATUS, &tmp);
	if (!tmp)
	{
		glGetShaderInfoLog(vMainObject, 4096, &tmp2, err);
		err[tmp2]=0;
		MessageBox(hWnd, err, "vMainObject shader error", MB_OK);
		return;
	}

	/* Generate shader specific programs */
	for (int shaderIdx = 0; shaderIdx < NUM_SHADERS; shaderIdx++)
	{
		// load shader from file
		FILE *fid = fopen(shaderFileName[shaderIdx], "rb");
		int numRead = fread(fragmentMainBackground, 1, MAX_SHADER_LENGTH-1,fid);
		fragmentMainBackground[numRead] = 0;
		fclose(fid);

		// init objects:
		GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);
		shaderPrograms[shaderIdx] = glCreateProgram();
	
		// compile sources:
		glShaderSource(fMainBackground, 1, &shaderPointer, NULL);
		glCompileShader(fMainBackground);
		
		// Check programs
		glGetShaderiv(fMainBackground, GL_COMPILE_STATUS, &tmp);
		if (!tmp)
		{
			glGetShaderInfoLog(fMainBackground, 4096, &tmp2, err);
			err[tmp2]=0;
			MessageBox(hWnd, err, shaderFileName[shaderIdx], MB_OK);
			return;
		}

		// link shaders:
		glAttachShader(shaderPrograms[shaderIdx], vMainObject);
		glAttachShader(shaderPrograms[shaderIdx], fMainBackground);
		glLinkProgram(shaderPrograms[shaderIdx]);
		
		glDeleteShader(fMainBackground);
	}

	glDeleteShader(vMainObject);	
}

void intro_init( void )
{
	char errorString[MAX_ERROR_LENGTH + 1];

	// Create the path for the screensaver:
	int lastNumLines = 5;
	float side = 1.0f;
	for (int i = 0; i < NUM_POWER_SCENES; i++)
	{
		int sceneID = (rand()>>8) % NUM_CORE_POWER_SCENES;
		float xDisplace = (frand() - 0.5f) * 0.5f;
		float yDisplace = (frand() - 0.5f) * 0.5f;
		float zDisplace = (frand() - 0.5f) * 0.5f;

		if ((rand() % 10000) > 8500) side = -side;

		for (int j = 0; j < NUM_POWER_LINES; j++)
		{
			powerLines[i][j][0] = side * (corePowerLines[sceneID][j][0] + xDisplace + (frand() - 0.5f) * 0.05f);
			powerLines[i][j][1] = corePowerLines[sceneID][j][1] + yDisplace + (frand() - 0.5f) * 0.05f;
			powerLines[i][j][2] = corePowerLines[sceneID][j][2] + zDisplace + (frand() - 0.5f) * 0.01f;
		}

		mastDisplacement[i][0] = xDisplace;
		mastDisplacement[i][1] = yDisplace;
		mastDisplacement[i][2] = zDisplace;
		mastSide[i] = side;

		if ((rand() % 1000) > 600) mastTextures[i] = "brown_mast_2.tga";
		else mastTextures[i] = "brown_mast_1.tga";

		for (int j = 0; j < 4; j++)
		{
			mastStuffID[i][j] = (rand() % (NUM_MAST_STUFF_TEXTURES + 0)) - 0;
		}
	}

	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// Create and initialize everything needed for texture Management
	if (textureManager.init(errorString))
	{
		MessageBox(hWnd, errorString, "Texture Manager Load", MB_OK);
		return;
	}

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop..
	compileShaders();

	// RLY?
	//glEnable(GL_CULL_FACE);

	// create the engawa icon
	float xpos = boxPosition[6][0]+0.01f;
	float ypos = boxPosition[6][1]-0.05f;
	engawaIcon[0].init("engawa_icon.tga", xpos, ypos, iconDistance, 0.02f);
	engawaIcon[1].init("engawa_icon_overlay.tga", xpos, ypos, iconDistance, 0.02f);

	// Create the desktop icons
	int index = 0;
	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 5; x++)
		{
			//icon[index].init(iconFileName[index], x * iconDistance - 1.0f,
			//			     1.0f - (y+1) * iconDistance * ASPECT_RATIO, iconDistance, iconBorderWidth);
			int boxID = iconBoxID[index];
			float xpos = boxPosition[boxID][0];
			float ypos = boxPosition[boxID][1];
			icon[index].init(iconFileName[index], xpos, ypos, iconDistance, iconBorderWidth);
			
			// Create the subcategories:
			xpos += 0.6f*iconDistance;
			ypos -= 0.2f*iconDistance*ASPECT_RATIO;
			if (index == 4) xpos -= 0.7f * iconDistance;
			if (index == 9) xpos -= 0.7f*iconDistance;
			if (index > 4 && index < 10 && index != 8) ypos += 0.15f * iconDistance * ASPECT_RATIO;
			if (index > 10) ypos += 1.2f*iconDistance*ASPECT_RATIO;
			if (index == 14) xpos -= 0.1f * iconDistance;
			subCategories[index].xPos = xpos;
			subCategories[index].yPos = ypos;

			for (int sx = 0; sx < subCategories[index].width; sx++)
			{
				for (int sy = 0; sy < subCategories[index].height; sy++)
				{
					subCategories[index].icons[sx][sy].init(subCategories[index].texName[sx][sy],
						xpos + sx * SUB_ICON_DISTANCE,
						ypos - sy * SUB_ICON_DISTANCE * ASPECT_RATIO,
						SUB_ICON_DISTANCE, SUB_ICON_BORDER_WIDTH);
				}
			}

			index++;
		}
	}
	deadIcon[0].init("man.tga", boxPosition[deadIconBoxID[0]][0], boxPosition[deadIconBoxID[0]][1], iconDistance, iconBorderWidth);
	deadIcon[1].init("woman.tga", boxPosition[deadIconBoxID[1]][0], boxPosition[deadIconBoxID[1]][1], iconDistance, iconBorderWidth);
	deadIcon[2].init("baby.tga", boxPosition[deadIconBoxID[2]][0], boxPosition[deadIconBoxID[2]][1], iconDistance, iconBorderWidth);

	// Set the texture locations in the relevant shaders:
	// Set texture locations
	glUseProgram(shaderPrograms[OTONE_SHADER]);
	int my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[OTONE_SHADER], "Texture0");
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(my_sampler_uniform_location, 1);
	my_sampler_uniform_location = glGetUniformLocation(shaderPrograms[OTONE_SHADER], "Texture1");
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(my_sampler_uniform_location, 0);
	glActiveTexture(GL_TEXTURE0);
}


void desktopScene(float ftime, DWORD itime)
{
	static float lastTime = 0.0f;
	float deltaTime = ftime - lastTime;
	lastTime = ftime;
	GLuint texID;

	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

#if 0
	//if (itime - musicPlayStartTime > 295000)
	if (itime - musicPlayStartTime > 295000)
	{
		isMusicPlaying = false;
	}
#endif

	char errorString[MAX_ERROR_LENGTH+1];

	glDisable(GL_BLEND);

	if (isSubMenu)
	{
		subMenuAlpha += deltaTime * 5.0f * (1.1f - subMenuAlpha);
		if (subMenuAlpha > 1.0f) subMenuAlpha = 1.0f;
	}
	else
	{
		float expTime = exp(-deltaTime*3.0f);
		subMenuAlpha *= expTime;
	}

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	const char *bgTex;

	switch(screenSaverID)
	{
	case 0:
		bgTex = "yellow_watercolor.tga";
		break;
	case 1:
		bgTex = "blue_watercolor.tga";
		break;
	case 2:
		bgTex = "green_watercolor.tga";
		break;
	case 3:
		bgTex = "black_watercolor.tga";
		break;
	case 4:
	default:
		bgTex = "red_watercolor.tga";
		break;
	}

	if (textureManager.getTextureID(bgTex, &texID, errorString))
	{
		MessageBox(hWnd, errorString, "texture not found", MB_OK);
		exit(1);
	}
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texID);
	textureManager.drawQuad(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f);

	if (textureManager.getTextureID("windows_panel.tga", &texID, errorString))
	{
		MessageBox(hWnd, errorString, "texture not found", MB_OK);
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);
	textureManager.drawQuad(-1.0f, -1.0f, 1.0f, -1.0f + 2.0f * 47.0f / 1600.0f * ASPECT_RATIO, 1.0f);

	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

	// Draw the icons
	if (screenSaverID != 2)
	{
		iconsXDelta = 0.0f;
	}
	else
	{
		if (itime > iconsMoveStartTime)
		{
			iconsXDelta = 2.0f - 2.0f * sqrtf(sqrtf(0.0025f * (float)(itime - iconsMoveStartTime)));
			if (iconsXDelta < 0.0f) iconsXDelta = 0.0f;
		}
		else
		{
			iconsXDelta = 2.0f;
		}
	}

	int numberOfTheDead = 1;
	if (screenSaverID > 2) numberOfTheDead = 2;
	if (screenSaverID > 3) numberOfTheDead = 3;
	for (int i = 0; i < numberOfTheDead; i++)
	{
		deadIcon[i].draw(ftime, iconsXDelta);
	}
	for (int i = 0; i < NUM_ICONS; i++)
	{
		if (i == 3 && isAlarmRinging) // ALARM!
		{
			// drawn later
			//icon[i].drawAlarming(0.001f * (float)(itime - alarmStartTime));
		}
		else
		{
			icon[i].draw(ftime, iconsXDelta);
		}
	}
	if (isAlarmRinging)
	{
		icon[3].drawAlarming(0.001f * (float)(itime - alarmStartTime), iconsXDelta);
	}

	// Draw the menu if present
	if (subMenuAlpha > 0.01f)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		const char *texName = "white.tga";
		if (subCategories[subMenuIndex].height == 1 && subCategories[subMenuIndex].width == 1) texName = "white_1x1.tga";
		if (subCategories[subMenuIndex].height == 2 && subCategories[subMenuIndex].width == 1) texName = "white_2x1.tga";
		if (subCategories[subMenuIndex].height == 3 && subCategories[subMenuIndex].width == 1) texName = "white_3x1.tga";
		if (subCategories[subMenuIndex].height == 4 && subCategories[subMenuIndex].width == 1) texName = "white_4x1.tga";
		if (subCategories[subMenuIndex].height == 5 && subCategories[subMenuIndex].width == 1) texName = "white_5x1.tga";
		if (subCategories[subMenuIndex].height == 5 && subCategories[subMenuIndex].width == 2) texName = "white_5x2.tga";

		if (textureManager.getTextureID(texName, &texID, errorString))
		{
			MessageBox(hWnd, errorString, "texture not found", MB_OK);
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		float xp = subCategories[subMenuIndex].xPos;
		float yp = subCategories[subMenuIndex].yPos;
		float height = (float)subCategories[subMenuIndex].height * SUB_ICON_DISTANCE * ASPECT_RATIO;
		float width = (float)subCategories[subMenuIndex].width * SUB_ICON_DISTANCE;
		textureManager.drawQuad(xp, yp, xp+width, yp-height, subMenuAlpha);

		// Draw all icons
		for (int y = 0; y < subCategories[subMenuIndex].height; y++)
		{
			for (int x = 0; x < subCategories[subMenuIndex].width; x++)
			{
				// here I'd rather have a subDraw!
				subCategories[subMenuIndex].icons[x][y].drawSubCategory(ftime);
			}
		}
	}

	// Show that music is playing
	if (isMusicPlaying)
	{
		float xp = icon[2].getGLX() + 0.7f * iconDistance;
		float yp = icon[2].getGLY() - 0.3f * ASPECT_RATIO;

		if (textureManager.getTextureID("white_1x1.tga", &texID, errorString))
		{
			MessageBox(hWnd, errorString, "texture not found", MB_OK);
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		textureManager.drawQuad(xp-0.01f, yp + 0.205f*ASPECT_RATIO, xp+0.095f, yp - 0.01f*ASPECT_RATIO, 1.0f);

		glBindTexture(GL_TEXTURE_2D, 0);
		for (int balken = 0; balken < 3; balken++)
		{
			float xpb = xp + balken * 0.03f;
			float height = sin(ftime*0.346f*(balken+1.3f) + balken);
			height += 0.5f * sin(ftime*0.8376f*(balken+2.1f) + balken*0.5f);
			height += 0.25f * sin(ftime*0.6124f*(balken+0.3f) + balken*0.2f);
			height = (height + 4.0f) * 0.025f;
			textureManager.drawQuad(xpb, yp, xpb+0.025f, yp + height*ASPECT_RATIO, 1.0f);
		}
	}

	// Draw an error if present
	if (true)
	{
		float arrowTime = 0.003f * (float)(itime - arrowStartTime);
		// Global:?
		//float arrowX = 0.3f;
		//float arrowY = 0.7f;

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float amount = arrowTime * 2.0f;
		if (amount > 1.0f) amount = 1.0f;
		if (arrowTime > 3.0f) amount = 1.0f - (arrowTime - 2.0f) * (arrowTime - 2.0f) * 0.02f;
		if (amount > 0.0f)
		{
			if (textureManager.getTextureID("select_mask.tga", &texID, errorString))
			{
				MessageBox(hWnd, errorString, "texture not found", MB_OK);
				exit(1);
			}
			glBindTexture(GL_TEXTURE_2D, texID);
			textureManager.drawQuad(arrowX-2.0f, arrowY-2.0f*ASPECT_RATIO,
				arrowX + 2.0f, arrowY + 2.0f*ASPECT_RATIO, 0.7f * amount);
		}
	}
	
	// Draw the cursor
	if (itime - cursorLastActiveTime < 3000)
	{
		if (textureManager.getTextureID("cursor_arrow.tga", &texID, errorString))
		{
			MessageBox(hWnd, errorString, "texture not found", MB_OK);
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		float mxp = 2.0f * mouseXPos - 1.0f;
		float myp = 1.0f - 2.0f * mouseYPos;
		textureManager.drawQuad(mxp, myp - mouseCursorHeight*ASPECT_RATIO, mxp + mouseCursorWidth, myp, 1.0f);
	}

	glDisable(GL_BLEND);
}

void rotateX(float dest[3], const float source[3], float alpha)
{
	float ca = (float)cos(alpha);
	float sa = (float)sin(alpha);
	dest[0] = source[0];
	dest[1] = ca * source[1] - sa * source[2];
	dest[2] = sa * source[1] + ca * source[2];
}

void rotateY(float dest[3], const float source[3], float alpha)
{
	float ca = (float)cos(alpha);
	float sa = (float)sin(alpha);
	dest[0] = ca * source[0] - sa * source[2];
	dest[1] = source[1];
	dest[2] = sa * source[0] + ca * source[2];
}

void crossProduct(float dest[3], const float s1[3], const float s2[3])
{
	dest[0] = s1[1]*s2[2] - s1[2]*s2[1];
	dest[1] = s1[2]*s2[0] - s1[0]*s2[2];
	dest[2] = s1[0]*s2[1] - s1[1]*s2[0];
}

void normalize(float vec[3])
{
	float len = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		len += vec[i] * vec[i];
	}
	float invLen = 1.0f / sqrtf(len);
	for (int i = 0; i < 3; i++)
	{
		vec[i] *= invLen;
	}
}

void project(float dest[3], const float source[3])
{
	float multiplier = 2.7f / source[2]; // TODO: Hardcoded FOV???
	dest[0] = multiplier * source[0];
	dest[1] = multiplier * source[1];
	dest[2] = 1.0f;
}

// assumes that BLEND_mode is on standard.
// Program must be standard
// Ignores ASPECT RATIO... I have to think about it.
void drawLine(float start[3], float end[3],
	          float width, const char *texName,
			  const float color[4],
			  float side = 1.0f)
{
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH+1];
	if (textureManager.getTextureID(texName, &texID, errorString) < 0)
	{
		MessageBox(hWnd, errorString, "Texture load error", MB_OK);
		exit(-1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);

	// I have to set the color differently...
	glColor4f(color[0], color[1], color[2], color[3]);

	float pStart[3];
	float pEnd[3];
	project(pStart, start);
	project(pEnd, end);

	float along[3];
	for (int i = 0; i < 3; i++)
	{
		along[i] = pEnd[i] - pStart[i];
	}
	float screenVector[3] = {0.0f, 0.0f, 1.0f};
	float normalVector[3];
	crossProduct(normalVector, along, screenVector);
	normalize(normalVector);

	glBegin(GL_QUADS);
	glTexCoord2f(0.5f - 0.5f*side, 1.0f);
	glVertex3f(start[0] - normalVector[0]*width, start[1] - normalVector[1]*width, -start[2]);
	glTexCoord2f(0.5f - 0.5f*side, 0.0f);
	glVertex3f(end[0] - normalVector[0]*width, end[1] - normalVector[1]*width, -end[2]);
	glTexCoord2f(0.5f + 0.5f*side, 0.0f);
	glVertex3f(end[0] + normalVector[0]*width, end[1] + normalVector[1]*width, -end[2]);
	glTexCoord2f(0.5f + 0.5f*side, 1.0f);
	glVertex3f(start[0] + normalVector[0]*width, start[1] + normalVector[1]*width, -start[2]);
	glEnd();
}

void drawLineHorizontal(float start[3], float end[3],
	                    float width, const char *texName,
			            const float color[4],
						float side,
						float normalVector[3])
{
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH+1];
	if (textureManager.getTextureID(texName, &texID, errorString) < 0)
	{
		MessageBox(hWnd, errorString, "Texture load error", MB_OK);
		exit(-1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);

	// I have to set the color differently...
	glColor4f(color[0], color[1], color[2], color[3]);

	//float normalVector[3] = {1.0f, 0.0f, 0.0f};

	glBegin(GL_QUADS);
	glTexCoord2f(0.5f - 0.5f*side, 1.0f);
	glVertex3f(start[0] - normalVector[0]*width, start[1] - normalVector[1]*width, -start[2] - normalVector[2]);
	glTexCoord2f(0.5f - 0.5f*side, 0.0f);
	glVertex3f(end[0] - normalVector[0]*width, end[1] - normalVector[1]*width, -end[2] - normalVector[2]);
	glTexCoord2f(0.5f + 0.5f*side, 0.0f);
	glVertex3f(end[0] + normalVector[0]*width, end[1] + normalVector[1]*width, -end[2] + normalVector[2]);
	glTexCoord2f(0.5f + 0.5f*side, 1.0f);
	glVertex3f(start[0] + normalVector[0]*width, start[1] + normalVector[1]*width, -start[2] + normalVector[2]);
	glEnd();
}

// The same as drawline, but uses multiple segments, for perspective correction
void drawMultiLine(float start[3], float end[3],
	               float width, const char *texName,
			       const float color[4], float hangThrough)
{
	const int numSegments = 20;

#if 1
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH+1];
	if (textureManager.getTextureID(texName, &texID, errorString) < 0)
	{
		MessageBox(hWnd, errorString, "Texture load error", MB_OK);
		exit(-1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);
#else
	glBindTexture(GL_TEXTURE_2D, 0);
#endif

	float dPos[3];
	for (int i = 0; i < 3; i++)
	{
		dPos[i] = (end[i] - start[i]) / (float)numSegments;
		end[i] = start[i] + dPos[i];
	}

	float hangers[numSegments+1];
	for (int i = 0; i < numSegments+1; i++)
	{
		// Stupid hanging:
		float hanger = (float)(i - numSegments/2) / (float)(numSegments/2);
		hanger *= hanger;
		hanger = (hanger - 1.0f) * hangThrough;
		hangers[i] = hanger;
	}

	for (int i = 0; i < numSegments; i++)
	{
		float startX = start[0];
		float startY = start[1] + hangers[i];
		float startZ = start[2];
		float endX = end[0];
		float endY = end[1] + hangers[i+1];
		float endZ = end[2];

		float lx = dPos[0];
		float ly = dPos[1];
		float invLen = 1.0f / sqrtf(lx * lx + ly * ly);
		lx *= invLen;
		ly *= invLen;
		float nx = -ly;
		float ny = lx;

		glColor4f(color[0], color[1], color[2], color[3]);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, i/(float)numSegments);
		glVertex3f(startX + nx*width, startY + ny*width, -startZ);
		glTexCoord2f(0.0f, (i+1)/(float)numSegments);
		glVertex3f(endX + nx*width, endY + ny*width, -endZ);
		glTexCoord2f(1.0f, (i+1)/(float)numSegments);
		glVertex3f(endX - nx*width, endY - ny*width, -endZ);
		glTexCoord2f(1.0, i/(float)numSegments);
		glVertex3f(startX - nx*width, startY - ny*width, -startZ);
		glEnd();

		for (int j = 0; j < 3; j++)
		{
			start[j] = end[j];
			end[j] += dPos[j];
		}
	}
}

void screensaverScene(float ftime, DWORD itime)
{
	const float X_ROTATE_ALPHA = 0.9f;
	const float Y_ROTATE_ALPHA = -0.15f;

	if (ftime < 0.01f) ftime = 0.01f;
	if (ftime > 10000.0f) ftime = 0.01f;

	char errorString[MAX_ERROR_LENGTH+1];
	GLuint offscreenTexID;
	GLUquadric* quad = gluNewQuadric();

	// draw background:
	glMatrixMode(GL_MODELVIEW);	

	//parameterMatrix[0] = sqrt(ftime) * 3.0f; // time
	parameterMatrix[0] = ftime; // time
	//parameterMatrix[3] = sqrtf(ftime * 0.1f); // time
	//if (parameterMatrix[3] > 1.0f) parameterMatrix[3] = 1.0f;
	//2:0.23(29) 3:0.57(73) 4:0.54(69) 5:0.00(0) 
	parameterMatrix[2] = 0.2f;//params.getParam(2, 0.2f);
	parameterMatrix[1] = 0.59f;//params.getParam(3, 0.59f);	
	parameterMatrix[3] = 0.84f;//params.getParam(4, 0.84f);
	parameterMatrix[6] = 0.65f;//params.getParam(5, 0.65f);
	// translation
	float rotSpeed = 0.02f;//0.3f;
	float rotAmount = 0.5f;//1.6f;
	float moveSpeed = 0.02f;//0.25f;
	float posX = rotAmount * sin(ftime * rotSpeed);
	float posY = rotAmount * cos(ftime * rotSpeed) - moveSpeed * ftime;
	float deltaX = rotAmount * rotSpeed * cos(ftime * rotSpeed);
	float deltaY = -moveSpeed - rotAmount * rotSpeed * sin(ftime * rotSpeed);
	float len = sqrtf(deltaX*deltaX + deltaY*deltaY);
	if (len > 0.0001f)
	{
		deltaX /= len;
		deltaY /= len;
	}
	else
	{
		deltaX = 1.0f;
		deltaY = 0.0f;
	}
	parameterMatrix[8] = posX;
	parameterMatrix[9] = posY;
	parameterMatrix[12] = deltaX;
	parameterMatrix[13] = deltaY;
	glLoadMatrixf(parameterMatrix);

	// Draw the background image
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
    //desktopScene(ftime);

#if 1
	// Draw the background
	glDisable(GL_BLEND);
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);
	textureManager.getTextureID("wolken2.tga", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	textureManager.drawQuad(-4.0f - 3.0f * cos(ftime*0.007f+screenSaverID*0.5f), -1.0f,
						     4.0f - 3.0f * cos(ftime*0.007f+screenSaverID*0.5f), 1.0f, 1.0f);

	// copy the video to texture for later rendering
	GLuint noiseTexID;
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	textureManager.getTextureID("renderTarget", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture

	// draw offscreen painters effect
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[OTONE_SHADER]);
	// Set texture0 (noise texture)
	textureManager.getTextureID("noise2D", &noiseTexID, errorString);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, noiseTexID);
	// Set texture1 (the original scene)
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glActiveTexture(GL_TEXTURE0);

	// Draw it
	textureManager.drawQuad(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f);
#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);

	// Set perspective correction matrix
	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();
	gluPerspective(FOV, ASPECT_RATIO, 0.1f, 1000.0f);

	// Calculate where we are in the world
	float distance = ftime * 1.25f + 50.0f * screenSaverID;
	int sceneIndex = (int)(distance / MAST_DISTANCE);
	distance -= sceneIndex * MAST_DISTANCE;

	// Draw all the lines in the current scene
	int sceneID[5];
	sceneID[0] = sceneIndex % NUM_POWER_SCENES;
	sceneID[1] = (sceneIndex+1) % NUM_POWER_SCENES;
	sceneID[2] = (sceneIndex+2) % NUM_POWER_SCENES;
	sceneID[3] = (sceneIndex+3) % NUM_POWER_SCENES;
	sceneID[4] = (sceneIndex+4) % NUM_POWER_SCENES;

	// Get the direction after rotation
	float tmpV[3];
	float up[3] = {0.0f, 1.0f, 0.0f};
	rotateY(tmpV, up, Y_ROTATE_ALPHA);
	rotateX(up, tmpV, X_ROTATE_ALPHA);
	const float screenVector[3] = {0.0f, 0.0f, 1.0f};
	float normalVector[3];
	crossProduct(normalVector, up, screenVector);
	normalize(normalVector);

	float transStart[3];
	float transEnd[3];
	// Draw the mast
	const float color[4] = {0.9f, 0.9f, 0.9f, 1.0f};

	// Draw the eagle
	float eagleRotation = ftime * 0.3f;
	transStart[0] = 2.0f - 6.0f * cos(eagleRotation) - screenSaverID * 5.0f;
	transStart[1] = 13.f;
	transStart[2] = 80.0f - 6.0f * sin(eagleRotation);
	transEnd[0] = transStart[0] - 0.75f * sin(eagleRotation);
	transEnd[1] = transStart[1];
	transEnd[2] = transStart[2] + 0.75f * cos(eagleRotation);
	transStart[2] -= distance + sceneIndex * MAST_DISTANCE;
	transEnd[2] -= distance + sceneIndex * MAST_DISTANCE;
	// Move Lines
	rotateY(tmpV, transStart, Y_ROTATE_ALPHA);
	rotateX(transStart, tmpV, X_ROTATE_ALPHA);
	rotateY(tmpV, transEnd, Y_ROTATE_ALPHA);
	rotateX(transEnd, tmpV, X_ROTATE_ALPHA);
	normalVector[0] = 1.5f * cos(eagleRotation);
	normalVector[1] = 0.0f;
	normalVector[2] = 1.5f * sin(eagleRotation);
	rotateY(tmpV, normalVector, Y_ROTATE_ALPHA);
	rotateX(normalVector, tmpV, X_ROTATE_ALPHA);
	// draw
	drawLineHorizontal(transStart, transEnd, 0.5f, "eagle.tga", color, 1.0f, normalVector);

	// Draw the tree
	transStart[0] = -5.2f + 2.0f * screenSaverID;
	transStart[1] = 2.0f;
	transStart[2] = 155.0f;
	transEnd[0] = -5.2f + 2.0f * screenSaverID;
	transEnd[1] = 0.6f;
	transEnd[2] = 155.0f;
	transStart[2] -= distance + sceneIndex * MAST_DISTANCE;
	transEnd[2] -= distance + sceneIndex * MAST_DISTANCE;
	// Move the lines
	rotateY(tmpV, transStart, Y_ROTATE_ALPHA);
	rotateX(transStart, tmpV, X_ROTATE_ALPHA);
	rotateY(tmpV, transEnd, Y_ROTATE_ALPHA);
	rotateX(transEnd, tmpV, X_ROTATE_ALPHA);
	// draw
	drawLine(transStart, transEnd, 0.4f, "tree.tga", color);

	for (int drawings = 3; drawings >= 0; drawings--)
	{
		// Draw the first round of stuff
		for (int j = 0; j < 8; j++)
		{
			// Do not show some lines if we are in a later saver
			if (j % (15 / (screenSaverID+1)) == (15 / (screenSaverID+1) - 1)) continue;

			// Translate (and copy...)
			for (int i = 0; i < 3; i++)
			{
				transStart[i] = powerLines[sceneID[drawings]][j][i];
				transEnd[i] = powerLines[sceneID[drawings+1]][j][i];
			}
			transStart[2] -= distance - (drawings-1) * MAST_DISTANCE;
			transEnd[2] -= distance - (drawings) * MAST_DISTANCE;

			// Move the lines
			rotateY(tmpV, transStart, Y_ROTATE_ALPHA);
			rotateX(transStart, tmpV, X_ROTATE_ALPHA);
			rotateY(tmpV, transEnd, Y_ROTATE_ALPHA);
			rotateX(transEnd, tmpV, X_ROTATE_ALPHA);

			// draw
			const float color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
			drawMultiLine(transStart, transEnd, POWER_LINE_WIDTH, "thin_line_small.tga", color, 0.5f);
		}

		// Mast left stuff
		transStart[0] = mastSide[sceneID[drawings]] * (coreMastLine[0][0] + mastDisplacement[sceneID[drawings]][0]);
		transStart[1] = coreMastLine[0][1] + mastDisplacement[sceneID[drawings]][1] - MAST_STUFF_TOP;
		transStart[2] = coreMastLine[0][2] + mastDisplacement[sceneID[drawings]][2];
		transEnd[0] = transStart[0];
		transEnd[1] = transStart[1] - MAST_STUFF_HEIGHT;
		transEnd[2] = transStart[2];
		transStart[2] -= distance - (drawings-1) * MAST_DISTANCE;
		transEnd[2] -= distance - (drawings-1) * MAST_DISTANCE;
		// Move the lines
		rotateY(tmpV, transStart, Y_ROTATE_ALPHA);
		rotateX(transStart, tmpV, X_ROTATE_ALPHA);
		rotateY(tmpV, transEnd, Y_ROTATE_ALPHA);
		rotateX(transEnd, tmpV, X_ROTATE_ALPHA);
		normalVector[0] = 1.0f;
		normalVector[1] = 0.0f;
		normalVector[2] = 0.0f;
		rotateY(tmpV, normalVector, Y_ROTATE_ALPHA);
		rotateX(normalVector, tmpV, X_ROTATE_ALPHA);
		// draw
		if (mastStuffID[sceneID[drawings]][1] >= 0)
		{
			drawLineHorizontal(transStart, transEnd, MAST_STUFF_WIDTH, mastStuffTextures[mastStuffID[sceneID[drawings]][1]], color, -mastSide[sceneID[drawings]], normalVector);
		}

		// Core mast
		transStart[0] = mastSide[sceneID[drawings]] * (coreMastLine[0][0] + mastDisplacement[sceneID[drawings]][0]);
		transStart[1] = coreMastLine[0][1] + mastDisplacement[sceneID[drawings]][1];
		transStart[2] = coreMastLine[0][2] + mastDisplacement[sceneID[drawings]][2];
		transEnd[0] = mastSide[sceneID[drawings]] * (coreMastLine[1][0] + mastDisplacement[sceneID[drawings]][0]);
		transEnd[1] = coreMastLine[1][1] + mastDisplacement[sceneID[drawings]][1];
		transEnd[2] = coreMastLine[1][2] + mastDisplacement[sceneID[drawings]][2];
		transStart[2] -= distance - (drawings-1) * MAST_DISTANCE;
		transEnd[2] -= distance - (drawings-1) * MAST_DISTANCE;
		// Move the lines
		rotateY(tmpV, transStart, Y_ROTATE_ALPHA);
		rotateX(transStart, tmpV, X_ROTATE_ALPHA);
		rotateY(tmpV, transEnd, Y_ROTATE_ALPHA);
		rotateX(transEnd, tmpV, X_ROTATE_ALPHA);
		// draw
		drawLine(transStart, transEnd, MAST_LINE_WIDTH, mastTextures[sceneID[drawings]], color);

		// Mast right stuff
		transStart[0] = mastSide[sceneID[drawings]] * (coreMastLine[0][0] + mastDisplacement[sceneID[drawings]][0]);
		transStart[1] = coreMastLine[0][1] + mastDisplacement[sceneID[drawings]][1] - MAST_STUFF_TOP;
		transStart[2] = coreMastLine[0][2] + mastDisplacement[sceneID[drawings]][2];
		transEnd[0] = transStart[0];
		transEnd[1] = transStart[1] - MAST_STUFF_HEIGHT;
		transEnd[2] = transStart[2];
		transStart[2] -= distance - (drawings-1) * MAST_DISTANCE;
		transEnd[2] -= distance - (drawings-1) * MAST_DISTANCE;
		// Move the lines
		rotateY(tmpV, transStart, Y_ROTATE_ALPHA);
		rotateX(transStart, tmpV, X_ROTATE_ALPHA);
		rotateY(tmpV, transEnd, Y_ROTATE_ALPHA);
		rotateX(transEnd, tmpV, X_ROTATE_ALPHA);
		normalVector[0] = 1.0f;
		normalVector[1] = 0.0f;
		normalVector[2] = 0.0f;
		rotateY(tmpV, normalVector, Y_ROTATE_ALPHA);
		rotateX(normalVector, tmpV, X_ROTATE_ALPHA);
		// draw
		if (mastStuffID[sceneID[drawings]][3] >= 0)
		{
			drawLineHorizontal(transStart, transEnd, MAST_STUFF_WIDTH, mastStuffTextures[mastStuffID[sceneID[drawings]][3]], color, mastSide[sceneID[drawings]], normalVector);
		}

		// Draw the second round of stuff
		for (int j = 8; j < NUM_POWER_LINES; j++)
		{
			// Do not show some lines if we are in a later saver
			if (j % (15 / (screenSaverID+1)) == (15 / (screenSaverID+1) - 1)) continue;

			// Translate (and copy...)
			for (int i = 0; i < 3; i++)
			{
				transStart[i] = powerLines[sceneID[drawings]][j][i];
				transEnd[i] = powerLines[sceneID[drawings+1]][j][i];
			}
			transStart[2] -= distance - (drawings-1) * MAST_DISTANCE;
			transEnd[2] -= distance - (drawings) * MAST_DISTANCE;

			// Move the lines
			rotateY(tmpV, transStart, Y_ROTATE_ALPHA);
			rotateX(transStart, tmpV, X_ROTATE_ALPHA);
			rotateY(tmpV, transEnd, Y_ROTATE_ALPHA);
			rotateX(transEnd, tmpV, X_ROTATE_ALPHA);

			// draw
			const float color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
			drawMultiLine(transStart, transEnd, POWER_LINE_WIDTH, "thin_line_small.tga", color, 0.3f);
		}
	}

	// Delete the icons in the final scene
	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);
	glEnable(GL_BLEND);
	if (screenSaverID == 5)
	{
		for (int i = 0; i < 3; i++)
		{
			deadIcon[i].drawAmount(0.0f, 1.0f, ftime);
		}

		for (int i = NUM_ICONS - 1; i >= 0; i--)
		{
			float toDelete[2];
			toDelete[0] = icon[0].getGLX() - icon[i].getGLX();
			toDelete[1] = icon[0].getGLY() - icon[i].getGLY();

			//float moveTime = (ftime - ((i * 3487639) % 29) - 10.0f) * 0.3f;
			float moveTime = 0.001f * (float)((int)itime - (int)itemDeleteStartTime - ((i * 3488769) % 13000)) * 0.3f;
			if (moveTime < 0.0f) moveTime = 0.0f;
			if (moveTime > 1.0f) moveTime = 1.0f;
			float moveAmount = 0.5f - cos(moveTime * 3.141592f) * 0.5f;

			float detour[2] = {0.0f, 0.0f};
			if (i != 0)
			{
				detour[0] = 0.3f * (sin((float)i + 0.3f) - 0.7f);
				detour[1] = -0.3f * (sin((float)i*3 + 1.5f) - 1.5f);
			}
			float detourAmount = moveTime * sin(moveTime * 3.141592f);

			float munchAmount = 0.0f;
			if (i == 0)
			{
				float munchTime = ((int)itime - (int)itemDeleteStartTime - 4500) * 0.001f * 0.08f;
				if (munchTime < 0.0f) munchTime = 0.0f;
				if (munchTime > 1.0f) munchTime = 1.0f;
				munchAmount = sqrtf(sin(munchTime * 3.141592f)) * 4.0f;
			}

			if (i == 0 || moveTime < 1.0f)
			{
				icon[i].drawAmount(munchAmount, 1.0f, ftime,
					               toDelete[0]*moveAmount + detour[0]*detourAmount,
								   toDelete[1]*moveAmount + detour[1]*detourAmount);
			}
		}
	}

	// Blackout of complete screen
	glBindTexture(GL_TEXTURE_2D, 0);
	float blackAmount = -1.0f;

	if (screenSaverID == 0)
	{
		blackAmount = 1.0f - ftime * 0.2f;	
		blackAmount *= blackAmount * blackAmount;
	}
	if (screenSaverID == 5)
	{
		//blackAmount = (ftime - 72.f) * 0.2f;
		blackAmount = (ftime - 39.f) * 0.2f;
		if (blackAmount > 1.0f) blackAmount = 1.0f;
	}
	if (blackAmount > 0.0f)
	{
		textureManager.drawQuad(-1.0f, -1.0f, 1.0f, 1.0f, blackAmount);
	}
}

void endScene(float ftime, DWORD itime)
{
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH+1];

	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT);
	float iconTime = 0.001f * (itime - demoStartTime);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	
	parameterMatrix[0] = ftime; // time
	glLoadMatrixf(parameterMatrix);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

	// Super-credit
#if 0
	if (creditID > 0 && creditID < 5)
	{
		const char *texName = "masako.tga";
		switch(creditID)
		{
		case 1:
			texName = "pascale.tga";
			break;
		case 2:
			texName = "maiko.tga";
			break;
		case 3:
			texName = "anja.tga";
			break;
		case 4:
		default:
			texName = "masako.tga";
			break;
		}
		glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);

		if (textureManager.getTextureID(texName, &texID, errorString))
		{
			MessageBox(hWnd, errorString, "Loading texture failed", MB_OK);
			exit(-1);
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		float creditTime = 0.25f * 0.001f * (float)(itime - creditStartTime);
		if (creditTime < 1.0f)
		{
			float amount = sin(creditTime*3.141592f);
			amount *= amount;
			float left = -0.5f - 0.1f * cos(creditTime * 3.141592f);
			float right = left + 1.0f;
			float top = 0.98f - 0.02f * cos(creditTime * 3.141592f);
			float bottom = top - 0.5f * ASPECT_RATIO;
			textureManager.drawQuad(left, bottom, right, top, amount);
		}
	}
#endif

	// normal rendering shader
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);
	engawaIcon[0].draw(iconTime);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);	
	// First highlight
	glUseProgram(shaderPrograms[ENGAWA_ICON_HIGHLIGHT_SHADER]);
	engawaIcon[1].draw(iconTime);

	// Second highlight
	if (textureManager.getTextureID("highlight.tga", &texID, errorString))
	{
		MessageBox(hWnd, errorString, "Loading texture failed", MB_OK);
		exit(-1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);
	float duration = 0.75f;
	float centerX = engawaIcon[0].getGLX() + iconDistance * (0.35f + 0.5f * ftime / duration);
	float centerY = engawaIcon[0].getGLY() + iconDistance * ASPECT_RATIO * (1.0f*ftime / duration - 1.0f);
	float amount = 0.0f;
	if (ftime < duration) amount = sin(ftime / duration * 3.1415f);
	float width = iconDistance * (2.25f + 0.5f*amount);
	textureManager.drawQuad(centerX - width, centerY - width * ASPECT_RATIO,
		                    centerX + width, centerY + width * ASPECT_RATIO,
							amount * 0.75f);

	// draw some sparkles if applicable
	if (textureManager.getTextureID("sparkle.tga", &texID, errorString))
	{
		MessageBox(hWnd, errorString, "Loading texture failed", MB_OK);
		exit(-1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);
	float sparkleTime = ftime - duration * 0.4f;
	for (int i = 0; i < 8; i++)
	{
		float sparkleDuration = 1.3f + 0.4f * sin(i*2.4f+2.3f);
		if (sparkleTime > 0.0f && sparkleTime < sparkleDuration)
		{
			amount = sqrtf(sin(sparkleTime / sparkleDuration * 3.1415f));
			centerX = engawaIcon[0].getGLX() + iconDistance * (0.55f + 0.25f * sin(i*2.1f + 7.3f));
			centerX += (0.6f+0.1f*sin(i*1.4f+8.3f)) * iconDistance / sparkleDuration * sparkleTime -
					   0.1f * sparkleTime*sparkleTime/sparkleDuration/sparkleDuration;
			centerY = engawaIcon[0].getGLY() + iconDistance * ASPECT_RATIO * (0.8f + 0.1f * sin(i*4.6f + 2.9f) - 1.0f);
			centerY += (0.5f+0.1f*sin(i*6.8f+3.0f)) * iconDistance / sparkleDuration * sparkleTime * ASPECT_RATIO -
					   0.2f * sparkleTime*sparkleTime/sparkleDuration/sparkleDuration;
			width = iconDistance * 0.25f;
			textureManager.drawQuad(centerX - width, centerY - width * ASPECT_RATIO,
									centerX + width, centerY + width * ASPECT_RATIO,
									amount);
		}
	}

	// show credits if applicable
	if (ftime < duration) areCreditsShown = false;
	if (areCreditsShown)
	{
		if (textureManager.getTextureID("credits.tga", &texID, errorString))
		{
			MessageBox(hWnd, errorString, "texture not found", MB_OK);
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		float left = engawaIcon[0].getGLX() + 1.0f * iconDistance;
		float width = iconDistance * 1.94f;
		float right = left + width;
		float top = engawaIcon[0].getGLY() + 0.6f * iconDistance * ASPECT_RATIO;
		float bottom = top - width * 245.0f / 130.0f * ASPECT_RATIO;
		textureManager.drawQuad(left, bottom, right, top, 1.0f);
	}

	// Wait until you draw the cursor...
	if (ftime < duration * 2.0f) return;

	if (itime - cursorLastActiveTime < 3000)
	{
		// Draw the cursor
		if (textureManager.getTextureID("cursor_arrow.tga", &texID, errorString))
		{
			MessageBox(hWnd, errorString, "texture not found", MB_OK);
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		float mxp = 2.0f * mouseXPos - 1.0f;
		float myp = 1.0f - 2.0f * mouseYPos;
		textureManager.drawQuad(mxp, myp - mouseCursorHeight*ASPECT_RATIO, mxp + mouseCursorWidth, myp, 1.0f);
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
}

void nothingScene(float ftime)
{
	glClear(GL_COLOR_BUFFER_BIT);
}

void intro_do( DWORD itime )
{
#if 0
	static int oldTimes[5] = {0, 0, 0, 0, 0};
	//static float ftime = -0.15f;
	static float ftime = -0.2f;
	ftime += 0.001f*(float)(itime-oldTimes[0]) / 5.0f;
	for (int i = 0; i < 4; i++)
	{
		oldTimes[i] = oldTimes[i+1];
	}
	oldTimes[4] = itime;
#endif

    // render
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_BLEND);
    glDisable( GL_CULL_FACE );
	//glCullFace( GL_FRONT );
	//glDisable( GL_BLEND );
    //glEnable( GL_LIGHTING );
    //glEnable( GL_LIGHT0 );
    //glEnable( GL_NORMALIZE );
	//glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);	
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

	// clear screan:
	//glClear(GL_COLOR_BUFFER_BIT);

	//glBindTexture(GL_TEXTURE_3D, noiseTexture); // 3D noise?	

	/* Set everything to beginning */
	for (int i = 0; i < 16; i++)
	{
		parameterMatrix[i] = 0.0f;
	}
	
	if (isEndScene)
	{
		float ftime = 0.001f * (float)((int)itime - (int)endSceneStartTime);
		endScene(ftime, itime);
	}
	else
	{
		if (isScreenSaverRunning)
		{
			if (screenSaverID >= 0)
			{
				float ftime = 0.001f * (float)((int)itime - (int)screenSaverStartTime);
				screensaverScene(ftime, itime);
			}
			else
			{
				nothingScene(0.0f);
			}
		}
		else
		{
			float ftime = 0.001f * (float)((int)itime - (int)demoStartTime);
			desktopScene(ftime, itime);
		}
	}

	// Reset the GL settings
	glEnable(GL_BLEND);
	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();
	glUseProgram(shaderPrograms[SIMPLE_TEX_SHADER]);

#if 1
	char errorString[MAX_ERROR_LENGTH+1];
	GLuint offscreenTexID;

	// Blackout of the icons due to removed boxes
	if (textureManager.getTextureID("blackout.tga", &offscreenTexID, errorString))
	{
		MessageBox(hWnd, errorString, "texture not found", MB_OK);
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	for (int y = 0; y < numIconsY; y++)
	{
		for (int x = 0; x < numIconsX; x++)
		{
			if (iconBlackout[y][x])
			{
				float xpos = 2.0f * (float)x / (float)numIconsX - 1.0f;
				float ypos = 1.0f - 2.0f * (float)y / (float)numIconsY;
				textureManager.drawQuad(xpos - 0.02f, ypos + 0.02f,
										xpos + 2.0f / numIconsX + 0.02f, ypos - 2.0f / numIconsY - 0.02f, 1.0f);
			}
		}
	}

	glDisable(GL_BLEND);

	// Bezel:
	glEnable(GL_BLEND);
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	textureManager.getTextureID("bezel.tga", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	textureManager.drawQuad(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f);

	// GAMMA CORRECTION 2:0.46(59) 3:0.46(58) 4:0.46(58) 
	parameterMatrix[15] = params.getParam(2, 0.46f) * 1.25f;
	parameterMatrix[14] = params.getParam(3, 0.46f) * 2.0f - 1.0f;
	parameterMatrix[13] = params.getParam(4, 0.46f) * 2.0f;
	parameterMatrix[7] = params.getParam(5, 0.2f) * 5.0f;
	parameterMatrix[6] = params.getParam(6, 0.5f) * 2.0f - 1.0f;
	parameterMatrix[5] = params.getParam(8, 0.5f) * 2.0f;
	parameterMatrix[3] = params.getParam(9, 0.2f) * 5.0f;
	parameterMatrix[2] = params.getParam(12, 0.5f) * 2.0f - 1.0f;
	parameterMatrix[1] = params.getParam(13, 0.5f) * 2.0f;


	// Flickering
	flickerAmount -= 0.25f;
	if (flickerAmount < 0.0f) flickerAmount = 0.0f;
	parameterMatrix[10] = flickerAmount; // No flicker
	parameterMatrix[11] = flickerPosition;
	if (isFlickering)
	{
		if (false &&
			itime > flickerStartTime + 6000 &&
			itime < flickerStartTime + 8000)
		{
			glDisable(GL_BLEND);
			textureManager.getTextureID("programming.tga", &offscreenTexID, errorString);
			glBindTexture(GL_TEXTURE_2D, offscreenTexID);
			textureManager.drawQuad(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f);
			glEnable(GL_BLEND);
			if (itime < flickerStartTime + 6100 ||
				itime > flickerStartTime + 7700)
			{
				glClear(GL_COLOR_BUFFER_BIT);
			}
		}
		else if (itime > nextFlickerTime)
		{
			//glClear(GL_COLOR_BUFFER_BIT);
			flickerAmount = 1.0f;
			flickerPosition = (float)(rand() % 1000) * 0.001f;
			parameterMatrix[10] = flickerAmount; // does flicker
			parameterMatrix[11] = flickerPosition;
			// Flacki Flacko

			float timeSinceFlickerStart = 0.001f * (float)(itime - flickerStartTime);
			float flickerMeanTime = timeSinceFlickerStart * timeSinceFlickerStart + 3.0f;
			if (flickerMeanTime > 25.0f) flickerMeanTime = 15.0f;
			int randi = rand();
			nextFlickerTime = itime + (DWORD)(((randi*randi) % 1000) * flickerMeanTime);
		}
	}

	// Parametric transwhatever stuff:
	// draw background:
	glMatrixMode(GL_MODELVIEW);	
	parameterMatrix[0] = 0.001f * (itime - screenSaverStartTime); // time
	glLoadMatrixf(parameterMatrix);

	glDisable(GL_BLEND);
	glViewport(0, 0, realXRes, realYRes);
	textureManager.getTextureID("renderTarget", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	

	glClear(GL_COLOR_BUFFER_BIT);
	//14:0.63(80) 15:0.72(91) 16:0.51(65) 17:0.76(96) 
	screenLeft = params.getParam(14, 0.64f)*0.4f;
	screenRight = params.getParam(15, 0.71f)*0.4f + 0.6f;
	screenTop = params.getParam(16, 0.51f) * 0.4f;
	screenBottom = params.getParam(17, 0.74f) * 0.4f + 0.6f;

	float left = screenLeft * 2 - 1;
	float right = screenRight * 2 - 1;
	float top = 1 - screenTop * 2;
	float bottom = 1 - screenBottom * 2;

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glTexCoord2f(0.001f, 0.999f);
	glVertex3f(left, top, 0.5f);
	glTexCoord2f(0.999f, 0.999f);
	glVertex3f(right, top, 0.5f);
	glTexCoord2f(0.999f, 0.001f);
	glVertex3f(right, bottom, 0.5f);
	glTexCoord2f(0.001f, 0.001f);
	glVertex3f(left, bottom, 0.5f);
	glEnd();
#endif
}

void intro_cursor(float xpos, float ypos, DWORD itime)
{
	cursorLastActiveTime = itime;
	// Adjust according to left and right
	//xpos = (xpos - screenLeft) / (screenRight - screenLeft);
	//ypos = (ypos - screenTop) / (screenBottom - screenTop);
	mouseXPos = xpos;
	mouseYPos = ypos;

	// click on engawa button...
	engawaIcon[0].setMousePosition(xpos, ypos);

	if (isSubMenu)
	{
		// Also inside all subcategories:
		for (int x = 0; x < subCategories[subMenuIndex].width; x++)
		{
			for (int y = 0; y < subCategories[subMenuIndex].height; y++)
			{
				subCategories[subMenuIndex].icons[x][y].setMousePosition(xpos, ypos);
			}
		}
	}
	
	for (int i = 0; i < NUM_ICONS; i++)
	{
		if (isSubMenu)
		{
			icon[i].setMousePosition(-100.0f, -100.0f);
		}
		else
		{
			icon[i].setMousePosition(xpos, ypos);
		}
	}
}

void intro_blackout(bool becomesBlack)
{
	int x = (int)(mouseXPos * numIconsX);
	int y = (int)(mouseYPos * numIconsY);

	iconBlackout[y][x] = becomesBlack;
}

void setArrowLocation(int iconIdx, DWORD itime)
{
	arrowStartTime = itime;
	arrowX = icon[iconIdx].getGLX() + iconDistance * 0.5f;
	arrowY = icon[iconIdx].getGLY() - iconDistance * 0.5f * ASPECT_RATIO;
}

void music(DWORD itime)
{
	isMusicPlaying = !isMusicPlaying;
	musicPlayStartTime = itime;
}

void intro_left_click(float xpos, float ypos, DWORD itime, int noSound)
{
	cursorLastActiveTime = itime;
	// Adjust according to left and right
	//xpos = (xpos - screenLeft) / (screenRight - screenLeft);
	//ypos = (ypos - screenTop) / (screenBottom - screenTop);

	if (isScreenSaverRunning) return;

	bool isDoubleClick = false;
#if 0
	if (isMusicPlaying)
	{
		isMusicPlaying = false;
		return; // no click sound on stopping!
	}
#endif

	if (isSubMenu)
	{
		// The submenu is removed, possibly something is shinied:
		isSubMenu = false;
		subMenuAlpha = 0.0f;

		for (int x = 0; x < subCategories[subMenuIndex].width; x++)
		{
			for (int y = 0; y < subCategories[subMenuIndex].height; y++)
			{
				subCategories[subMenuIndex].icons[x][y].setMousePosition(xpos, ypos);
				if (subCategories[subMenuIndex].icons[x][y].clickMouse())
				{
					if (subMenuIndex == 2 && x == 0 && y == 1)
					{
						isMusicPlaying = true;
						//PlaySound("sounds/tsuki_no_sabaku.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
						musicPlayStartTime = itime;
						// Avoid clicking sounds!
						return;
					}
					else
					{
						setArrowLocation(subMenuIndex, itime);
						isDoubleClick = true;
						if (!noSound)
						{
							PlaySound("sounds/doppelclick.wav", NULL, SND_FILENAME | SND_ASYNC);
						}
						return;
					}
				}
			}
		}
	}

	// No sub menu, click
	for (int i = 0; i < NUM_ICONS; i++)
	{
		icon[i].setMousePosition(xpos, ypos);
		if (icon[i].clickMouse())
		{
			if (i == 2)
			{
				isMusicPlaying = true;
				//PlaySound("sounds/tsuki_no_sabaku.wav", NULL, SND_FILENAME | SND_ASYNC);
				musicPlayStartTime = itime;
				// Avoid clicking sounds!
				return;
			}
			else
			{
				// Instead do the thing directly
				setArrowLocation(i, itime);
				isDoubleClick = true;
			}			
		}
	}

	if (!noSound)
	{
		if (isDoubleClick)
		{
			PlaySound("sounds/doppelclick.wav", NULL, SND_FILENAME | SND_ASYNC);
		}
		else
		{
			PlaySound("sounds/click_right.wav", NULL, SND_FILENAME | SND_ASYNC);
		}
	}
}

void intro_right_click(float xpos, float ypos, DWORD itime, int noSound)
{
	cursorLastActiveTime = itime;

	// Adjust according to left and right
	//xpos = (xpos - screenLeft) / (screenRight - screenLeft);
	//ypos = (ypos - screenTop) / (screenBottom - screenTop);

	// click on engawa button...
	engawaIcon[0].setMousePosition(xpos, ypos);
	if (engawaIcon[0].clickMouse())
	{
		areCreditsShown = true;
	}

	if (isScreenSaverRunning) return;

#if 0
	if (isMusicPlaying)
	{
		isMusicPlaying = false;
		return; // no clicking!
	}
#endif

	if (isSubMenu)
	{
		// Rightclick gets out of submenu
		isSubMenu = false;
		subMenuAlpha = 0.0f;
		//PlaySound("sounds/click_right.wav", NULL, SND_FILENAME | SND_ASYNC);
		//return;
	}

	// No sub menu, check...
	for (int i = 0; i < NUM_ICONS; i++)
	{
		icon[i].setMousePosition(xpos, ypos);
		if (icon[i].clickMouse())
		{
			if (subCategories[i].width > 0)
			{
				// There is a submenu, draw it:
				isSubMenu = true;
				subMenuAlpha = 1.0f;
				subMenuIndex = i;
			}
		}
	}

	if (!noSound)
	{
		PlaySound("sounds/click_right.wav", NULL, SND_FILENAME | SND_ASYNC);
	}
}