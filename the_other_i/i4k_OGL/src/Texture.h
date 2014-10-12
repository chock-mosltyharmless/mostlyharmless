#pragma once
class Texture
{
public:
	Texture(void);
	~Texture(void);
	
	HRESULT init(const char *tgaFileName);
	void deInit();
	void setTexture();
	void drawScreenAlignedQuad(float startX = -1.0f, float startY = -1.0f,
							   float endX = 1.0f, float endY = 1.0f);

private:
	static int nextPowerOfTwo(int value);
	int textureWidth;  // real width in opengl (with padding)
	int textureHeight; // real height in opengl (with padding)
	int imageWidth;
	int imageHeight;
	GLuint textureGLID;
};

