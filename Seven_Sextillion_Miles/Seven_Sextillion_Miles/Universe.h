#pragma once

#include "GLGraphics.h"
#include "Galaxy.h"
#include "PlayerShip.h"

class Universe
{
public:
	Universe(void);
	virtual ~Universe(void);

	void init(void);

	int draw(GLGraphics *renderer, char *errorString);
	void timeStep(void);

	/// Get the player ship so that player input can be handled
	PlayerShip *getPlayerShip() { return player; }

private:
	// Array of Galaxies (resizable)
	int numGalaxies;
	int numAllocatedGalaxies;
	Galaxy *galaxy;

	// The one player ship
	PlayerShip *player;
};

