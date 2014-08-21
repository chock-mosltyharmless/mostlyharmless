#pragma once

#include "GLGraphics.h"

// 10 miles grid spacing
#define SB_GRID_SPACING 1000.0f

// The number of different colors a space material can have
// This number is used to shade material weights differently
#define SB_MATERIAL_NUM_COLORS 2
// The distance of weights that is used to choose a different material color
#define SB_MATERIAL_SHADE_STEP_SIZE 1.0f

// The matrial of a SpaceObject. This is used in each grid point
// This is used as a singleton.
class SpaceBodyMaterialList
{
public:
	SpaceBodyMaterialList();
	virtual ~SpaceBodyMaterialList();

	// returns 0 on success.
	int init(char *errorString);

	// color is the return value (4 floats)
	void getColor(int material, float weight, float *color) {
		if (material < 0 || material >= numMaterials) { color[0] = 1.0f; color[1] = 0.5f; color[2] = 0.5f; color[3] = 1.0f; return; }
		if (weight < 0.0f) weight = 0.0f;
		if (weight > SB_MATERIAL_SHADE_STEP_SIZE * SB_MATERIAL_NUM_COLORS) weight = SB_MATERIAL_SHADE_STEP_SIZE * SB_MATERIAL_NUM_COLORS;
	    int index = (int)(weight / SB_MATERIAL_SHADE_STEP_SIZE);
		float amount = weight / SB_MATERIAL_SHADE_STEP_SIZE - (float)index;
		for (int i = 0; i < 3; i++)
		{
			float c1 = colorBuffer[material][index][i];
			float c2 = colorBuffer[material][index + 1][i];
			color[i] = (1.0f - amount) * c1 + amount * c2;
		}
		color[3] = 1.0f; // alpha
	}

private:
	int numMaterials;
	// The last material is doubled for easier access
	float (*colorBuffer)[SB_MATERIAL_NUM_COLORS + 1][3];
};

// One grid point of a space body.
class SpaceBodyGridPoint
{
public:
	// Each grid point consists of one material (this is not a resource).
	// Material 0 is void (empty space), the other integers are an enumeration
	// of materials
	int materialID;

	// The amount of presence. For fluids that can be the depth of the water.
	// I am not quite sure yet what this means for non-fluids...
	// Probably it is the amount/density relative to the neighboring stuff.
	float weight;
};

// A space body is the physical representation of a planet, sun, asteroid, ...
// This includes the resources that can be mined from it.
// The class is also responsible for rendering.
// A space body has properties on a square grid. The rendering is done
// using some modified marching squares.
class SpaceBody
{
public:
	SpaceBody(void);
	virtual ~SpaceBody(void);

	// returns 0 on success
	int createSun(int size, unsigned int seed, char *errorString);

	/// Returns 0 if successful.
	int draw(GLGraphics *renderer, float x, float y, float rotation, float zoom, char *errorString);	

	float getGridWeight(int xIdx, int yIdx) {
		return grid[yIdx * size + xIdx].weight;
	}

	int getGridMaterialID(int xIdx, int yIdx) {
		return grid[yIdx * size + xIdx].materialID;
	}

private:
	// Used as a singleton
	static SpaceBodyMaterialList materialList;

	// This is the number of elements in the grid.
	int size;
	// Linearized grid
	SpaceBodyGridPoint *grid;
};

