#pragma once

class FlowIcon
{
public:
	FlowIcon(void);
	~FlowIcon(void);

	// Distance is 2*width + 2*borderWidth
	void init(const char *texName, float xpos, float ypos, float distance, float borderWidth);
	void draw(float time);
	void drawAmount(float mouseOverAmount, float time, float xDelta = 0.0f, float yDelta = 0.0f);
	void drawAlarming(float time);
	void drawSubCategory(float time);
	void setMousePosition(float xpos, float ypos);
	bool clickMouse();

	float getGLX();
	float getGLY();

private:
	float posX, posY;
	const char *texName;
	float borderWidth;
	float distance;

	// Control for wether it is clicked
	float curTime;
	float mouseX;
	float mouseY;
	float clickTime; // Set in the past to show normal
	float mouseOverAmount;
	float lastDrawTime;
	bool mouseIsOver;
};

