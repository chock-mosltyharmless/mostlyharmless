#include "StdAfx.h"
#include "Swarm.h"
#include "mathhelpers.h"

// Global data

// Triangle data
Triangles tris;

// Get signed distance value.

// get signed distance value
static float getSDSphere(float *position)
{
	float length = 0.0f;

	length = sqrtf(position[0] * position[0] +
			       position[1] * position[1] +
				   position[2] * position[2]);
	length -= 3.0;
	return length;
}

// get Normal. I may need to use the signed distance for that?
static void getSDNormalSphere(float *position, float *normal)
{
	normal[0] = position[0];
	normal[1] = position[1];
	normal[2] = position[2];

	normalize(normal, 3);
}

// three metaballs SD
static float metaCenters[3][3];
static float getSD(float *position, int sdIndex)
{
	float value = 0.0f;

	if (sdIndex == METABALL_FIELD)
	{
		for (int ball = 0; ball < 3; ball++)
		{
			position[0] -= metaCenters[ball][0];
			position[1] -= metaCenters[ball][1];
			position[2] -= metaCenters[ball][2];

			value += 1.0f / (position[0] * position[0] +
							 position[1] * position[1] +
							 position[2] * position[2] + 0.01f);
		
			position[0] += metaCenters[ball][0];
			position[1] += metaCenters[ball][1];
			position[2] += metaCenters[ball][2];
		}
		
		value = sqrtf(1.0f / value) - 1.3f;
	}
	else if (sdIndex == SPHERE_FIELD)
	{
		float dist = position[0] * position[0] +
					 position[1] * position[1] +
					 position[2] * position[2];
		value = sqrtf(dist) - 2.0f;
	}
	else
	{
		value = 1000.0f;
	}

	return value;
}

static void getSDNormal(float *position, float *normal, int sdIndex)
{
	normal[0] = 0.0f;
	normal[1] = 0.0f;
	normal[2] = 0.0f;

	if (sdIndex == METABALL_FIELD)
	{
		for (int ball = 0; ball < 3; ball++)
		{
			position[0] -= metaCenters[ball][0];
			position[1] -= metaCenters[ball][1];
			position[2] -= metaCenters[ball][2];

			float value = 1.0f / (position[0] * position[0] +
								  position[1] * position[1] +
								  position[2] * position[2] + 0.01f);
			value = value * value;
			normal[0] += value * position[0];
			normal[1] += value * position[1];
			normal[2] += value * position[2];
		
			position[0] += metaCenters[ball][0];
			position[1] += metaCenters[ball][1];
			position[2] += metaCenters[ball][2];
		}
	}
	else if (sdIndex == SPHERE_FIELD)
	{
		normal[0] = position[0];
		normal[1] = position[1];
		normal[2] = position[2];
	}

	normalize(normal, 3);
}


// update center positions of the metaballs
static void updateSD(int sdIndex, float fDeltaTime)
{
	static float randomValues[3][3][2];
	static bool randomValuesAreMade = false;
	static float time = 0.0f;

	time += fDeltaTime;

	if (!randomValuesAreMade)
	{
		randomValuesAreMade = true;
		for (int i = 0; i < 3*3*2; i++)
		{
			randomValues[0][0][i] = frand() * 1.0f;
		}
	}
	for (int i = 0; i < 3*3; i++)
	{
		metaCenters[0][i] = 2.5f * sinf(randomValues[0][i][0]*time + 3.0f * randomValues[0][i][1]);
	}
}



// triangle position initialization
// Call this once to set the initial position of the swarm
void initSwarm(void)
{
	for (int i = 0; i < NUM_TRIANGLES; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			tris.position[i][j] = (frand() - 0.5f) * 10.0f;
			tris.direction[i][j] = (frand() - 0.5f);
			tris.normal[i][j] = (frand() - 0.5f);
		}
		normalize(tris.direction[i], 3);
		normalize(tris.normal[i], 3);
		reNormal(tris.normal[i], tris.direction[i], 3);
	}
}

// Update the position of the swarm, this does not update the move direction and so on
// You can multiply the deltaTime with a constant so that you get slowmotion or speedup
void moveSwarm(float deltaTime)
{
	// update position normally
	for (int i = 0; i < NUM_TRIANGLES; i++)
	{
		float pointLengthSquared = 0.0f;
		for (int j = 0; j < 3; j++)
		{
			// update position based on fly direction
			tris.position[i][j] += tris.direction[i][j] * deltaTime * 3.0f;
			pointLengthSquared += tris.position[i][j] * tris.position[i][j];
		}

		// If a point flies out of range, it comes back at the other end.
		// Now if that is not a stupid update rule...
		if (pointLengthSquared > MAX_POINT_DISTANCE_SQUARED)
		{
			for (int j = 0; j < 3; j++)
			{
				tris.position[i][j] = -tris.position[i][j];
			}
		}
	}
}

void updateSwarmWithSignedDistance(int distanceID, float fDeltaTime, float sdInfluence)
{
	// update the signed distance function based on the time passed.
	updateSD(distanceID, fDeltaTime);

	for (int i = 0; i < NUM_TRIANGLES; i++)
	{
		// update direction based on destination point
		float destDirection[3];
		destDirection[0] = tris.moveToPoint[i][0] - tris.position[i][0];
		destDirection[1] = tris.moveToPoint[i][1] - tris.position[i][1];
		destDirection[2] = tris.moveToPoint[i][2] - tris.position[i][2];
		// normalize the length to where we go so that the speed is fixed.
		normalize(destDirection, 3);
		slerp(tris.direction[i], destDirection, tris.direction[i], 3, 1.8f * fDeltaTime);
		reNormal(tris.normal[i], tris.direction[i], 3);

		// update normal based on signed distance
		float signedDist = getSD(tris.position[i], distanceID);
		getSDNormal(tris.position[i], destDirection, distanceID);
		float t = 1.0f - fabsf(signedDist);
		t = t < 0.0f ? 0.0f : t;
		t = t > 1.0f ? 1.0f : t;
		slerp(tris.normal[i], destDirection, tris.normal[i], 3, t * sdInfluence);
		reNormal(tris.direction[i], tris.normal[i], 3);
	}
}

void updateSwarmDestinations(int pathID, float fDeltaTime, float overShootAmount)
{
	static float oldOverShootRing[NUM_TRIANGLES];
	static int ringPosition = NUM_TRIANGLES - 1; // end point of the overshoot ring.
	static float fCurTime = 0.0f;
	fCurTime += fDeltaTime;
	const int numMPts = 8;

	// TODO: How do I make this not too much framerate dependent?
	// TODO: Accuracy
	for (int i = 0; i < (int)(fDeltaTime * 300.0f + 1.0f); i++)
	{
		oldOverShootRing[ringPosition] = overShootAmount;
		ringPosition--;
		if (ringPosition < 0) ringPosition = NUM_TRIANGLES - 1;
	}

	if (pathID == 0)
	{
		// 8 moving move-to points
		float moveToPoint[numMPts][3];
		static float randomValues[numMPts][3][4];
		static bool randomValuesAreMade = false;

		if (!randomValuesAreMade)
		{
			randomValuesAreMade = true;
			for (int ptIdx = 0; ptIdx < numMPts; ptIdx++)
			{
				for (int i = 0; i < 3 * 4; i++)
				{
					randomValues[ptIdx][0][i] = frand() * 1.0f;
				}
			}
		}

		for (int ptIdx = 0; ptIdx < numMPts; ptIdx++)
		{
			moveToPoint[ptIdx][0] = 3.04f * sinf(fCurTime * randomValues[ptIdx][0][0] + randomValues[ptIdx][0][1]) +
					2.71f * sinf(fCurTime * randomValues[ptIdx][0][2] + randomValues[ptIdx][0][3]);
			moveToPoint[ptIdx][1] = 2.53f * sinf(fCurTime * randomValues[ptIdx][1][0] + randomValues[ptIdx][1][1]) +
					3.22f * sinf(fCurTime * randomValues[ptIdx][1][2] + randomValues[ptIdx][1][3]);
			moveToPoint[ptIdx][2] = 2.57f * sinf(fCurTime * randomValues[ptIdx][2][0] + randomValues[ptIdx][2][1]) +
					2.83f * sinf(fCurTime * randomValues[ptIdx][2][2] + randomValues[ptIdx][2][3]);
		}

		for (int i = 0; i < NUM_TRIANGLES; i++)
		{
			// Determine the closest move to point
			float closestDist = 1.0e20f;
			int closestIdx = 0;
			for (int ptIdx = 0; ptIdx < numMPts; ptIdx++)
			{
				float dist = 0.0f;
				for (int dim = 0; dim < 3; dim++)
				{
					float d = tris.position[i][dim] - moveToPoint[ptIdx][dim];
					dist += d * d;
				}
				if (dist < closestDist)
				{
					closestDist = dist;
					closestIdx = ptIdx;
				}
			}

			float overShoot = oldOverShootRing[(ringPosition+i)%NUM_TRIANGLES];

			tris.moveToPoint[i][0] = overShoot * moveToPoint[closestIdx][0] + frand() - 0.5f;
			tris.moveToPoint[i][1] = overShoot * moveToPoint[closestIdx][1] + frand() - 0.5f;
			tris.moveToPoint[i][2] = overShoot * moveToPoint[closestIdx][2] + frand() - 0.5f;
		}
	}
	else if (pathID == 1)
	{
		// 4 swirling lines 
		/* Lines update */
		for (int triIdx = 0; triIdx < NUM_TRIANGLES; triIdx++)
		{
			float zPos = tris.position[triIdx][2];
			float phase = 10.0f * sinf(zPos*0.05f + fCurTime*0.3f) * 0.15f;
			float bestAngle = 0;
			int bestLine = 0;
			float closestDist = 1.0e20f;
			for (int i = 0; i < 4; i++)
			{
				float linex = sinf(phase);
				float liney = cosf(phase);
				float distX = tris.position[triIdx][0] - linex;
				float distY = tris.position[triIdx][1] - liney;
				float dist = distX * distX + distY * distY;
				if (dist < closestDist)
				{
					closestDist = dist;
					bestAngle = phase;
					bestLine = i;
				}
				phase += 3.1415926f * 0.5f;
			}
		
			tris.moveToPoint[triIdx][0] = 2.7f * sinf(bestAngle);
			tris.moveToPoint[triIdx][1] = 2.7f * cosf(bestAngle);
			float direction = sinf(fCurTime*0.4f);
			if (bestLine & 1) direction = -direction;
			tris.moveToPoint[triIdx][2] = tris.position[triIdx][2] + direction - 0.1f * tris.position[triIdx][2];
		}
	}
}