#include "StdAfx.h"
#include "Snippets.h"
#include "Math.h"

#ifndef PI
#define PI 3.1415926f
#endif

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
		snippet[i].pos[1] = 2.0f * (float)rand() / (float)RAND_MAX + 1.0f;
		snippet[i].pos[2] = 2.0f * (float)rand() / (float)RAND_MAX - 1.0f;

		snippet[i].speed[0] = 0.0f;
		snippet[i].speed[1] = 0.0f;
		snippet[i].speed[2] = 0.0f;

		for (int dim = 0; dim < 3; dim++)
		{
			snippet[i].rpy[dim] = 2.0f * PI * (float)rand() / (float)RAND_MAX;
		}
	}
}

void Snippets::update(float deltaTime)
{
	// Do a little bit of rotation
	for (int i = 0; i < NUM_SNIPPETS; i++)
	{
		for (int dim = 0; dim < 3; dim++)
		{
			snippet[i].rpy[dim] += deltaTime*0.9f;
			while (snippet[i].rpy[dim] > PI*2.0f) snippet[i].rpy[dim] -= PI*2.0f;
		}
	}

	// Move according to speed
	for (int i = 0; i < NUM_SNIPPETS; i++)
	{
		for (int dim = 0; dim < 3; dim++)
		{
			snippet[i].pos[dim] += snippet[i].speed[dim] * deltaTime;
		}
	}

	// Gravitate
	for (int i = 0; i < NUM_SNIPPETS; i++)
	{
		snippet[i].speed[1] -= 1.5f * deltaTime;
	}

	// Remove normal component
	for (int i = 0; i < NUM_SNIPPETS; i++)
	{
		float normal[3] = {0.0f, 0.0f, 1.0f};
		rotate(normal, snippet[i].rpy);
		float scalProd = 0.0f;
		for (int dim = 0; dim < 3; dim++)
		{
			scalProd += normal[dim] * snippet[i].speed[dim];
		}
		
		// Remove it completely
		for (int dim = 0; dim < 3; dim++)
		{
			snippet[i].speed[dim] -= scalProd * normal[dim];
		}
	}

	// Slow down due to air resistance
	for (int i = 0; i < NUM_SNIPPETS; i++)
	{
		for (int dim = 0; dim < 3; dim++)
		{
			snippet[i].speed[dim] *= (float)exp(deltaTime * -8.f);
		}
	}

	// Warp snippet up
	for (int i = 0; i < NUM_SNIPPETS; i++)
	{
		if (snippet[i].pos[1] < -1.2f)
		{
			snippet[i].pos[0] = 2.0f * (float)rand() / (float)RAND_MAX - 1.0f;
			snippet[i].pos[1] = 2.0f * (float)rand() / (float)RAND_MAX + 1.0f;
			snippet[i].pos[2] = 2.0f * (float)rand() / (float)RAND_MAX - 1.0f;
		}
	}
}

void Snippets::draw(void)
{
#if 0
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
#endif

	// Width of a standard tex unit
	const float texW = 0.02f;
	// Width of a standard poly unit
	const float polyW = 0.025f;

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

		float rvec[3] = {polyW, 0.0f, 0.0f};
		float uvec[3] = {0.0f, polyW, 0.0f};

		rotate(rvec, snippet[i].rpy);
		rotate(uvec, snippet[i].rpy);

		glTexCoord2f(texX + texW*(displ[0][0]-1.0f), texY + texW*(displ[0][1]+1.0f));
		//glVertex3f(px + polyW*(displ[0][0]-1.0f), py + polyW*(displ[0][1]+1.0f), 0.99f);
		glVertex3f(px + rvec[0]*(displ[0][0]-1.0f) + uvec[0]*(displ[0][1]+1.0f),
			       py + rvec[1]*(displ[0][0]-1.0f) + uvec[1]*(displ[0][1]+1.0f), 0.99f);
		glTexCoord2f(texX + texW*(displ[1][0]+1.0f), texY + texW*(displ[1][1]+1.0f));
		//glVertex3f(px + polyW*(displ[1][0]+1.0f), py + polyW*(displ[1][1]+1.0f), 0.99f);
		glVertex3f(px + rvec[0]*(displ[1][0]+1.0f) + uvec[0]*(displ[1][1]+1.0f),
			       py + rvec[1]*(displ[1][0]+1.0f) + uvec[1]*(displ[1][1]+1.0f), 0.99f);		
		glTexCoord2f(texX + texW*(displ[2][0]+1.0f), texY + texW*(displ[2][1]-1.0f));
		//glVertex3f(px + polyW*(displ[2][0]+1.0f), py + polyW*(displ[2][1]-1.0f), 0.99f);
		glVertex3f(px + rvec[0]*(displ[2][0]+1.0f) + uvec[0]*(displ[2][1]-1.0f),
			       py + rvec[1]*(displ[2][0]+1.0f) + uvec[1]*(displ[2][1]-1.0f), 0.99f);				
		glTexCoord2f(texX + texW*(displ[3][0]-1.0f), texY + texW*(displ[3][1]-1.0f));
		//glVertex3f(px + polyW*(displ[3][0]-1.0f), py + polyW*(displ[3][1]-1.0f), 0.99f);
		glVertex3f(px + rvec[0]*(displ[3][0]-1.0f) + uvec[0]*(displ[3][1]-1.0f),
			       py + rvec[1]*(displ[3][0]-1.0f) + uvec[1]*(displ[3][1]-1.0f), 0.99f);
	}
	glEnd();
}

void Snippets::rotate(float pos[3], float rpy[3])
{
	float t[3];

	t[0] = pos[0];
	t[1] = cos(rpy[0]) * pos[1] - sin(rpy[0]) * pos[2];
	t[2] = sin(rpy[0]) * pos[1] + cos(rpy[0]) * pos[2];

	pos[0] = cos(rpy[1]) * t[0] - sin(rpy[1]) * t[2];
	pos[1] = t[1];
	pos[2] = sin(rpy[1]) * t[0] + cos(rpy[1]) * t[2];

	t[0] = cos(rpy[2]) * pos[0] - sin(rpy[2]) * pos[1];
	t[1] = sin(rpy[2]) * pos[0] + cos(rpy[2]) * pos[1];
	t[2] = pos[2];

	pos[0] = t[0];
	pos[1] = t[1];
	pos[2] = t[2];
}