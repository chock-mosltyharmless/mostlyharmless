#pragma once

#define METABALL_FIELD 2
#define SPHERE_FIELD 1

// Triangle data
#define NUM_TRIANGLES 3000
struct Triangles
{
	float position[NUM_TRIANGLES][3];
	float direction[NUM_TRIANGLES][3];
	float normal[NUM_TRIANGLES][3];
	float moveToPoint[NUM_TRIANGLES][3];
};

// Maximum squared distance of a triangle to the center of the scene
#define MAX_POINT_DISTANCE (128.0f)
#define MAX_POINT_DISTANCE_SQUARED (MAX_POINT_DISTANCE * MAX_POINT_DISTANCE)
void initSwarm(void);
// Updates the positions of the swarms
void moveSwarm(float deltaTime);
// Update the direction that the individual triangles move to
// based on their destination points and the chosen signed distance field identifier.
void updateSwarmWithSignedDistance(int distanceID, float fDeltaTime, float sdInfluence);
// Update the position where the swarm will move to.
// The overshootAmount is a multiplicative value. This value modifies the destination
// of the point, but delayed based on the index of the particle.
void updateSwarmDestinations(int pathID, float fDeltaTime, float overShootAmount);

extern Triangles tris;