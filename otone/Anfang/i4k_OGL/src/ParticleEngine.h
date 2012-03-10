#pragma once

#include <math.h>
#include <stdlib.h>

#define NUM_PARTICLES 2048
// Time when the ageing of the particle starts
#define PARTICLE_AGE_START 8.0f
// Time from ageing till the death of a particle
#define PARTICLE_AGE_TIME 2.0f
// The total lifetime of a particle
#define PARTICLE_LIFTIME (PARTICLE_AGE_START + PARTICLE_AGE_TIME)
// Time between two updates of the emission
#define EMISSION_UPDATE_TIME 9.57f
#define NUM_EMITTED_PARTICLES 16
#define NUM_PALETTE_ENTRIES 8
#define NUM_SCENES 256

class Particle
{
public: // data
	// The current position
	float position[2];
	// The color (fixed)
	float color[4];
	// The size of the particle (fixed)
	float size;
	// the current speed and direction of the particle
	// The particle slows down after birth.
	float moveDirection[2];
	// The rotation of the sprite.
	float rotation;
	float rotationChange;
	// age of the particle in seconds
	float age;
	// The texture. Rather static. Problem is they will change often...
	int textureID;
};

class ParticleEngine
{
public:
	ParticleEngine(void);
	~ParticleEngine(void);

public: // methods
	// The center point is the point where new particles are generated.
	void update(float deltaTime, float centerPoint[2], float shortEmissionTime);
	// Draws the particles. The camera is not updated. So set it beforeHand.
	// CameraCenter is used for culling images.
	void draw(float cameraCenter[2]);

private: // methods
	float frand() {return (float)rand()/(float)RAND_MAX;}

private: // data
	int nextParticleAdd;
	int sceneIdx;
	float time;
	float nextEmissionTime; // last time that particles were emitted
	float nextShortEmissionTime;
	Particle particle[NUM_PARTICLES];
	static float palette[NUM_PALETTE_ENTRIES][4];
	static int scenes[NUM_SCENES];
};

