#include "StdAfx.h"
#include "PlayerShip.h"
#include "global.h"

PlayerShip::PlayerShip(void)
{
	// On initialization, set the player to the center of the univers
	loc.setPos(LargeInt(0), LargeInt(0));
	loc.setSpeed(0.0f, 0.0f);
	loc.setAcceleration(0.0f);
	loc.setRot(0.0f);
	loc.setRotSpeed(0.0f);
	loc.setRotAcc(0.0f);
}


PlayerShip::~PlayerShip(void)
{
}

int PlayerShip::draw(GLGraphics *renderer, Camera *camera, char *errorString)
{
	if (0 != renderer->setTexture(PS_SHIP_TEXTURE, errorString))
	{
		return -1;
	}

	// Transform to camera location
	LargeInt xp = loc.getXPos();
	LargeInt yp = loc.getYPos();
	float rot = loc.getRotation();
	float transX, transY, transRot; // transformed coordinates
	camera->transform(xp, yp, rot, &transX, &transY, &transRot);
	float zoomFactor = camera->getZoomFactor();

	// draw relative to center
	renderer->beginQuadRendering();
	renderer->drawSprite(0, 0, transX, transY, 1000.0f * zoomFactor, 1000.0f * zoomFactor, transRot);
	renderer->endRendering();

	return 0;
}

void PlayerShip::timeStep(void)
{
	loc.timeStep();
}