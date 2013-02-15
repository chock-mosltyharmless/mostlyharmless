#ifndef _PARTICLE_SYSTEM_H_
#define _PARTICLE_SYSTEM_H_
#pragma once

#include <math.h>

/**
 * In here is all the stuffs for particle systems.
 * I am not yet sure whether I will also put in rendering.
 * Particle systems are always warpped around with a dimension.
 *
 * Units of particles:
 * 1 in position is approximately the size of a particle
 * 32 in position is approximately the sphere of influence of a particle
 * 1 in speed means that the particle moves one position in one second
 */

// The number of parameters that constitute the
// current state of the particle.
#define NUM_PARTICLE_PARAMETERS 1

// The maximum number of particles in the particle System.
// Maybe there are always that many particles, they are just gone?
#define MAX_NUM_PARTICLES (1<<14)

// The grid size of the hash map that contains the particles.
// I have no idea what a sensible number might be...
#define PARTICLE_GRID_SIZE 128

// The size of the particle system where it modulo-wrappes around
// This number divided by PARTICLE_GRID_SIZE gives the maximum
// sphere of influence of a particle
#define PARTICLE_SYSTEM_SIZE (PARTICLE_GRID_SIZE * 4.0f)

// The maximum number of nearby particles that
// are considered for updating the state of this
// particle.
#define MAX_NEARBY_HEAP_DEPTH 4
#define MAX_NUM_NEARBY_PARTICLES ((1<<MAX_NEARBY_HEAP_DEPTH) - 1)

// This is the tick size of the particles system. The
// tick size defines how often the particle system has
// to be updated. After a PARTICLE_TICK amount of seconds,
// the update() method must be called
// I should make an update rate of KGV(50,60) = 300 Hz
//
// Note that I do not have to recalculate all the information
// for every tick, and that I can schedule the information update
// in a clever way so that the load is balanced. E.g. all particles
// move during a tick, but only every 10th tick the direction is
// updated but only if the hash is not updated which happens every
// 30th tick (or something like that)... 
//
// Even better: The information of the particle is only updated for a
// portion of the set of all particles. For example:
// position: all particles
// direction and properties: 500 particles
// placement in hash grid: 200 particles
// calculation of nearby particles: 150 particles
// ...
// If I am very cool, I put these into different threads (which is nasty).
#define PARTICLE_TICK (1.0f/300.0f)
// The number of particles that are update per tick and algorithm
#define NUM_PARTICLE_PROPERTIES_UPDATE 500
#define NUM_PARTICLE_GRID_UPDATE 200
#define NUM_PARTICLE_NEARBY_UPDATE 150

struct Particle;
struct Particle
{
	float position[3];

	// movement Direction is always normalized.
	float movementDirection[3];
	float movementSpeed;

	// This is the list of parameters that the particle holds
	// I do not yet have an idea how the parameters vary over time.
	float parameters[NUM_PARTICLE_PARAMETERS];

	// The linked list for the hash grid.
	// This is NULL if this is the first element of the list
	Particle *father;
	// This is NULL if this is the last element of the list
	Particle *child;
	// The cube of the grid that this particle is stored in. This
	// is not necessarily the correct cube, as cube locations are
	// updated more scarcely than other things.
	int hashGridLocation[3];

	// This is a list of particles that are potentially nearby. This
	// list is only updated rarely. There may be more particles
	// nearby, but they are ignored for the update calculations.
	// The array is arranged as a heap so that the calculation of
	// nearby particles is faster.
	// The values may be NULL with a distance of very large.
	// The heap ensures that the father is larger than all its children
	// and nothing more. It seems to work no other way???
	Particle *nearbyParticle[MAX_NUM_NEARBY_PARTICLES];
	// The squared (!) distance to the nearby particles
	float distanceToNearby[MAX_NUM_NEARBY_PARTICLES];
};

class ParticleSystem
{
public: // methods
	// Initialize the particle system. I do not know yet how I place the particles
	// inside the world. Maybe random? That would be pretty stupid. I should
	// probably pass some parameters that define the statistical distribution
	// of the particles. (Maybe just some presets)
	void init();

	// The update of the particle system.
	// Time is the number of seconds that passed since the last update
	// of the system.
	void update(float time);

	// Renders the particle system. I have yet to see which parameters
	// I have to pass to this function.
	void render(float camera[3][4]);

private: // methods
	// Makes one update of the particle system with a time of
	// PARTICLE_TICK seconds
	void updateTick();

	// Update the list of nearby particles of a single particle
	void updateNearby(int particleID);

	// Update the location of the particle inside the hash grid
	void updateHashGrid(int particleID);

	// Update the direction, move speed and properties of the particles
	void updateProperties(int particleID);

	// Move all particles according to their movement speed and direction
	// for one tick count
	void moveAllParticles();

	// Sorts the array of transformedLocations along its thrid element.
	void sortTransformed(int startIndex, int numEntries);

	// Returns the distance vector between two vectors.
	// Note that this handles particle-system wraparound correctly only
	// if the vector locations are within [0..PARTICLE_SYSTEM_SIZE]
	static void getVectorDifference(float pos1[3], float pos2[3], float result[3]);

	// Get the grid location from a given floating position
	// This only works if the position is within [0..PARTICLE_SYSTEM_SIZE[
	static int getGridIndex(float pos)
	{
		int index = (int)(pos * (PARTICLE_GRID_SIZE / PARTICLE_SYSTEM_SIZE));
		// Due to problems with floatint point, I need to modulo the size.
		index = (index + PARTICLE_GRID_SIZE) % PARTICLE_GRID_SIZE;
		return index;
	}

private: // data
	// The particles that are there. I do not yet know how to have particles
	// disappear? Maybe a bitfield? We'll see.
	Particle particle[MAX_NUM_PARTICLES];

	// Transformed (rotated etc.) and projected particle locations
	// This is X, Y, W (but no Z...)
	float transformedLocation[MAX_NUM_PARTICLES][3];

	// Sorted indices into the transformedLocation array so that
	// I can render back to front.
	int transformedIndex[MAX_NUM_PARTICLES];

	// I need to consider how to do fast lookup of nearby particles. I think for
	// now I will use a 3D grid that has a hash map of where the particles are.
	// Each entry in the hash map has a bidirectional linked list of particles
	// that are inside this grid location.
	// The grid is spaced in a way so that elements that are two steps away from
	// the current grid point are never considered.
	// As far as I know there are no operations that need to be done on the whole
	// grid after initialization. If a particle moves, it deletes itself from the
	// cube's linked list and attaches it to the new one. In order to find
	// the neighbourhood of a particles, I only consort the neighbors.
	// So it's only a problem of memory size and potentially of cache?
	//
	// This is NULL if this grid cube has no particles in it.
	Particle *hashGrid[PARTICLE_GRID_SIZE][PARTICLE_GRID_SIZE][PARTICLE_GRID_SIZE];

	// The following data is for update rates of particle algorithms in the updateTick
	// function. The value contains the next particle to be updated.
	int updatePropertiesLoc;
	int updateGridLoc;
	int updateNearbyLoc;

	// The fraction of time that was not processed during the last call
	// to update(). This time is then used in the next call.
	float unprocessedTime;
};

#endif
