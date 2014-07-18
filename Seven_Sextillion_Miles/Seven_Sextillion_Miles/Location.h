#pragma once

#include "LargeInt.h"

// Multiplier of the speed on each time step
#define LOC_SPEED_DILATION 1.0f
#define LOC_ROT_SPEED_DILATION 0.99f
// Multiplier of the acceleration at each time step (acceleration gone immediately?)
#define LOC_ACC_DILATION 0.0f
#define LOC_ROT_ACC_DILATION 0.0f

class Location
{
public:
	Location(void);
	virtual ~Location(void);

	void setPos(LargeInt x, LargeInt y) {posX = x; posY = y;}
	void setRot(float r) {rotation = r;}
	void setSpeed(float x, float y) {speedX = x; speedY = y;}
	void setRotSpeed(float r) {rotationSpeed = r;}
	void setAcceleration(float a) {acceleration = a;}
	void setRotAcc(float a) {rotationAcceleration = a;}

	LargeInt getXPos(void) { return posX; }
	LargeInt getYPos(void) { return posY; }
	float getRotation(void) { return rotation; }

	// Do one time interval
	void timeStep(void)
	{
		// update position
		LargeInt xSpd(speedX);
		LargeInt ySpd(speedY);
		posX.add(&xSpd);
		posY.add(&ySpd);
		rotation += rotationSpeed;
		
		// update speed depending on current acceleration
		speedX += -sin(rotation) * acceleration;
		speedY += cos(rotation) * acceleration;
		speedX *= LOC_SPEED_DILATION;
		speedY *= LOC_SPEED_DILATION;
		rotationSpeed += rotationAcceleration;
		rotationSpeed *= LOC_ROT_SPEED_DILATION;

		// dilate acceleration
		acceleration *= LOC_ACC_DILATION;
		rotationAcceleration *= LOC_ROT_ACC_DILATION;
	}

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

