#pragma once

#include "GLGraphics.h"
#include "Galaxy.h"
#include "PlayerShip.h"

// Number of initially allocated memory for galaxies
#define U_NUM_INITIAL_GALAXIES 64

// These defines are for rendering the star background
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

	// Returns 0 on success
	int init(char *errorString);

	int draw(GLGraphics *renderer, char *errorString);
	void timeStep(void);

	/// Get the player ship so that player input can be handled
	PlayerShip *getPlayerShip() { return player; }

private:
	// Returns 0 on success
	int addGalaxy(Location *loc, char *errorString);

private:
	// Array of Galaxies (resizable)
	int numGalaxies;
	int numAllocatedGalaxies;
	Galaxy *galaxyList;

	// The one player ship
	PlayerShip *player;
	/// IDs of the galaxy and star that the ship is currently in.
	int playerGalaxyID, playerStarID;
};

