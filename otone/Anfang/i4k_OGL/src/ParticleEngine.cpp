#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"

#include "ParticleEngine.h"
#include "Texture.h"

extern Texture textures;

ParticleEngine::ParticleEngine(void)
{
	nextParticleAdd = 0;
	time = 0.0f;
	//nextEmissionTime = EMISSION_UPDATE_TIME; // last time that particles were emitted
	nextEmissionTime = 47.8f; // last time that particles were emitted
	nextShortEmissionTime = 47.8f;
	sceneIdx = 0;

	// Clear particle list
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		particle[i].age = 1.0e10f;
		particle[i].size = 0.0f;
		particle[i].position[0] = 0.0f;
		particle[i].position[1] = 0.0f;
		particle[i].color[0] = 0.0f;
		particle[i].color[1] = 0.0f;
		particle[i].color[2] = 0.0f;
		particle[i].color[3] = 0.0f;
		particle[i].moveDirection[0] = 0.0f;
		particle[i].moveDirection[1] = 0.0f;
		particle[i].rotation = 0.0f;
		particle[i].rotationChange = 0.0f;
	}	
}


ParticleEngine::~ParticleEngine(void)
{
}

// The center point is the point where new particles are generated.
void ParticleEngine::update(float deltaTime, float centerPoint[2], float shortEmissionTime)
{
	// The buffer of the particles that are actually visible
	static int activeBuffer[NUM_PARTICLES];
	int numActiveParticles = 0;

	// Set the active Particles:
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		if (particle[i].age < PARTICLE_LIFTIME)
		{
			float xDist = particle[i].position[0] - centerPoint[0];
			float yDist = particle[i].position[1] - centerPoint[1];
			float distanceSquared = (xDist * xDist) + (yDist * yDist);
			if (distanceSquared < 4.0f)
			{
				activeBuffer[numActiveParticles++] = i;
			}
		}
	}

	// Set the moving things for all the active particles
#if 0
	for (int i = 1; i < numActiveParticles; i++)
	{
		int a1 = activeBuffer[i];
		for (int j = 0; j < i; j++)
		{
			int a2 = activeBuffer[j];
			float deltaPos[2];
			deltaPos[0] = particle[a1].position[0] - particle[a2].position[0];
			deltaPos[1] = particle[a1].position[1] - particle[a2].position[1];
			float dist = sqrtf(deltaPos[0] * deltaPos[0] + deltaPos[1] * deltaPos[1]);
			float moveAmount = 0.005f * exp(-8.0f * dist);
			if (dist > 1.0e-10f)
			{
				deltaPos[0] = deltaPos[0] / dist;
				deltaPos[1] = deltaPos[1] / dist;
			}
			else
			{
				deltaPos[0] = frand() * 2.0f - 1.0f;
				deltaPos[1] = frand() * 2.0f - 1.0f;
			}
			particle[a1].moveDirection[0] += deltaPos[0] * moveAmount;
			particle[a1].moveDirection[1] += deltaPos[1] * moveAmount;
			particle[a2].moveDirection[0] -= deltaPos[0] * moveAmount;
			particle[a2].moveDirection[1] -= deltaPos[1] * moveAmount;
		}
	}
#endif

	if (time > nextEmissionTime)
	{
		if (time < 103.0f || time > 140.0f) nextEmissionTime += EMISSION_UPDATE_TIME;
		else nextEmissionTime += EMISSION_UPDATE_TIME / 2;
		if (time > 181.0f) nextEmissionTime = 1000.0f;
		sceneIdx++;

		// move other particles away
		for (int i = 0; i < NUM_PARTICLES; i++)
		{
			float deltaPos[2];
			deltaPos[0] = particle[i].position[0] - centerPoint[0];
			deltaPos[1] = particle[i].position[1] - centerPoint[1];
			float dist = sqrtf(deltaPos[0] * deltaPos[0] + deltaPos[1] * deltaPos[1]);
			float moveAmount = 0.5f * exp(-0.5f * dist);
			if (dist > 1.0e-10f)
			{
				deltaPos[0] = deltaPos[0] / dist;
				deltaPos[1] = deltaPos[1] / dist;
			}
			else
			{
				deltaPos[0] = frand() * 2.0f - 1.0f;
				deltaPos[1] = frand() * 2.0f - 1.0f;
			}
			particle[i].moveDirection[0] += deltaPos[0] * moveAmount;
			particle[i].moveDirection[1] += deltaPos[1] * moveAmount;
			particle[i].rotationChange = 0.5f * moveAmount * frand();
		}

		/* emit some particles */
		for (int i = 0; i < NUM_EMITTED_PARTICLES; i++)
		{
			int n = nextParticleAdd;
			particle[n].age = 0.0f;
			particle[n].size = 0.15f * frand() + 0.075f;
			particle[n].position[0] = centerPoint[0] + (frand()-0.5f) * 1.3f;
			particle[n].position[1] = centerPoint[1] + (frand()-0.5f) * 1.3f;
			particle[n].textureID = (rand() % 8);
			int paletteIndex = rand() % NUM_PALETTE_ENTRIES;
			particle[n].color[0] = palette[paletteIndex][0];
			particle[n].color[1] = palette[paletteIndex][1];
			particle[n].color[2] = palette[paletteIndex][2];
			particle[n].color[3] = palette[paletteIndex][3];
			particle[n].moveDirection[0] = 1.0f * (frand() - 0.5f);
			particle[n].moveDirection[1] = 1.0f * (frand() - 0.5f);
			particle[n].rotation = frand() * 3.1415926f * 2.0f;
			particle[n].rotationChange = 2.0f * frand();
			nextParticleAdd++;
			nextParticleAdd %= NUM_PARTICLES;
		}
	}

	if (time > nextShortEmissionTime)
	{
		nextShortEmissionTime += shortEmissionTime;
		if (time > 182.3f) nextShortEmissionTime = 1000.0f;

		// find good position for a particle
		for (int trye = 0; trye < 32; trye++)
		{
			float trypos[2];
			trypos[0] = centerPoint[0] + (frand()-0.5f) * 0.075f * (float)trye;
			trypos[1] = centerPoint[1] + (frand()-0.5f) * 0.075f * (float)trye;
			float closestDist = 1.0e20f;
			for (int enemy = 0; enemy < numActiveParticles; enemy++)
			{
				int n = activeBuffer[enemy];
				float dist1 = trypos[0] - particle[n].position[0];
				float dist2 = trypos[1] - particle[n].position[1];
				float dist = dist1*dist1 + dist2*dist2;
				if (dist < closestDist) closestDist = dist;
			}
			
			/* check if it is good enough */
			if (closestDist > 0.02f)
			{
				/* Add it */
				int n = nextParticleAdd;
				particle[n].age = 0.0f;
				particle[n].size = 0.1f * frand() + 0.1f;
				particle[n].position[0] = trypos[0];
				particle[n].position[1] = trypos[1];
				particle[n].textureID = (rand() % 8);
				int paletteIndex = rand() % NUM_PALETTE_ENTRIES;
				particle[n].color[0] = palette[paletteIndex][0];
				particle[n].color[1] = palette[paletteIndex][1];
				particle[n].color[2] = palette[paletteIndex][2];
				particle[n].color[3] = palette[paletteIndex][3];
				particle[n].moveDirection[0] = 0.0f;
				particle[n].moveDirection[1] = 0.0f;
				particle[n].rotation = frand() * 3.1415926f * 2.0f;
				particle[n].rotationChange = 0.0f;
				nextParticleAdd++;
				nextParticleAdd %= NUM_PARTICLES;
				break;
			}
		}
	}

	// update particles
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		particle[i].position[0] += particle[i].moveDirection[0] * deltaTime;
		particle[i].position[1] += particle[i].moveDirection[1] * deltaTime;
		particle[i].moveDirection[0] *= (float)exp(-2.0f * deltaTime);
		particle[i].moveDirection[1] *= (float)exp(-2.0f * deltaTime);
		particle[i].rotation += particle[i].rotationChange * deltaTime;
		particle[i].rotationChange *= (float)exp(-2.0f * deltaTime);
		particle[i].age += deltaTime;
	}

	// Set the actual time
	time += deltaTime;
}

// Draws the particles. The camera is not updated. So set it beforeHand.
void ParticleEngine::draw(float cameraCenter[2])
{
	if (time > 181.75f) return;

	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		if (particle[i].age < PARTICLE_LIFTIME)
		{
			float xDist = particle[i].position[0] - cameraCenter[0];
			float yDist = particle[i].position[1] - cameraCenter[1];
			float distanceSquared = (xDist * xDist) + (yDist * yDist);
			// TODO: I might be able to set this somewhat smaller.
			if (distanceSquared < 4.0f)
			{
				float color[4];
				color[0] = particle[i].color[0];
				color[1] = particle[i].color[1];
				color[2] = particle[i].color[2];
				color[3] = particle[i].color[3];

				// set texture
				textures.setTexture(particle[i].textureID + scenes[sceneIdx] * 8);

				// set alpha
				if (particle[i].age > PARTICLE_AGE_START)
				{
					color[3] = 1.0f - 1.0f * (particle[i].age - PARTICLE_AGE_START) / PARTICLE_AGE_TIME;
				}
				color[3] *= 1.0f;

				// size ramp
				float size = particle[i].size * (1.0f - exp(-8.0f * particle[i].age));
				// size rattle
				size += 0.1f * sin(sqrtf(particle[i].age * 20.0f)) * exp(-3.0f * particle[i].age);

				// blending if shortly before shattering
				if (time > nextEmissionTime - 1.0f)
				{
					float innerTime = (time - nextEmissionTime + 1.0f) / 1.0f;
					float fader = 1.5f * innerTime * fabsf(sin(time*10.0f + (float)i)) +
						(1.0f - innerTime);
					color[3] *= fader;
					particle[i].size *= 0.05f * fader + 0.95f;
				}

				if (scenes[sceneIdx] == 2)
				{
					// paper is always black
					color[0] = color[1] = color[2] = 0.0f;
					//color[3] = 1.0f;
				}

				// draw particle
#define NUM_ANTIALIAS 8
				color[3] /= NUM_ANTIALIAS;
				glColor4fv(color);		
				glBegin(GL_QUADS);
				for (int multi = 0; multi < NUM_ANTIALIAS; multi++)
				{
					float pos[2];
					// TODO: I do not update according to camera stuff. That is BAD!
					pos[0] = particle[i].position[0] - (float)multi / NUM_ANTIALIAS * 0.04f * particle[i].moveDirection[0];
					pos[1] = particle[i].position[1] - (float)multi / NUM_ANTIALIAS * 0.04f * particle[i].moveDirection[1];
					float rotation;
					rotation = particle[i].rotation - (float)multi / 4.0f * 0.04f * particle[i].rotationChange;
					glTexCoord2f(0.0f, 1.0f);
					glVertex3f(pos[0] + size * sin(rotation),
							pos[1] + size * cos(rotation), 0.5);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3f(pos[0] + size * sin(rotation + 1.575796f),
							pos[1] + size * cos(rotation + 1.575796f), 0.5);
					glTexCoord2f(1.0f, 0.0f);
					glVertex3f(pos[0] + size * sin(rotation + 3.1415926f),
							pos[1] + size * cos(rotation + 3.1415926f), 0.5);
					glTexCoord2f(0.0, 0.0f);
					glVertex3f(pos[0] + size * sin(rotation + 4.7173889f),
							pos[1] + size * cos(rotation + 4.7173889f), 0.5);
				}				
				glEnd();
			}
		}
	}
}

float ParticleEngine::palette[NUM_PALETTE_ENTRIES][4] =
{
	{0.56f, 0.53f, 0.1f, 1.0f},
	{0.56f, 0.53f, 0.1f, 1.0f},
	{0.56f, 0.53f, 0.1f, 1.0f},
	{0.51f, 0.34f, 0.06f, 1.0f},
	{0.3f, 0.3f, 0.3f, 1.0f},
	{0.4f, 0.4f, 0.06f, 1.0f},
	{0.4f, 0.4f, 0.06f, 1.0f},
	{0.4f, 0.4f, 0.06f, 1.0f},
};

int ParticleEngine::scenes[NUM_SCENES] =
{
	0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 1, 3, 0, 1, 0, 1, 3, 1, 2, 2,
};