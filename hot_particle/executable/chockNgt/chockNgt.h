#pragma once

#include "resource.h"
#include "TextureManager.h"
#include "Smartphones.h"

extern HDC mainDC;
extern HGLRC mainRC;
extern HWND mainWnd;
extern TextureManager textureManager;
extern Smartphones smartphones_;

void DrawQuadColor(float startX, float endX, float startY, float endY,
    float startU, float endU, float startV, float endV,
    float red, float green, float blue, float alpha);
void DrawQuadColor(float startX, float endX, float startY, float endY,
    float red, float green, float blue, float alpha);
void DrawQuad(float startX, float endX, float startY, float endY, float startU, float endU, float startV, float endV, float alpha);
void DrawQuad(float startX, float endX, float startY, float endY, float startV, float endV, float alpha);
void DrawQuad(float startX, float endX, float startY, float endY, float alpha);
