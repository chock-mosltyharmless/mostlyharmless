#include "StdAfx.h"
#include "Camera.h"


Camera::Camera(void)
{
	// On initialization, set the camera to the center of the univers
	loc.setPos(LargeInt(0), LargeInt(0));
	loc.setSpeed(0.0f, 0.0f);
	loc.setAcceleration(0.0f);
	loc.setRot(0.0f);
	loc.setRotSpeed(0.0f);
	loc.setRotAcc(0.0f);
	zoomFactor = CAM_DEF_ZOOM_FACTOR;
}

Camera::~Camera(void)
{
}

void Camera::transform(LargeInt x, LargeInt y, float rot,
		               float *transX, float *transY, float *transRot)
{
	*transRot = rot - loc.getRotation();
	x.sub(&loc.getXPos());
	y.sub(&loc.getYPos());

	*transX = x.getFloat() * zoomFactor;
	*transY = y.getFloat() * zoomFactor;
}

