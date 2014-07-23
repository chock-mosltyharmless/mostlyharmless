#pragma once

#include "GLGraphics.h"
#include "Camera.h"

class Galaxy
{
public:
	Galaxy(void);
	virtual ~Galaxy(void);

	int draw(GLGraphics *renderer, Camera *camera, char *errorString);
};

