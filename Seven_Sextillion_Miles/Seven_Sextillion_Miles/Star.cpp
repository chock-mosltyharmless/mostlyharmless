#include "StdAfx.h"
#include "Star.h"
#include "global.h"

Star::Star(void)
{
	planetList = NULL;
}

Star::~Star(void)
{
	if (planetList) delete [] planetList;
}

int Star::init(Location loc, char *errorString)
{
	sunLocation = loc;
	
	// Should be loaded from a data file
	sunTextureName = "sun1.png"; 
	sunRadius = 10000.0f;
	
	// Initialize planets

	return 0;
}

int Star::draw(GLGraphics *renderer, Camera *camera, char *errorString)
{
	// Draw the central sun
	if (0 != renderer->setTexture(sunTextureName, errorString))
	{
		return -1;
	}

	// Transform to camera location
	LargeInt xp = sunLocation.getXPos();
	LargeInt yp = sunLocation.getYPos();
	float rot = sunLocation.getRotation() - camera->getRotation();
	float transX, transY, transRot; // transformed coordinates
	camera->transform(xp, yp, rot, &transX, &transY, &transRot);
	float zoomFactor = camera->getZoomFactor();

	// draw relative to center
	renderer->beginRendering();
	renderer->drawSprite(0, 0, transX, transY,
						 sunRadius * zoomFactor, sunRadius * zoomFactor, rot);
	renderer->endRendering();

	return 0;
}