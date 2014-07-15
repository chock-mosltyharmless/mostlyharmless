#pragma once

#include "Location.h"
#include "GLGraphics.h"
#include "Camera.h"

#define PS_SHIP_TEXTURE "arrow.png"

/**
 * Location and status in the world map of the player ship.
 * This also includes what the ship looks like, what it carries and so on.
 */
class PlayerShip
{
public:
	PlayerShip(void);
	virtual ~PlayerShip(void);

	/// These setters should not usually be called unless on init
	void setPos(LargeInt x, LargeInt y) { loc.setPos(x, y); }
	void setRot(float r) { loc.setRot(r); }
	void setSpeed(float x, float y) { loc.setSpeed(x, y); }
	void setRotSpeed(float s) { loc.setRotSpeed(s); }

	/// Set the acceleration at every frame before doing physics
	void setAcceleration(float a) { loc.setAcceleration(a); }
	/// Set the rotation acceleration at every frame before doing physics
	void setRotAcc(float a) { loc.setRotAcc(a); }

	/// Returns 0 if successful.
	int draw(GLGraphics *renderer, Camera *camera, char *errorString);

	/// Perform one step of physics
	void timeStep(void);

private:
	Location loc;
};

