#include "ParticleSystem.h"

void ParticleSystem::init()
{
	// Set the starting position, movement and state of all particles
	// Also delete the grid stuff.
	// TODO

	// delete the hashGrid
	for (int z = 0; z < PARTICLE_GRID_SIZE; z++)
	{
		for (int y = 0; y < PARTICLE_GRID_SIZE; y++)
		{
			for (int x = 0; x < PARTICLE_GRID_SIZE; x++)
			{
				hashGrid[z][y][x] = 0;
			}
		}
	}

	// Set the hash grid of all particles.
	for (int k = 0; k < MAX_NUM_PARTICLES; k++)
	{
		Particle *p = &(particle[k]);

		// determine grid location
		for (int dim = 0; dim < 3; dim++)
		{
			p->hashGridLocation[dim] = getGridIndex(p->position[dim]);
		}
	}

	// Initialize the values that are needed for the
	// state machine update of the physics.
	updatePropertiesLoc = 0;
	updateGridLoc = 0;
	updateNearbyLoc = 0;
	unprocessedTime = 0;
}

void ParticleSystem::update(float time)
{
	// also spend the time that was not spent during the last call
	time += unprocessedTime;

	while (time >= PARTICLE_TICK)
	{
		updateTick;
		time -= PARTICLE_TICK;
	}

	// The time that was not spent processing will be used later on.
	unprocessedTime = time;
}

void ParticleSystem::updateTick()
{
	moveAllParticles();

	for (int i = 0; i < NUM_PARTICLE_PROPERTIES_UPDATE; i++)
	{
		updateProperties(updatePropertiesLoc++);
		updatePropertiesLoc %= MAX_NUM_PARTICLES;
	}

	for (int i = 0; i < NUM_PARTICLE_GRID_UPDATE; i++)
	{
		updateHashGrid(updateGridLoc++);
		updateGridLoc %= MAX_NUM_PARTICLES;
	}

	for (int i = 0; i < NUM_PARTICLE_NEARBY_UPDATE; i++)
	{
		updateNearby(updateNearbyLoc++);
		updateNearbyLoc %= MAX_NUM_PARTICLES;
	}
}

void ParticleSystem::updateHashGrid(int particleID)
{
	Particle *p = &(particle[particleID]);

	// get the current location in the hash grid
	int newHashPos[3];
	bool isAlreadyCorrect = true;
	for (int dim = 0; dim < 3; dim++)
	{
		newHashPos[dim] = getGridIndex(p->position[dim]);
		isAlreadyCorrect = isAlreadyCorrect &&
			(newHashPos[dim] == p->hashGridLocation[dim]);
	}

	// Location only has to be updated if it is not yet correct.
	if (!isAlreadyCorrect)
	{
		// remove the particle from the current grid.
		if (p->father)
		{
			// It is not the root node. Connect the father with the child.
			p->father->child = p->child;
		}
		else
		{
			// This is the root node. Make the child the root in the grid.
			// Note that this is also correct if the child is NULL.
			hashGrid[p->hashGridLocation[0]][p->hashGridLocation[1]][p->hashGridLocation[2]] =
				p->child;
		}

		// Add this particle as the new grid to the grid.
		p->child = hashGrid[newHashPos[0]][newHashPos[1]][newHashPos[2]];
		hashGrid[newHashPos[0]][newHashPos[1]][newHashPos[2]] = p;

		// Register the hash location in the particle.
		for (int dim = 0; dim < 3; dim++)
		{
			p->hashGridLocation[dim] = newHashPos[dim];
		}
	}
}

void ParticleSystem::moveAllParticles()
{
	for (int k = 0; k < MAX_NUM_PARTICLES; k++)
	{
		Particle *p = &(particle[k]);
		for (int dim = 0; dim < 3; dim++)
		{
			p->position[dim] += PARTICLE_TICK * p->movementSpeed *
				p->movementDirection[dim];
			// Modulo of the position
			if (p->position[dim] >= PARTICLE_SYSTEM_SIZE) p->position[dim] -= PARTICLE_SYSTEM_SIZE;
			if (p->position[dim] < 0.0f) p->position[dim] += PARTICLE_SYSTEM_SIZE;
		}
	}
}

void ParticleSystem::updateNearby(int particleID)
{
	Particle *p = &(particle[particleID]);

	// delete the list of nearby particles
	for (int k = 0; k < MAX_NUM_NEARBY_PARTICLES; k++)
	{
		p->nearbyParticle[k] = 0;
		p->distanceToNearby[k] = PARTICLE_SYSTEM_SIZE*PARTICLE_SYSTEM_SIZE;
	}

	// Iterate though all possible neighbor particles
	for (int zi = -1; zi <= 1; zi++)
	{
		for (int yi = -1; yi <= 1; yi++)
		{
			for (int xi = -1; xi < 1; xi++)
			{
				Particle *other = hashGrid[(p->hashGridLocation[0] + xi) % PARTICLE_GRID_SIZE]
										  [(p->hashGridLocation[0] + xi) % PARTICLE_GRID_SIZE]
										  [(p->hashGridLocation[0] + xi) % PARTICLE_GRID_SIZE];
				// Move through the linked list until there is nothing left
				while (other)
				{
					// Get the distance to the other particle
					float diff[3];
					getVectorDifference(p->position, other->position, diff);
					float distance = diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2];
					
					// Note that this is somewhat inefficient... I do some stuff several times
					// If the inserted particle is farther away than this,
					// we are screwed:
					if (distance > p->distanceToNearby[0])
					{
						break;
					}
					else
					{
						p->distanceToNearby[0] = distance;
						p->nearbyParticle[0] = other;
					}

					// Try to insert particle into the nearby heap
					int heapLocation = 0;
					for (int i = 0; i < MAX_NEARBY_HEAP_DEPTH-1; i++)
					{
						// Check with the larger of the two successors
						int nextHeapLocation = heapLocation * 2 + 1;
						if (p->distanceToNearby[nextHeapLocation+1] > p->distanceToNearby[nextHeapLocation])
						{
							nextHeapLocation = nextHeapLocation + 1;
						}

						// Check whether something has to be done with the follow-up
						if (p->distanceToNearby[nextHeapLocation] > distance)
						{
							// The stuff must be swapped.
							p->distanceToNearby[heapLocation] = p->distanceToNearby[nextHeapLocation];
							p->nearbyParticle[heapLocation] = p->nearbyParticle[nextHeapLocation];
							// The current stuff is inserted at the next heap location
							heapLocation = nextHeapLocation;
							p->distanceToNearby[nextHeapLocation] = distance;
							p->nearbyParticle[nextHeapLocation] = other;
						}
						else
						{
							// Everything is heapy, nothing needs to be done.
							break;
						}
					}

					// Go forward in the linked list
					other = other->child;
				}
			}
		}
	}
}

void ParticleSystem::getVectorDifference(float pos1[3], float pos2[3], float result[3])
{
	for (int k = 0; k < 3; k++)
	{
		float res = pos2[k] - pos1[k];
		res = res <= PARTICLE_SYSTEM_SIZE/2  ? res : res - PARTICLE_SYSTEM_SIZE;
		res = res >= -PARTICLE_SYSTEM_SIZE/2 ? res : res + PARTICLE_SYSTEM_SIZE;
		result[k] = res;
	}
}
