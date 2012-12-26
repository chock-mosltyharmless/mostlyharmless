//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"

#include "config.h"
#include "intro.h"
#include "mzk.h"
#include "font.h"

float frand();
int rand();

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;
const int fontWidth = 128;
const int fontHeight = 64;
const int numValues = 2536;
unsigned char fontCompressed[4][fontHeight][fontWidth];
unsigned char font[fontHeight][fontWidth][4];
static GLuint fontTexture;

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

void intro_init( void )
{
	// Turn off lighting and other things that I do not need.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); 
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);						// Enable Texture Mapping

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

	// create texture for font
	glGenTextures(1, &fontTexture);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// TODO: Mip Mapping!!!!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, fontWidth, fontHeight,
					  GL_BGRA, GL_UNSIGNED_BYTE, font);
	glDisable(GL_TEXTURE_2D);						// Enable Texture Mapping
}

void intro_do( long itime )
{
	//itime += SYNC_ADJUSTER;
	float ftime = 0.001f*(float)itime;

    // render
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
    glDisable( GL_CULL_FACE );

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// fovy, aspect, zNear, zFar
	gluPerspective(25.0f, 1.8f, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// eye, center, up
	gluLookAt(0.0f, 0.2f, 1.0f,
			  0.0f, 0.0f, 0.0f,
			  0.0f, 1.0f, 0.0f);

	// Draw checkerboard floor
	glTranslatef(sin(ftime * 0.531f + 0.9f) + sin(ftime * 1.31f + 1.0f),
				 0.0f,
				 sin(ftime * 0.256f + 0.1f) + sin(ftime * 0.75f + 2.6f));
	glBegin(GL_QUADS);
	for (int z = -30; z < 30; z++)
	{
		for (int x = -30; x < 30; x++)
		{
			if ((x ^ z) & 1)
			{
				glColor4f(23/256.0f, 46/256.0f, 69/256.0f, 1.0f);
			}
			else
			{
				glColor4f(208/256.0f, 231/256.0f, 1.0f, 1.0f);
			}
			glVertex3f((float)x, -2.0f, (float)z);
			glVertex3f((float)x + 1.0f, -2.0f, (float)z);
			glVertex3f((float)x + 1.0f, -2.0f, (float)z + 1.0f);
			glVertex3f((float)x, -2.0f, (float)z + 1.0f);
		}
	}
	glEnd();

	// Draw the cube
	glTranslatef(sin(ftime * 1.7f + 1.1f) + sin(ftime * 2.4f + 2.5f),
				 0.5f * sin(ftime * 1.83f + 1.3f) + 0.5f * sin(ftime * 1.4f + 3.0f) - 1.0f,
				 sin(ftime * 1.34f + 1.1f) + sin(ftime * 2.844f + 1.9f) - 6.0f);
	glRotatef(ftime * 40.0f + sin(ftime * 0.6f) * 350.0f, 
		      sin(ftime * 1.71f + 0.3f), sin(ftime * 1.332f + 1.7f), sin(ftime * 1.579f + 2.1f));
	glScalef(0.5f, 0.5f, 0.5f);
	glBegin(GL_QUADS);
	
	// unten
	glColor4f(231/256.0f, 1.0f, 1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);
	// oben
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);
	// links
	glColor4f(162/256.0f, 185/256.0f, 208/256.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	// rechts
	glVertex3f(1.0f, -1.0f, -1.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);
	// vorne
	glColor4f(69/256.0f, 92/256.0f, 139/256.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);
	// hinten
	glVertex3f(1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glEnd();

	// draw font
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glBegin(GL_QUADS);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	for (int i = 0; i < 28; i++)
	{
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(1.3f + 0.43f*i - 0.6f*ftime, .55f);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(1.3f + 0.43f*i - 0.6f*ftime, .9f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(1.7f + 0.43f*i - 0.6f*ftime, .9f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(1.7f + 0.43f*i - 0.6f*ftime, .55f);
	}
	glEnd();
	glDisable(GL_TEXTURE_2D);						// Enable Texture Mapping
}
