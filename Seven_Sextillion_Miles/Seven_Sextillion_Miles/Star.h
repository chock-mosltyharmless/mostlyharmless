#pragma once

#include "Planet.h"
#include "GLGraphics.h"
#include "Location.h"
#include "Camera.h"

class Star
{
public:
	Star(void);
	virtual ~Star(void);

	// Returns 0 on success
	int init(Location loc, char *errorString);

	int draw(GLGraphics *renderer, Camera *camera, char *errorString);

private:
	// Location of the star (everything rotates around this)
	Location sunLocation;
	
	// Texture name of the central sun to render
	char *sunTextureName;
	float sunRadius;

	// Array of Planets (fixed size, there are no more)
	int numPlanets;
	Planet *planetList;
};

