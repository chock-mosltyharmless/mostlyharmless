#pragma once

#include "GLGraphics.h"

class Planet
{
public:
	Planet(void);
	virtual ~Planet(void);

	int draw(GLGraphics *renderer, char *errorString);
};

