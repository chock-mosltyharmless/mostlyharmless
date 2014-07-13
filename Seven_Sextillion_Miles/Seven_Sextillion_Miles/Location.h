#pragma once

#include "LargeInt.h"

class Location
{
public:
	Location(void);
	virtual ~Location(void);

private:
	// In base units (mile/1024)
	LargeInt posX, posY;
	float rotation; // Current rotation in rad

	// Relative to one time tick
	float speedX, speedY;
	float rotationSpeed;

	// Relative to one time tick
	float acceleration; // pointing towards the direction of the moving object
	float rotationAcceleration;
};

