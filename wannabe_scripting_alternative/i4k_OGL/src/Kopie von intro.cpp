//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "intro.h"
#include "mzk.h"

float frand();

// 100 particles (per flow?)
//#define NUM_PARTICLES 2000
//#define NUM_PARTICLES 1024
#define NUM_PARTICLES 128
// The number of sums of sin functions per moving object.
#define NUM_SUMS 3
#define PARTICLE_SPEED 0.3f
//#define PARTICLE_MOVEMENT_SIZE 0.25f // Size of the movement within the snake
#define PARTICLE_MOVEMENT_SIZE 0.3f // Size of the movement within the snake
//#define PARTICLE_SIZE 0.05f // Size of one particle
#define PARTICLE_SIZE 0.1f // Size of one particle
//#define STREAM_SPEED 0.2f
#define STREAM_SPEED 2.5f
#define STREAM_LENGTH (10.0f/NUM_PARTICLES)
#define NUM_STREAMS 8
#define PARTICLE_NORMAL_FACTOR 1.5f
//#define STREAM_SIZE 0.5f
#define STREAM_SIZE 1.0f

// TODO: put everything into one big array in order to decrease init space?
float particleFrequency[NUM_PARTICLES][3][NUM_SUMS];
//float particlePhase[NUM_PARTICLES][3][NUM_SUMS];

//---------------------------------------------------------------------

#define fzn  0.005f
#define fzf  1000.0f

static const float projectionmatrix[16] = {
    1.0f, 0.00f,  0.0f,                    0.0f,
    0.0f, 1.33f,  0.0f,                    0.0f,
	//0.0f, 2.33f,  0.0f,                    0.0f,
    0.0f, 0.00f, -(fzf+fzn)/(fzf-fzn),    -1.0f,
    0.0f, 0.00f, -2.0f*fzf*fzn/(fzf-fzn),  0.0f };

void intro_init( void )
{
	for (int k = 0; k < sizeof(particleFrequency)/sizeof(float); k++)
	{
		particleFrequency[0][0][k] = (frand() + 1.0f) * PARTICLE_SPEED;
		//particlePhase[0][0][k] = frand() * 16.0f;
	}
}

void intro_do( long itime )
{
    const float ftime = 0.001f*(float)itime;

    // animate
    float pos[3] = { 3.0f*cosf( ftime*0.30f ), 
                     3.0f*cosf( ftime*0.26f ), 
                     3.0f*sinf( ftime*0.30f ) };
    float tar[3] = { 0.0f, 0.0f, 0.0f };
	//float fogColor[4] = {0.1f, 0.2f, 0.4f, 0.0f};	
	//float fogColor[4] = {1.0f, 0.875f, 0.75f, 0.0f};
	//float fogColor[4] = {1.0f, 0.8f, 0.5f, 0.0f};
	float fogColor[4] = {1.0f, 0.85f, 0.6f, 0.0f};
	//float specularColor[4] = {1.0f, 0.875f, 0.75f, 0.0f};
	//float specularColor[4] = {0.25f, 0.5f, 1.0f, 0.0f};
	float specularColor[4] = {0.0f, 0.25f, 1.0f, 0.0f};
	float ambientColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // render
	glEnable( GL_DEPTH_TEST );
    //glEnable( GL_CULL_FACE );
	//glDisable( GL_BLEND );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );
    //glEnable( GL_NORMALIZE );
	//glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_DENSITY, 0.375f); 
	glEnable(GL_FOG);

	//glLightfv(GL_LIGHT0, GL_SPECULAR, specularColor);
	//glLightfv(GL_LIGHT0, GL_DIFFUSE, specularColor);
	//glLightfv(GL_LIGHT0, GL_AMBIENT, specularColor);

	//glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, specularColor);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambientColor);	
	//glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 20);
 
    glClearColor(fogColor[0], fogColor[1], fogColor[2], fogColor[3]);	
	glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );

    glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( projectionmatrix );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    //gluLookAt( pos[0], pos[1], pos[2], tar[0], tar[1], tar[2], 0.0f, 1.0f, 0.0f );

	// draw triangle
	//glBegin(GL_TRIANGLES);	
	//glVertex3f(0, 0, 0);
	//glVertex3f(2, 0, 0);
	//glVertex3f(0, 2, 0);

	// Draw particles:
	for (int stream = 0; stream < NUM_STREAMS; stream++)
	{
		glBegin(GL_TRIANGLE_STRIP);

		for (int particle = 0; particle < NUM_PARTICLES; particle++)
		{
			float streamPos[3];
			float ctime = ftime + particle * STREAM_LENGTH;
			for (int dim = 0; dim < 3; dim++)
			{
				streamPos[dim] = 0.0f;
				for	(int run = 0; run < NUM_SUMS; run++)
				{
					streamPos[dim] += STREAM_SIZE *
						sin(particleFrequency[stream][dim][run] * ctime + 64.0f * particleFrequency[stream][dim][run]);
				}
			}

			float particlePos[3];
			float particleNormal[3];
			for	(int dim = 0; dim < 3; dim++)
			{			
				particlePos[dim] = 0.0f;
				particleNormal[dim] = 0.0f;
				for (int run = 0; run < NUM_SUMS; run++)
				{
					particlePos[dim] +=
						sin(particleFrequency[particle][dim][run] * ftime + 64.0f * particleFrequency[particle][dim][run]);
					//particleNormal[dim] +=
					//	sin(particleFrequency[particle][dim][run] * ftime + particlePhase[particle][dim][run]);
				}
				particlePos[dim] *= PARTICLE_MOVEMENT_SIZE;				
			}

			// normal:			
			particleNormal[2] -= particlePos[2];			
			particleNormal[2] *= PARTICLE_NORMAL_FACTOR;

			for	(int dim = 0; dim < 3; dim++)
			{			
				particlePos[dim] += streamPos[dim];
			}

			float behindfronter = 2.0f - ftime * 0.1f;
			if (behindfronter < 0) behindfronter = 0; //nonono
			if (ftime > 60)
			{
				behindfronter = (ftime - 60) * 0.1f;
			}
			behindfronter *= behindfronter;
			float multer = behindfronter * behindfronter + 1;
			//particlePos[2] -= 2.5f + stream * 0.5f * multer + behindfronter;
			particlePos[2] -= 2.5f + stream * 0.4f * multer + behindfronter;
	
			if (stream < 4)
			{
				float spherePos[3] = {0, 0, -2.0};			
				spherePos[0] = 1 * sin(ftime * 0.6f);
				spherePos[1] = 1 * sin(ftime * 0.5f);
				spherePos[2] = -2.5f + sin(ftime * 0.3f) - behindfronter;
				float normalPos[3] = {0, 0, 0};
				float sphereSize = 1.5f;
				float length = 0;
				for (int dim = 0; dim < 3; dim++)
				{
					normalPos[dim] = particlePos[dim] - spherePos[dim];
					length += normalPos[dim] * normalPos[dim];				
				}
				length = sphereSize / sqrt(length + 1e-8f);
				for (int dim = 0; dim < 3; dim++)
				{
					normalPos[dim] *= length;		
				}			
				
				float interpolator = 0;
				for (int dim = 0; dim < 3; dim++)
				{
					float diff = spherePos[dim] + normalPos[dim] - particlePos[dim];
					interpolator += diff * diff;				
				}
				//float timefactor = 0.00001f * ftime * ftime;
				float timefactor = 0.0f;
				interpolator = 1 / (interpolator * timefactor + 1);
	
				if (ftime > 55)
				{
					interpolator = interpolator / ((ftime-55) * 0.05f + 1) / ((ftime-55) * 0.05f + 1);
				}

				for (int dim = 0; dim < 3; dim++)
				{								
					// Here I need linear interpolation... (plus from distance...)								
					particlePos[dim] = interpolator * (spherePos[dim] + normalPos[dim]) + (1-interpolator) * particlePos[dim];				
				}
				particleNormal[2] = interpolator * normalPos[2] * 0.15f + (1-interpolator) * particleNormal[2];
			}
	
			glNormal3fv(particleNormal);
			glVertex3fv(particlePos);		
			/*for (int dim = 0; dim < 3; dim++)
			{
				for (int run = 0; run < 2; run++)
				{
					particlePos[dim] += PARTICLE_SIZE * 
						sin(particleFrequency[particle][dim][run] * ftime + particlePhase[particle][dim][run]);
				}		
			}
			glNormal3fv(particleNormal);
			glVertex3fv(particlePos);
			for (int dim = 0; dim < 3; dim++)
			{
				for (int run = 2; run < NUM_SUMS; run++)
				{
					particlePos[dim] += PARTICLE_SIZE * 
						sin(particleFrequency[particle][dim][run] * ftime + particlePhase[particle][dim][run]);
				}
			}
			glNormal3fv(particleNormal);
			glVertex3fv(particlePos);*/
		}
		glEnd();
	}	
}

