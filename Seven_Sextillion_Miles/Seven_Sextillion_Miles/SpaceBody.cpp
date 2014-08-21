#include "StdAfx.h"
#include "SpaceBody.h"
#include "global.h"

SpaceBodyMaterialList SpaceBody::materialList;

SpaceBodyMaterialList::SpaceBodyMaterialList(void)
{
	colorBuffer = NULL;
}

SpaceBodyMaterialList::~SpaceBodyMaterialList(void)
{
	if (colorBuffer) delete [] colorBuffer;
}

int SpaceBodyMaterialList::init(char *errorString)
{
	// Only initialize the first time
	if (colorBuffer) return 0;

	// This would actually load from file. For now it is hardcoded
	numMaterials = 4;
	colorBuffer = new float[numMaterials][SB_MATERIAL_NUM_COLORS + 1][3];
	if (!colorBuffer)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Out of memory");
		return ERROR_OUT_OF_MEMORY;
	}

	// void
	colorBuffer[0][0][0] = 0.0f;
	colorBuffer[0][0][1] = 0.0f;
	colorBuffer[0][0][2] = 0.0f;
	colorBuffer[0][1][0] = 0.0f;
	colorBuffer[0][1][1] = 0.0f;
	colorBuffer[0][1][2] = 0.0f;
	// Red (cold) Star material
	colorBuffer[1][0][0] = 0.7f;
	colorBuffer[1][0][1] = 0.2f;
	colorBuffer[1][0][2] = 0.0f;
	colorBuffer[1][1][0] = 0.75f;
	colorBuffer[1][1][1] = 0.3f;
	colorBuffer[1][1][2] = 0.0f;
	// Orange (middle) star material
	colorBuffer[2][0][0] = 0.8f;
	colorBuffer[2][0][1] = 0.5f;
	colorBuffer[2][0][2] = 0.1f;
	colorBuffer[2][1][0] = 0.8f;
	colorBuffer[2][1][1] = 0.6f;
	colorBuffer[2][1][2] = 0.15f;
	// Yellow (hot) star material
	colorBuffer[3][0][0] = 0.9f;
	colorBuffer[3][0][1] = 0.9f;
	colorBuffer[3][0][2] = 0.4f;
	colorBuffer[3][1][0] = 0.9f;
	colorBuffer[3][1][1] = 0.9f;
	colorBuffer[3][1][2] = 0.5f;

	// Copy last color
	for (int mat = 0; mat < numMaterials; mat++)
	{
		for (int c = 0; c < 3; c++)
		{
			colorBuffer[mat][SB_MATERIAL_NUM_COLORS][c] =
				colorBuffer[mat][SB_MATERIAL_NUM_COLORS - 1][c];
		}
	}
	return 0;
}

SpaceBody::SpaceBody(void)
{
}

SpaceBody::~SpaceBody(void)
{
}

int SpaceBody::createSun(int size, unsigned int seed, char *errorString)
{
	int res = materialList.init(errorString);
	if (res != 0) return res;

	this->size = size;
	grid = new SpaceBodyGridPoint[size * size];
	if (!grid)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Out of memory");
		return ERROR_OUT_OF_MEMORY;
	}

	for (int y = 0; y < size; y++)
	{
		for (int x = 0; x < size; x++)
		{
			seed = blr_rand(seed);
			float xp = x - size * 0.5f + 0.5f;
			float yp = y - size * 0.5f + 0.5f;
			if (xp * xp + yp * yp > (size-2) * (size-2) * 0.25f) grid[y * size + x].materialID = 0;
			else grid[y * size + x].materialID = (seed >> 16) % 3 + 1;
			grid[y * size + x].weight = ((seed >> 8) % 65536) * (1.0f / 32768.0f);
		}
	}
	
	return 0;
}

int SpaceBody::draw(GLGraphics *renderer, float x, float y, float rotation, float zoom, char *errorString)
{
	renderer->disableTexturing();
	renderer->beginTriRendering();

	float dxX = zoom * SB_GRID_SPACING * cos(rotation); // x move along x index
	float dyX = zoom * SB_GRID_SPACING * sin(rotation); // y move along x index
	float dxY = -zoom * SB_GRID_SPACING * sin(rotation); // x move along y index
	float dyY = zoom * SB_GRID_SPACING * cos(rotation); // y move along x index
	for (int yIdx = 0; yIdx < size - 1; yIdx++)
	{
		for (int xIdx = 0; xIdx < size - 1; xIdx++)
		{
			float edgePos[3][3][2]; // top-to-bottom, left-to-right
			int edgeMaterialID[2][2];
			float edgeWeight[2][2];
			float centerX = x + (xIdx - size * 0.5f + 0.5f) * dxX + (yIdx - size * 0.5f + 0.5f) * dxY;
			float centerY = y + (xIdx - size * 0.5f + 0.5f) * dyX + (yIdx - size * 0.5f + 0.5f) * dyY;

			for (int yi = -1; yi <= 1; yi++)
			{
				for (int xi = -1; xi <= 1; xi++)
				{
					edgePos[yi+1][xi+1][0] = centerX + 0.5f * xi * dxX + 0.5f * yi * dxY;
					edgePos[yi+1][xi+1][1] = centerY + 0.5f * xi * dyX + 0.5f * yi * dyY;
				}
			}
			for (int yi = 0; yi < 2; yi++)
			{
				for (int xi = 0; xi < 2; xi++)
				{
					edgeMaterialID[yi][xi] = getGridMaterialID(xIdx + xi, yIdx + yi);
					edgeWeight[yi][xi] = getGridWeight(xIdx + xi, yIdx + yi);
				}
			}

			// Top-left quad
			float pointWeight[2][2];
			float pointColor[2][2][4];
			
			pointWeight[0][0] = getGridWeight(xIdx, yIdx);
			pointWeight[0][1] = 0.5f * (edgeWeight[0][0] + edgeWeight[0][1]);
			if (edgeMaterialID[0][0] != edgeMaterialID[1][0]) pointWeight[0][1] = 0.0f;
			pointWeight[1][0] = 0.5f * (edgeWeight[0][0] + edgeWeight[1][0]);
			if (edgeMaterialID[0][0] != edgeMaterialID[0][1]) pointWeight[1][0] = 0.0f;
			pointWeight[1][1] = 0.0f;
			if (edgeMaterialID[0][0] == edgeMaterialID[0][1] == edgeMaterialID[1][0] == edgeMaterialID[1][1])
			{
				pointWeight[1][1] = 0.25f * (edgeWeight[0][0] + edgeWeight[0][1] + edgeWeight[1][0] + edgeWeight[1][1]);
			}
			for (int yi = 0; yi < 2; yi++)
			{
				for (int xi = 0; xi < 2; xi++)
				{
					materialList.getColor(edgeMaterialID[0][0], pointWeight[yi][xi], pointColor[yi][xi]);
				}
			}
			renderer->drawColorTri(edgePos[0][0], edgePos[0][1], edgePos[1][1], pointColor[0][0], pointColor[0][1], pointColor[1][1]);
			renderer->drawColorTri(edgePos[0][0], edgePos[1][0], edgePos[1][1], pointColor[0][0], pointColor[1][0], pointColor[1][1]);
		}
	}

	renderer->endRendering();

	return 0;
}