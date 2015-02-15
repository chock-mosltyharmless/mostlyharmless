#pragma once

#define NUM_SNIPPETS 256

class Snippet
{
public:
	float pos[3]; // But we simply ignore Z
	float speed[3]; // Movement speed, should be 0 along normal
	float rotation[4]; // Rotation stored in a quaternion
};

class Snippets
{
public:
	Snippets(void);
	virtual ~Snippets(void);

	// Put everything to initial positions before falling...
	void init(void);

	// Do movement update of all snippets (For simplicity I do not have const time)
	void update(float dTime);
	
	// Draws all snippets (assuming ortho projection)
	void draw(void);

private:
	Snippet snippet[NUM_SNIPPETS];
};

