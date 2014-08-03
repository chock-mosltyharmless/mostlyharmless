#include "StdAfx.h"
#include "Universe.h"
#include "Camera.h"
#include "global.h"

Universe::Universe(void)
{
	player = NULL;
}

Universe::~Universe(void)
{
	if (player) delete player;
	player = NULL;
}

int Universe::init(char *errorString)
{
	player = new PlayerShip();
	if (!player)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Out of memory");
		return ERROR_OUT_OF_MEMORY;
	}
	player->setPos(LargeInt(182000), LargeInt(525000));
	playerGalaxyID = 0;
	playerStarID = 0;

	// Allocate memory for galaxies in the universe
	numGalaxies = 0;
	numAllocatedGalaxies = U_NUM_INITIAL_GALAXIES;
	galaxyList = new Galaxy[numAllocatedGalaxies];
	if (!galaxyList)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Out of memory");
		return ERROR_OUT_OF_MEMORY;
	}

	// Create a single Galaxy
	Location galaxyLocation;
	galaxyLocation.setPos(LargeInt(182000), LargeInt(525000));
	galaxyLocation.setRot(0.0f);
	galaxyLocation.setSpeed(0.0f, 0.0f);
	galaxyLocation.setRotSpeed(0.0f);
	int returnValue = addGalaxy(&galaxyLocation, errorString);
	if (returnValue != 0)
	{
		return returnValue;
	}

	return 0;
}

int Universe::addGalaxy(Location *loc, char *errorString)
{
	if (numGalaxies == numAllocatedGalaxies)
	{
		// Have to reallocate
		Galaxy *newList = new Galaxy[numAllocatedGalaxies * 2];
		if (!newList)
		{
			sprintf_s(errorString, MAX_ERROR_LENGTH, "Out of memory");
			return ERROR_OUT_OF_MEMORY;
		}
		// Copy old list to new list
		for (int i = 0; i < numAllocatedGalaxies; i++)
		{
			newList[i] = galaxyList[i];
		}
		numAllocatedGalaxies *= 2;
		delete [] galaxyList;
		galaxyList = newList;
	}

	numGalaxies++;
	return galaxyList[numGalaxies-1].init(*loc, errorString);
}

void Universe::timeStep(void)
{
	player->timeStep();
}

int Universe::draw(GLGraphics *renderer, char *errorString)
{
	int returnVal = 0;
	Camera camera;
	camera.setPos(player->getXPos(), player->getYPos());
	camera.setRot(player->getRotation());

	// Render the starfield...
	if (0 != renderer->setTexture(U_SMALL_STAR_TEXTURE, errorString))
	{
		return -1;
	}

	renderer->beginRendering();
	float rot = camera.getRotation();
	unsigned int xPosLo = player->getXPos().getLowInt();
	int xPosInner = (int)(xPosLo & (U_SMALL_STAR_STEP_SIZE - 1));
	xPosLo &= -U_SMALL_STAR_STEP_SIZE;
	unsigned int yPosLo = (int)player->getYPos().getLowInt();
	int yPosInner = (int)(yPosLo & (U_SMALL_STAR_STEP_SIZE - 1));
	yPosLo &= -U_SMALL_STAR_STEP_SIZE;
	for (int starY = -U_SMALL_STAR_RADIUS * U_SMALL_STAR_STEP_SIZE;
		 starY <= +U_SMALL_STAR_RADIUS * U_SMALL_STAR_STEP_SIZE;
		 starY += U_SMALL_STAR_STEP_SIZE)
	{
		for (int starX = -U_SMALL_STAR_RADIUS * U_SMALL_STAR_STEP_SIZE;
			 starX <= U_SMALL_STAR_RADIUS * U_SMALL_STAR_STEP_SIZE;
			 starX += U_SMALL_STAR_STEP_SIZE)
		{
			float xPos = (float)(starX - xPosInner);
			float yPos = (float)(starY - yPosInner);
			xPos += U_SMALL_STAR_SPAWN_RADIUS * sin((xPosLo + starX) * 103.0f + 3897.23f * cos((yPosLo + starY) * 723.0f));
			yPos += U_SMALL_STAR_SPAWN_RADIUS * sin((yPosLo + starY) * 2349.0f + 1203.70f * cos((xPosLo + starX) * 2389.0f));
			float xp = cos(rot) * xPos + sin(rot) * yPos;
			float yp = -sin(rot) * xPos + cos(rot) * yPos;
			float zoom = camera.getZoomFactor() * (0.75f + 0.25f * sin((yPosLo + starY) * 34567.0f + 1231.70f * cos((xPosLo + starX) * 235267.0f)));
			renderer->drawSprite(0, 0, xp * zoom, yp * zoom,
					50.0f * camera.getZoomFactor(), 50.0f * camera.getZoomFactor(), 0.0f);
		}
	}
	renderer->endRendering();

	// Draw the galaxies, stars and planets
	returnVal = galaxyList[playerGalaxyID].draw(renderer, &camera, playerStarID, errorString);
	if (returnVal != 0) return returnVal;

	returnVal = player->draw(renderer, &camera, errorString);
	if (returnVal != 0) return returnVal;

	return returnVal;
}