#include "StdAfx.h"
#include "Galaxy.h"
#include "global.h"

Galaxy::Galaxy(void)
{
	starList = NULL;
}

Galaxy::~Galaxy(void)
{
	if (starList) delete [] starList;
}

int Galaxy::init(Location loc, char *errorString)
{
	// Copy the position
	position = loc;

	// Allocate memory for stars in the galaxy
	numStars = 0;
	numAllocatedStars = GXY_NUM_INITIAL_STARS;
	starList = new Star[numAllocatedStars];
	if (!starList)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Out of memory");
		return ERROR_OUT_OF_MEMORY;
	}

	// Create a single Star in the center of the Galaxy
	Location starLocation;
	starLocation.setPos(loc.getXPos(), loc.getYPos());
	starLocation.setRot(0.0f);
	starLocation.setSpeed(0.0f, 0.0f);
	starLocation.setRotSpeed(0.0f);
	int returnValue = addStar(&starLocation, errorString);
	if (returnValue != 0)
	{
		return returnValue;
	}

	return 0;
}

int Galaxy::addStar(Location *loc, char *errorString)
{
	if (numStars == numAllocatedStars)
	{
		// Have to reallocate
		Star *newList = new Star[numAllocatedStars * 2];
		if (!newList)
		{
			sprintf_s(errorString, MAX_ERROR_LENGTH, "Out of memory");
			return ERROR_OUT_OF_MEMORY;
		}
		// Copy old list to new list
		for (int i = 0; i < numAllocatedStars; i++)
		{
			newList[i] = starList[i];
		}
		numAllocatedStars *= 2;
		delete [] starList;
		starList = newList;
	}

	numStars++;
	return starList[numStars - 1].init(*loc, errorString);
}

int Galaxy::draw(GLGraphics *renderer, Camera *camera, int starID, char *errorString)
{
	return starList[starID].draw(renderer, camera, errorString);
}