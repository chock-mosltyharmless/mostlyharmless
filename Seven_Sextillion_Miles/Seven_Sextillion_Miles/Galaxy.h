#pragma once

#include "GLGraphics.h"
#include "Camera.h"
#include "Star.h"
#include "Location.h"

// Number of initially allocated memory for stars
#define GXY_NUM_INITIAL_STARS 64

class Galaxy
{
public:
	Galaxy(void);
	virtual ~Galaxy(void);

	// Returns 0 on success
	int init(Location loc, char *errorString);

	/// Returns 0 on success
	/// the starID is the ID of the star that has to be rendered
	int draw(GLGraphics *renderer, Camera *camera, int starID, char *errorString);

private:
	// Returns 0 on success
	int addStar(Location *loc, char *errorString);

private:
	// Array of Stars (resizable)
	int numStars;
	int numAllocatedStars;
	Star *starList;

	// Location of the galaxy in the universe (static)
	// This is the center of the galaxy which is not the jump position...
	Location position;
};

