#pragma once

#include "resource.h"
#include "TextureManager.h"
#include "Configuration.h"

class Audio;
class Smartphones;

extern HDC mainDC;
extern HGLRC mainRC;
extern HWND mainWnd;
extern TextureManager textureManager;
extern Audio audio_;
extern Smartphones smartphones_;

void DrawQuadColor(float startX, float endX, float startY, float endY,
    float startU, float endU, float startV, float endV,
    float red, float green, float blue, float alpha);
void DrawQuadColor(float startX, float endX, float startY, float endY,
    float red, float green, float blue, float alpha);
void DrawQuad(float startX, float endX, float startY, float endY, float startU, float endU, float startV, float endV, float alpha);
void DrawQuad(float startX, float endX, float startY, float endY, float startV, float endV, float alpha);
void DrawQuad(float startX, float endX, float startY, float endY, float alpha);
void DrawBlackFade(float startX, float endX, float startY, float endY);