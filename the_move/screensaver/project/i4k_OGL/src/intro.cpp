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
extern int demoStartTime;
extern bool isScreenSaverRunning;
extern int screenSaverStartTime;
extern bool isAlarmRinging;
extern int alarmStartTime;
extern int screenSaverID;

const int numIconsX = 5;
const int numIconsY = 4;
const float iconDistance = 2.0f / 5.0f;
const float iconBorderWidth = 0.04f;
bool iconBlackout[numIconsY][numIconsX] =
{
	{false, false, false, false},
	{false, false, false, false},
	{false, false, false, false},
	{false, false, false, false},
};

bool isArrow = false;
int arrowStartTime = 0;
float arrowX = 0;
float arrowY = 0;
bool isMusicPlaying = false;
int musicPlayStartTime = 0;

float mouseXPos = 0.0f;
float mouseYPos = 0.0f;

const float FOV = 45;

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
FlowIcon deadIcon[3];
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

// icon subcategories
const int MAX_X_SUBCATEGORIES = 2;
const int MAX_Y_SUBCATEGORIES = 5;
const float SUB_ICON_DISTANCE = 0.125f;
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
	// Mülleimer
	{1, 5, {{"affe.tga", "fisch.tga", "Zucker.tga", "pilz.tga", "gluehbirne.tga"}}},
	// Television
	{1, 3, {{"cracker.tga", "yen.tga", "zwiebel.tga"}}},
	// Music
	{1, 3, {{"herz.tga", "platte.tga", "magic.tga"}}},
	// Alarm
	{1, 3, {{"euro.tga", "spritze.tga", "Waesche.tga"}}},
	// Weather
	{1, 4, {{"feuerloescher.tga", "schirm.tga", "banane.tga", "thermometer.tga"}}},
	// Dusche
	{1, 4, {{"bett.tga", "birne.tga", "sanitaetskasten.tga", "stift.tga"}}},
	// Lampe
	{1, 3, {{"Tischdecke01.tga", "pfeffer.tga", "trauben.tga"}}},
	// Freezer
	{2, 5, {{"bein.tga", "ananas.tga", "bier.tga", "eingelegtes.tga", "Kaese.tga"}, {"Milch.tga", "pfeffer.tga", "pilz.tga", "Kavier.tga", "zitrone.tga"}}},
	// Teppich
	{1, 2, {{"werkzeug.tga", "karotte.tga"}}},
	// Komode
	{2, 5, {{"apfel.tga", "tischdecke02.tga", "radiergummi.tga", "ordner.tga", "medizin.tga"}, {"Streichholz.tga", "schere.tga", "waescheleine.tga", "vase.tga", "Hausschuhe.tga"}}},
	// Garderobe
	{2, 5, {{"besteck.tga", "topf.tga", "Zeitung.tga", "werkzeug.tga", "weinglas.tga"}, {"aubergine.tga", "kanne.tga", "tischdecke03.tga", "hamburger.tga", "tasse.tga"}}},
	// Sofa
	{1, 2, {{"euro.tga", "kinderwagen.tga"}}},
	// Tisch
	{0, 0}, // Done directly!!!
	// Chair
	{0, 0}, // Done directly!!!
	// Flower:
	{1, 3, {{"sushi.tga", "bonbon.tga", "Eimer.tga"}}}
};

// -------------------------------------------------------------------
//        ALL the stuff that I need to represent scenes
// -------------------------------------------------------------------

const int NUM_POWER_LINES = 10;
const int NUM_POWER_SCENES = 1000;
const float POWER_LINE_WIDTH = 0.005f;
const float MAST_LINE_WIDTH = 0.07f;
const float MAST_DISTANCE = 10.0f;
const int NUM_CORE_POWER_SCENES = 7;

float coreMastLine[2][3] =
{
	{-1.0f, 2.52f, 0.0f}, {-1.0f, 0.5f, 0.0f}
};

const float corePowerLines[NUM_CORE_POWER_SCENES][NUM_POWER_LINES][3] =
{
	{
		{-1.10f, 2.50f, -0.0f},
		{-1.00f, 2.50f, -0.0f},
		{-0.90f, 2.50f, -0.0f},
		{-0.98f, 2.35f, -0.0f},
		{-0.98f, 2.25f, -0.0f},
		{-0.98f, 2.15f, -0.0f},
		{-0.90f, 2.20f, -0.0f},
		{-0.80f, 2.15f, -0.0f},
		{-0.72f, 2.12f, -0.0f},
		{-0.98f, 2.00f, -0.0f}
	},
	{
		{-7.10f, 2.50f, 4.0f},
		{-7.00f, 2.50f, 4.0f},
		{-6.90f, 2.50f, 4.0f},
		{-0.98f, 2.35f, -0.0f},
		{-0.98f, 2.25f, -0.0f},
		{-0.98f, 2.15f, -0.0f},
		{-0.90f, 2.20f, -0.0f},
		{-0.80f, 2.15f, -0.0f},
		{-0.72f, 2.12f, -0.0f},
		{-0.98f, 2.00f, -0.0f}
	},
	{
		{6.90f, 2.50f, -3.0f},
		{7.00f, 2.50f, -3.0f},
		{7.10f, 2.50f, -3.0f},
		{-0.98f, 2.35f, -0.0f},
		{-0.98f, 2.25f, -0.0f},
		{-0.98f, 2.15f, -0.0f},
		{-0.90f, 2.20f, -0.0f},
		{-0.80f, 2.15f, -0.0f},
		{-0.72f, 2.12f, -0.0f},
		{-0.98f, 2.00f, -0.0f}
	},
	{
		{-1.10f, 2.50f, -0.0f},
		{-1.00f, 2.50f, -0.0f},
		{-0.90f, 2.50f, -0.0f},
		{-0.98f, 2.35f, -0.0f},
		{-0.98f, 0.25f, -3.0f},
		{-0.98f, 0.00f, -3.0f},
		{-0.90f, 2.20f, -0.0f},
		{-0.80f, 2.15f, -0.0f},
		{-0.72f, 2.12f, -0.0f},
		{-0.98f, 2.00f, -0.0f}
	},
	{
		{-1.10f, 2.50f, -0.0f},
		{-1.00f, 2.50f, -0.0f},
		{-0.90f, 2.50f, -0.0f},
		{-0.98f, 2.35f, -0.0f},
		{-2.98f, 0.25f, -8.0f},
		{-2.98f, 0.00f, -8.0f},
		{-0.90f, 2.20f, -0.0f},
		{-0.80f, 2.15f, -0.0f},
		{-0.72f, 2.12f, -0.0f},
		{-0.98f, 2.00f, -0.0f}
	},
	{
		{-1.10f, 2.50f, -0.0f},
		{-1.00f, 2.50f, -0.0f},
		{-0.90f, 2.50f, -0.0f},
		{-2.28f, 0.0f, -0.0f},
		{-2.18f, 0.0f, -0.0f},
		{-2.08f, 0.0f, -0.0f},
		{-0.90f, 2.20f, -0.0f},
		{-0.80f, 2.15f, -0.0f},
		{-0.72f, 2.12f, -0.0f},
		{-0.98f, 2.00f, -0.0f}
	},
	{
		{-1.10f, 2.50f, -0.0f},
		{-1.00f, 2.50f, -0.0f},
		{-0.90f, 2.50f, -0.0f},
		{-0.98f, 2.35f, -0.0f},
		{-0.98f, 2.25f, -0.0f},
		{-0.98f, 2.15f, -0.0f},
		{9.90f, 2.20f, -0.0f},
		{-0.80f, 2.15f, -0.0f},
		{-0.72f, 2.12f, -0.0f},
		{9.98f, 2.00f, -0.0f}
	},
};

float powerLines[NUM_POWER_SCENES][NUM_POWER_LINES][3];
float mastLines[NUM_POWER_SCENES][2][3];

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

		if ((rand() % 10000) < 1500) side = -side;

		for (int j = 0; j < NUM_POWER_LINES; j++)
		{
			powerLines[i][j][0] = side * (corePowerLines[sceneID][j][0] + xDisplace + (frand() - 0.5f) * 0.05f);
			powerLines[i][j][1] = corePowerLines[sceneID][j][1] + yDisplace + (frand() - 0.5f) * 0.05f;
			powerLines[i][j][2] = corePowerLines[sceneID][j][2] + zDisplace + (frand() - 0.5f) * 0.01f;
		}

		for (int j = 0; j < 2; j++)
		{
			mastLines[i][j][0] = side * (coreMastLine[j][0] + xDisplace);
			mastLines[i][j][1] = coreMastLine[j][1] + yDisplace;
			mastLines[i][j][2] = coreMastLine[j][2] + zDisplace;
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

	// Create the icons
	int index = 0;
	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 5; x++)
		{
			icon[index].init(iconFileName[index], x * iconDistance - 1.0f,
						     1.0f - (y+1) * iconDistance * ASPECT_RATIO, iconDistance, iconBorderWidth);
			
			// Create the subcategories:
			float xpos = (x+0.6f)*iconDistance - 1.0f;
			float ypos = 1.0f - (y+1.2f)*iconDistance*ASPECT_RATIO;
			if (index == 10) ypos += 0.9f*iconDistance*ASPECT_RATIO;
			if (index == 9) xpos -= 0.7f*iconDistance;
			if (index == 14) ypos += 0.25f*iconDistance*ASPECT_RATIO;
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
	deadIcon[0].init("man.tga", 4 * iconDistance - 1.0f, 1.0f, iconDistance, iconBorderWidth);
	deadIcon[1].init("woman.tga", 3 * iconDistance - 1.0f, 1.0f, iconDistance, iconBorderWidth);
	deadIcon[2].init("baby.tga", 2 * iconDistance - 1.0f, 1.0f, iconDistance, iconBorderWidth);

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


void desktopScene(float ftime, int itime)
{
	static float lastTime = 0.0f;
	float deltaTime = ftime - lastTime;
	lastTime = ftime;
	GLuint texID;

	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

	if (itime - musicPlayStartTime > 295000)
	{
		isMusicPlaying = false;
	}

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
	int numberOfTheDead = 1;
	if (screenSaverID > 2) numberOfTheDead = 2;
	if (screenSaverID > 3) numberOfTheDead = 3;
	for (int i = 0; i < numberOfTheDead; i++)
	{
		deadIcon[i].draw(ftime);
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
			icon[i].draw(ftime);
		}
	}
	if (isAlarmRinging)
	{
		icon[3].drawAlarming(0.001f * (float)(itime - alarmStartTime));
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

	// Draw an error if present
	if (isArrow)
	{
		float arrowTime = 0.003f * (float)(itime - arrowStartTime);
		// Global:?
		//float arrowX = 0.3f;
		//float arrowY = 0.7f;

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float amount = arrowTime * 2.0f;
		if (amount > 1.0f) amount = 1.0f;
		if (arrowTime > 3.0f) amount = 1.0f - (arrowTime - 2.0f) * (arrowTime - 2.0f) * 0.02f;
		if (amount < 0.0f)
		{
			amount = 0.0f;
		}

		if (textureManager.getTextureID("select_mask.tga", &texID, errorString))
		{
			MessageBox(hWnd, errorString, "texture not found", MB_OK);
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		textureManager.drawQuad(arrowX-2.0f, arrowY-2.0f*ASPECT_RATIO,
			arrowX + 2.0f, arrowY + 2.0f*ASPECT_RATIO, 0.7f * amount);
	}

	// Show that music is playing
	if (isMusicPlaying)
	{
		float xp = icon[2].getGLX() + 0.175f;
		float yp = icon[2].getGLY() - 0.1f * ASPECT_RATIO;

		if (textureManager.getTextureID("white_1x1.tga", &texID, errorString))
		{
			MessageBox(hWnd, errorString, "texture not found", MB_OK);
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		textureManager.drawQuad(xp-0.01f, yp + 0.165f*ASPECT_RATIO, xp+0.095f, yp - 0.01f*ASPECT_RATIO, 1.0f);

		glBindTexture(GL_TEXTURE_2D, 0);
		for (int balken = 0; balken < 3; balken++)
		{
			float xpb = xp + balken * 0.03f;
			float height = sin(ftime*0.546f*(balken+1.3f) + balken);
			height += 0.5f * sin(ftime*1.7376f*(balken+2.1f) + balken*0.5f);
			height += 0.25f * sin(ftime*1.3124f*(balken+0.3f) + balken*0.2f);
			height = (height + 4.0f) * 0.02f;
			textureManager.drawQuad(xpb, yp, xpb+0.025f, yp + height*ASPECT_RATIO, 1.0f);
		}
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
			  const float color[4])
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
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(start[0] - normalVector[0]*width, start[1] - normalVector[1]*width, -start[2]);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(end[0] - normalVector[0]*width, end[1] - normalVector[1]*width, -end[2]);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(end[0] + normalVector[0]*width, end[1] + normalVector[1]*width, -end[2]);
	glTexCoord2f(1.0, 1.0f);
	glVertex3f(start[0] + normalVector[0]*width, start[1] + normalVector[1]*width, -start[2]);
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

void screensaverScene(float ftime)
{
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
	parameterMatrix[2] = params.getParam(2, 0.2f);
	parameterMatrix[1] = params.getParam(3, 0.59f);	
	parameterMatrix[3] = params.getParam(4, 0.84f);
	parameterMatrix[6] = params.getParam(5, 0.65f);
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
	textureManager.drawQuad(-4.0f - 3.0f * cos(ftime*0.007f), -1.0f, 4.0f - 3.0f * cos(ftime*0.007f), 1.0f, 1.0f);

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
	float distance = ftime * 1.25f;
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
	rotateY(tmpV, up, -0.2f);
	rotateX(up, tmpV, 1.0f);
	const float screenVector[3] = {0.0f, 0.0f, 1.0f};
	float normalVector[3];
	crossProduct(normalVector, up, screenVector);
	normalize(normalVector);

	for (int drawings = 3; drawings >= 0; drawings--)
	{
		float transStart[3];
		float transEnd[3];

		// Draw the first round of stuff
		for (int j = 0; j < 3; j++)
		{
			// Translate (and copy...)
			for (int i = 0; i < 3; i++)
			{
				transStart[i] = powerLines[sceneID[drawings]][j][i];
				transEnd[i] = powerLines[sceneID[drawings+1]][j][i];
			}
			transStart[2] -= distance - (drawings-1) * MAST_DISTANCE;
			transEnd[2] -= distance - (drawings) * MAST_DISTANCE;

			// Move the lines
			rotateY(tmpV, transStart, -0.2f);
			rotateX(transStart, tmpV, 1.0f);
			rotateY(tmpV, transEnd, -0.2f);
			rotateX(transEnd, tmpV, 1.0f);

			// draw
			const float color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
			drawMultiLine(transStart, transEnd, POWER_LINE_WIDTH, "thin_line_small.tga", color, 0.5f);
		}

		// Draw the mast
		for (int i = 0; i < 3; i++)
		{
			transStart[i] = mastLines[sceneID[drawings]][0][i];
			transEnd[i] = mastLines[sceneID[drawings]][1][i];
		}
		transStart[2] -= distance - (drawings-1) * MAST_DISTANCE;
		transEnd[2] -= distance - (drawings-1) * MAST_DISTANCE;

		// Move the lines
		rotateY(tmpV, transStart, -0.2f);
		rotateX(transStart, tmpV, 1.0f);
		rotateY(tmpV, transEnd, -0.2f);
		rotateX(transEnd, tmpV, 1.0f);

		// draw
		const float color[4] = {0.8f, 0.8f, 0.8f, 1.0f};
		drawLine(transStart, transEnd, MAST_LINE_WIDTH, "brown_mast_1.tga", color);

		// Draw the second round of stuff
		for (int j = 3; j < NUM_POWER_LINES; j++)
		{
			// Translate (and copy...)
			for (int i = 0; i < 3; i++)
			{
				transStart[i] = powerLines[sceneID[drawings]][j][i];
				transEnd[i] = powerLines[sceneID[drawings+1]][j][i];
			}
			transStart[2] -= distance - (drawings-1) * MAST_DISTANCE;
			transEnd[2] -= distance - (drawings) * MAST_DISTANCE;

			// Move the lines
			rotateY(tmpV, transStart, -0.2f);
			rotateX(transStart, tmpV, 1.0f);
			rotateY(tmpV, transEnd, -0.2f);
			rotateX(transEnd, tmpV, 1.0f);

			// draw
			const float color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
			drawMultiLine(transStart, transEnd, POWER_LINE_WIDTH, "thin_line_small.tga", color, 0.3f);
		}
	}
}




void nothingScene(float ftime)
{
	char errorString[MAX_ERROR_LENGTH+1];
	GLuint offscreenTexID;
	GLUquadric* quad = gluNewQuadric();

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);

	parameterMatrix[0] = ftime; // time	
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	//glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glUseProgram(shaderPrograms[1]);
	//glBindTexture(GL_TEXTURE_2D, noiseTexture);
	gluSphere(quad, 2.0f, 16, 16);

	// copy to front
	glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
	textureManager.getTextureID("renderTarget", &offscreenTexID, errorString);
	glBindTexture(GL_TEXTURE_2D, offscreenTexID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);   //Copy back buffer to texture
	glUseProgram(shaderCopyProgram);	
	gluSphere(quad, 2.0f, 16, 16);
}

void intro_do( long itime )
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
	
	if (isScreenSaverRunning)
	{
		float ftime = 0.001f * (float)(itime - screenSaverStartTime);
		screensaverScene(ftime);
	}
	else
	{
		float ftime = 0.001f * (float)(itime - demoStartTime);
		desktopScene(ftime, itime);
	}

	// Reset the GL settings
	glEnable(GL_BLEND);
	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();

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
	textureManager.drawQuad(-1.0f, -1.0f, 1.0f, 1.0f, 0.7f);

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
	screenLeft = params.getParam(14, 0.1f);
	screenRight = params.getParam(15, 0.9f);
	screenTop = params.getParam(16, 0.2f);
	screenBottom = params.getParam(17, 1.0f);

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

void intro_cursor(float xpos, float ypos)
{
	// Adjust according to left and right
	xpos = (xpos - screenLeft) / (screenRight - screenLeft);
	ypos = (ypos - screenTop) / (screenBottom - screenTop);
	mouseXPos = xpos;
	mouseYPos = ypos;

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

void intro_left_click(float xpos, float ypos, int itime)
{
	// Adjust according to left and right
	xpos = (xpos - screenLeft) / (screenRight - screenLeft);
	ypos = (ypos - screenTop) / (screenBottom - screenTop);

	if (isScreenSaverRunning) return;

	bool isDoubleClick = false;
	if (isMusicPlaying)
	{
		isMusicPlaying = false;
		return; // no click sound on stopping!
	}

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
						isArrow = true;
						arrowStartTime = itime;
						int iconIdx = rand() % NUM_ICONS;
						arrowX = icon[iconIdx].getGLX() + iconDistance * 0.5f;
						arrowY = icon[iconIdx].getGLY() - iconDistance * 0.5f * ASPECT_RATIO;
						isDoubleClick = true;
						PlaySound("sounds/doppelclick.wav", NULL, SND_FILENAME | SND_ASYNC);
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
				isArrow = true;
				arrowStartTime = itime;
				int iconIdx = rand() % NUM_ICONS;
				arrowX = icon[iconIdx].getGLX() + iconDistance * 0.5f;
				arrowY = icon[iconIdx].getGLY() - iconDistance * 0.5f * ASPECT_RATIO;
				isDoubleClick = true;
			}			
		}
	}

	if (isDoubleClick)
	{
		PlaySound("sounds/doppelclick.wav", NULL, SND_FILENAME | SND_ASYNC);
	}
	else
	{
		PlaySound("sounds/click_right.wav", NULL, SND_FILENAME | SND_ASYNC);
	}
}

void intro_right_click(float xpos, float ypos, int itime)
{
	// Adjust according to left and right
	xpos = (xpos - screenLeft) / (screenRight - screenLeft);
	ypos = (ypos - screenTop) / (screenBottom - screenTop);

	if (isScreenSaverRunning) return;

	if (isMusicPlaying)
	{
		isMusicPlaying = false;
		return; // no clicking!
	}

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

	PlaySound("sounds/click_right.wav", NULL, SND_FILENAME | SND_ASYNC);
}