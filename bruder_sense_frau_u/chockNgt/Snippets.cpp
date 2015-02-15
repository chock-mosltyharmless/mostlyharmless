#include "StdAfx.h"
#include "Snippets.h"
#include "Math.h"

Snippets::Snippets(void)
{
}

Snippets::~Snippets(void)
{
}

void Snippets::init(void)
{
	// Just randomly put them somewhere (with random rotation and no speed)
	for (int i = 0; i < NUM_SNIPPETS; i++)
	{
		snippet[i].pos[0] = 2.0f * (float)rand() / (float)RAND_MAX - 1.0f;
		snippet[i].pos[1] = 2.0f * (float)rand() / (float)RAND_MAX - 1.0f;
		snippet[i].pos[2] = 2.0f * (float)rand() / (float)RAND_MAX - 1.0f;

		snippet[i].speed[0] = 0.0f;
		snippet[i].speed[1] = 0.0f;
		snippet[i].speed[2] = 0.0f;

		float quatLen = 0.0f;
		for (int dim = 0; dim < 4; dim++)
		{
			snippet[i].rotation[dim] = 2.0f * (float)rand() / (float)RAND_MAX - 1.0f;
			quatLen += snippet[i].rotation[dim];
		}
		quatLen = 1.0f / (quatLen + 0.0001f);
		for (int dim = 0; dim < 4; dim++)
		{
			snippet[i].rotation[dim] *= quatLen;
		}
	}
}

void Snippets::update(float deltaTime)
{
}

void Snippets::draw(void)
{
	// Width of a standard tex unit
	const float texW = 0.03f;
	// Width of a standard poly unit
	const float polyW = 0.04f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	glBegin(GL_QUADS);
	for (int i = 0; i < NUM_SNIPPETS; i++)
	{
		float texX = 0.5f + 0.47f * cos(i * 3784.5f);
		float texY = 0.5f + 0.47f * cos(i * 6392.2f);

		float displ[4][2];
		for (int corner = 0; corner < 4; corner++)
		{
			displ[corner][0] = 0.75f + 1.0f * cos(i * 7602.2f + corner * 2531.3f);
			displ[corner][1] = 0.75f + 1.0f * cos(i * 3452.2f + corner * 8423.7f);
		}

		float px = snippet[i].pos[0];
		float py = snippet[i].pos[1];

		glTexCoord2f(texX + texW*(displ[0][0]-1.0f), texY + texW*(displ[0][1]+1.0f));
		glVertex3f(px + polyW*(displ[0][0]-1.0f), py + polyW*(displ[0][1]+1.0f), 0.99f);
		glTexCoord2f(texX + texW*(displ[1][0]+1.0f), texY + texW*(displ[1][1]+1.0f));
		glVertex3f(px + polyW*(displ[1][0]+1.0f), py + polyW*(displ[1][1]+1.0f), 0.99f);
		glTexCoord2f(texX + texW*(displ[2][0]+1.0f), texY + texW*(displ[2][1]-1.0f));
		glVertex3f(px + polyW*(displ[2][0]+1.0f), py + polyW*(displ[2][1]-1.0f), 0.99f);
		glTexCoord2f(texX + texW*(displ[3][0]-1.0f), texY + texW*(displ[3][1]-1.0f));
		glVertex3f(px + polyW*(displ[3][0]-1.0f), py + polyW*(displ[3][1]-1.0f), 0.99f);
	}
	glEnd();
}