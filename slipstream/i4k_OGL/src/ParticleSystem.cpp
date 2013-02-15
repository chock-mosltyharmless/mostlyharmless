#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <assert.h>
#include "glext.h"

#include "ParticleSystem.h"
#include "Mathhelpers.h"

// TODO: CheckIndex should only be called if this is
//       a debug build???
#define checkIndex(index, maximum) {assert(index >= 0); assert(index < maximum);}

// Calculate fmod(x, y) by doing
// (x * invy) - floor(x * invy) * y
// Note: This also works for negative values, as floor does not
//       truncate but floor(-0.x) = -1.0
static float jfmod(float x, float y, float invy)
{
	float adjustedX = x * invy;
	float cropped = adjustedX - floor(adjustedX);
	return cropped * y;
}

void ParticleSystem::init()
{
	// Set the starting position, movement and state of all particles
	for (int k = 0; k < MAX_NUM_PARTICLES; k++)
	{
		checkIndex(k, MAX_NUM_PARTICLES);
		Particle *p = &(particle[k]);

		float movDirNorm = 0.0f;
		for (int dim = 0; dim < 3; dim++)
		{
			p->position[dim] = fjrand() * PARTICLE_SYSTEM_SIZE;
			p->movementDirection[dim] = fjrand() - 0.5f;
			movDirNorm += p->movementDirection[dim] * p->movementDirection[dim];
		}
		p->movementSpeed = fjrand() * 5.0f;

		// normalize movement direction
		movDirNorm = 1.0f / sqrtf(movDirNorm);
		for (int dim = 0; dim < 3; dim++)
		{ 
			p->movementDirection[dim] *= movDirNorm;
		}
	}

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
		checkIndex(k, MAX_NUM_PARTICLES);
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

void ParticleSystem::render(float camera[3][4])
{
	const float nearClipPlane = 0.01f;
	const float farClipPlane = 100.0f;

	// Number of transformed particles that shall be rendered.
	// At most this is MAX_NUM_PARTICLES, but it may be less.
	int numTransformed = 0;

	// transform and project all particles
	for (int k = 0; k < MAX_NUM_PARTICLES; k++)
	{
		for (int dim = 0; dim < 3; dim++)
		{
			// transformation
			transformedLocation[numTransformed][dim] = camera[dim][3];
			for (int i = 0; i < 3; i++)
			{
				checkIndex(k, MAX_NUM_PARTICLES);
				transformedLocation[numTransformed][dim] +=
					particle[k].position[i] * camera[dim][i];
			}

			// now I need to check that I get positions around
			// -PARTICLE_SYSTEM_SIZE/2..PARTICLE_SYSTEM_SIZE/2
			// As the camera is at 0,0,0... (I could do something else
			// for z, but I won't)???
			transformedLocation[numTransformed][dim] = 
					jfmod(transformedLocation[numTransformed][dim] + PARTICLE_SYSTEM_SIZE/2.0f,
						  PARTICLE_SYSTEM_SIZE, 1.0f/PARTICLE_SYSTEM_SIZE) -
					PARTICLE_SYSTEM_SIZE/2.0f;
		}

		// Here comes the FOV check:
		if (transformedLocation[numTransformed][2] > nearClipPlane &&
			transformedLocation[numTransformed][2] < farClipPlane)
		{
			// projection
			checkIndex(numTransformed, MAX_NUM_PARTICLES);
			float w = 1.0f / transformedLocation[numTransformed][2];
			transformedLocation[numTransformed][0] *= w;
			transformedLocation[numTransformed][1] *= w;
			transformedLocation[numTransformed][2] = w;

			// Check for left and right clip planes:
			// TODO: Here I need to take the width and height of
			//       the particle into account so that the borders
			//       are not taken away.
			if (transformedLocation[numTransformed][0] > -1.0f &&
				transformedLocation[numTransformed][0] < 1.0f &&
				transformedLocation[numTransformed][1] > -1.0f &&
				transformedLocation[numTransformed][1] < 1.0f)
			{
				// This particle is actually used
				numTransformed++;
			}
		}
	}

	// Sort the particles so that I can render back to front
	// build up indices (this is inefficient!!!)
	// DEBUG!
	for (int k = 0; k < MAX_NUM_PARTICLES; k++)
	{
		transformedIndex[k] = -1;
	}
	for (int k = 0; k < numTransformed; k++)
	{
		transformedIndex[k] = k;
	}
	sortTransformed(0, numTransformed);

	// turns out that sorting may very well introduce incorrect values
	// That is, one element too far to the right is actually taken.

	// I will just do bad ass normal rendering
	glBegin(GL_QUADS);
	for (int k = 0; k < numTransformed; k++)
	{
		checkIndex(k, MAX_NUM_PARTICLES);
		int realIdx = transformedIndex[k];

		checkIndex(realIdx, numTransformed);
		float x = transformedLocation[realIdx][0];
		float y = transformedLocation[realIdx][1];
		float w = transformedLocation[realIdx][2];

		// Test whether particle shall be rendered
		// This is tricky... I should do the positive z check earlier?
		// For now very simple FOV: Color is w?
		float depthColor = 2.0f * transformedLocation[realIdx][2];
		glColor3f(depthColor, depthColor, depthColor);

		// TODO: size of a quad is just 2.0/z right now...
		// I am ignoring the camera??? This is not right.
		// I actually need to extract the scaling form the 
		// matrix or else hand it in as an additional parameter
		glVertex3f(x-w, y-w, w);
		glVertex3f(x+w, y-w, w);
		glVertex3f(x+w, y+w, w);
		glVertex3f(x-w, y+w, w);
	}
	glEnd();
}

void ParticleSystem::update(float time)
{
	// also spend the time that was not spent during the last call
	time += unprocessedTime;

	// We do a savety here so that we do not deadlock
	// We expect a minimum of 10 Hz.
	if (time > 0.1f) time = 0.1f;

	while (time >= PARTICLE_TICK)
	{
		updateTick();
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

void ParticleSystem::updateProperties(int particleID)
{
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
			checkIndex(p->hashGridLocation[0], PARTICLE_GRID_SIZE);
			checkIndex(p->hashGridLocation[1], PARTICLE_GRID_SIZE);
			checkIndex(p->hashGridLocation[2], PARTICLE_GRID_SIZE);
			hashGrid[p->hashGridLocation[0]][p->hashGridLocation[1]][p->hashGridLocation[2]] =
				p->child;
		}

		// Add this particle as the new grid to the grid.
		checkIndex(newHashPos[0], PARTICLE_GRID_SIZE);
		checkIndex(newHashPos[1], PARTICLE_GRID_SIZE);
		checkIndex(newHashPos[2], PARTICLE_GRID_SIZE);
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
		checkIndex(k, MAX_NUM_PARTICLES);
		Particle *p = &(particle[k]);
		for (int dim = 0; dim < 3; dim++)
		{
			float pos = p->position[dim];
			pos += PARTICLE_TICK * p->movementSpeed *
				p->movementDirection[dim];
			// Modulo of the position
			// fmod turns out to be crippling slow. So I'd rather
			// do something else here...
			pos = jfmod(pos, PARTICLE_SYSTEM_SIZE, 1.0f/PARTICLE_SYSTEM_SIZE);

			// Theoretically, at this point, p->position should
			// be in [0, PARTICLE_SYSTEM_SIZE[
			// Due to the fact that floats change their accuracy
			// depending on context, p->position is actually in
			// the range [0, PARTICLE_SYSTEM_SIZE]!!!!
			p->position[dim] = pos;
		}
	}
}

void ParticleSystem::updateNearby(int particleID)
{
	checkIndex(particleID, MAX_NUM_PARTICLES);
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
				Particle *other = hashGrid[(p->hashGridLocation[0] + xi + PARTICLE_GRID_SIZE) % PARTICLE_GRID_SIZE]
										  [(p->hashGridLocation[1] + yi + PARTICLE_GRID_SIZE) % PARTICLE_GRID_SIZE]
										  [(p->hashGridLocation[2] + zi + PARTICLE_GRID_SIZE) % PARTICLE_GRID_SIZE];
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
						checkIndex(nextHeapLocation, MAX_NUM_NEARBY_PARTICLES);
						checkIndex(nextHeapLocation+1, MAX_NUM_NEARBY_PARTICLES);
						if (p->distanceToNearby[nextHeapLocation+1] > p->distanceToNearby[nextHeapLocation])
						{
							nextHeapLocation = nextHeapLocation + 1;
						}

						// Check whether something has to be done with the follow-up
						checkIndex(nextHeapLocation, MAX_NUM_NEARBY_PARTICLES);
						if (p->distanceToNearby[nextHeapLocation] > distance)
						{
							// The stuff must be swapped.
							checkIndex(heapLocation, MAX_NUM_NEARBY_PARTICLES);
							checkIndex(nextHeapLocation, MAX_NUM_NEARBY_PARTICLES);
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

// use quicksort (for whatever reason)
// sort in ascending order (for whatever reason)
void ParticleSystem::sortTransformed(int startIndex, int numEntries)
{
	// The seed value is used to pick a random index.
	// This is done to prevent strange cases from being a problem
	static unsigned int seed;
	
	// update seed so that the next access is also random.
	// This is a very bad random number generator, but look if I care.
	seed += 3571;
	seed *= 3517;

	if (numEntries == 2)
	{
		int index1 = transformedIndex[startIndex];
		int index2 = transformedIndex[startIndex+1];
		// check if we have to sort
		if (transformedLocation[index1][2] > transformedLocation[index2][2])
		{
			int tmp = transformedIndex[startIndex];
			transformedIndex[startIndex] = transformedIndex[startIndex+1];
			transformedIndex[startIndex+1] = tmp;
		}
		return;
	}
	if (numEntries < 2)
	{
		// One or no entries do not have to be sorted.
		return;
	}

	// Use the location for the pivot
	int location = (seed % numEntries) + startIndex;
	int pivotIdx = transformedIndex[location];
	transformedIndex[location] = transformedIndex[startIndex];
	transformedIndex[startIndex] = pivotIdx;
	float pivot = transformedLocation[pivotIdx][2];
	// pivot is now at the start of the list.

	// partition the list.
	// left points to the next element on the left side
	// right points to the next element on the right side
	int left = startIndex + 1;
	int right = startIndex + numEntries - 1;
	while (left < right)
	{
		// move the left border until something to move is found
		int leftPos = transformedIndex[left];
		// I need <= here so that the left index moves on if something goes wrong...
		while (left < right && transformedLocation[leftPos][2] <= pivot)
		{
			// this thing is on the right side already
			left++;
			leftPos = transformedIndex[left];
		}

		// move the right border until something to move is found
		int rightPos = transformedIndex[right];
		while (left < right && transformedLocation[rightPos][2] >= pivot)
		{
			// this thing is on the right side already
			right--;
			rightPos = transformedIndex[right];
		}

		// Exchange the two indices, as they are both wrong
		if (left < right)
		{
			checkIndex(left - startIndex, numEntries);
			checkIndex(right - startIndex, numEntries);
			int tmp = transformedIndex[left];
			transformedIndex[left] = transformedIndex[right];
			transformedIndex[right] = tmp;
		}

		// I can go on now, because left one is smaller than right one
		// left++;
		// right--;
		// can I really?
	}

	// special case: At left we are still smaller than
	// pivot. That happens if all entries were smaller than
	// pivot.
	int leftPos = transformedIndex[left];
	if (transformedLocation[leftPos][2] <= pivot)
	{
		// In this case we actually change the pivot element with that one
		left++;
	}

	// Now I only need to move the pivot element, but where?
	// I know that left was at least moved once, right?
	checkIndex(left - startIndex - 1, numEntries);
	int tmp = transformedIndex[left-1];
	transformedIndex[left-1] = transformedIndex[startIndex];
	transformedIndex[startIndex] = tmp;

	// recurse
	// I really hope that I got those numbers right. I won't check it.
	sortTransformed(startIndex, (left-2)-startIndex + 1);
	sortTransformed(left, (startIndex + numEntries - 1) - left + 1);
}