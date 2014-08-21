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
	//sunTextureName = "sun1.png"; 
	//sunRadius = 10000.0f;
	int retVal = body.createSun(20, 0x34958673, errorString);
	if (retVal != 0) return retVal;

	// Initialize planets

	return 0;
}

int Star::draw(GLGraphics *renderer, Camera *camera, char *errorString)
{
	int retVal;

	// Draw the central sun
#if 0
	if (0 != renderer->setTexture(sunTextureName, errorString))
	{
		return -1;
	}
#endif

	// Transform to camera location
	LargeInt xp = sunLocation.getXPos();
	LargeInt yp = sunLocation.getYPos();
	float rot = sunLocation.getRotation();
	float transX, transY, transRot; // transformed coordinates
	camera->transform(xp, yp, rot, &transX, &transY, &transRot);
	float zoomFactor = camera->getZoomFactor();

	// draw relative to center
#if 0
	renderer->beginQuadRendering();
	renderer->drawSprite(0, 0, transX, transY,
						 sunRadius * zoomFactor, sunRadius * zoomFactor, rot);
	renderer->endRendering();
#endif

	retVal = body.draw(renderer, transX, transY, transRot, zoomFactor, errorString);
	if (retVal != 0) return retVal;

	return 0;
}