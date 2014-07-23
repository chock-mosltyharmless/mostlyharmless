#include "StdAfx.h"
#include "Universe.h"
#include "Camera.h"


Universe::Universe(void)
{
	player = NULL;
}

Universe::~Universe(void)
{
	if (player) delete player;
	player = NULL;
}

void Universe::init(void)
{
	player = new PlayerShip(); // TODO: may crash?
	player->setPos(LargeInt(182000), LargeInt(525000));
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

	// Render the starfield...

	returnVal = player->draw(renderer, &camera, errorString);
	if (returnVal != 0) return returnVal;

	return returnVal;
}