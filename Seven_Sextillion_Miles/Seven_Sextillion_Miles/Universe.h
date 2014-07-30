#pragma once

#include "GLGraphics.h"
#include "Galaxy.h"
#include "PlayerShip.h"

#define U_SMALL_STAR_TEXTURE "SmallStar.png"
#define U_SMALL_STAR_SPAWN_RADIUS 8192.0f
#define U_SMALL_STAR_STEP_SIZE 2048
// How far to left/right I go...
#define U_SMALL_STAR_RADIUS 15

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

