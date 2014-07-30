#pragma once

#include "Location.h"
#include "LargeInt.h"

#define CAM_DEF_ZOOM_FACTOR 0.0001f

class Camera
{
public:
	Camera(void);
	virtual ~Camera(void);

	void setPos(LargeInt x, LargeInt y) { loc.setPos(x, y); }
	void setRot(float r) { loc.setRot(r); }
	float getRotation(void) { return loc.getRotation(); }
	float getZoomFactor(void) { return zoomFactor; }

	void transform(LargeInt x, LargeInt y, float rot,
		           float *transX, float *transY, float *transRot);	

private:
	/// The camera has a location, but acceleration and speed are always 0?
	Location loc;

	/// Inverse in the sense that 10x zoom means 0.1f
	float zoomFactor;
};

