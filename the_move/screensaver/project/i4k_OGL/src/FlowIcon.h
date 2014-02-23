#pragma once

class FlowIcon
{
public:
	FlowIcon(void);
	~FlowIcon(void);

	void init(const char *texName, int xpos, int ypos);
	void draw(float time);
	void drawAlarming(float time);
	void setMousePosition(float xpos, float ypos);
	bool clickMouse();

	float getGLX();
	float getGLY();

private:
	int posX, posY;
	const char *texName;
	const static float borderWidth;
	const static float distance;

	// Control for wether it is clicked
	float curTime;
	float mouseX;
	float mouseY;
	float clickTime; // Set in the past to show normal
	float mouseOverAmount;
	float lastDrawTime;
	bool mouseIsOver;
};

