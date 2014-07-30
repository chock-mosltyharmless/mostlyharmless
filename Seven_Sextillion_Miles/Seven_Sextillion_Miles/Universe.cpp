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

	returnVal = player->draw(renderer, &camera, errorString);
	if (returnVal != 0) return returnVal;

	return returnVal;
}