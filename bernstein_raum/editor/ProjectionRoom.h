#pragma once

#include "stdafx.h"

// Simplified room, just my Flur, 4 walls

struct RoomEdge
{
    // Call this every time you need 3d coordinates
    void Update(void)
    {
        location[0] = coord[0] * coord[2] * 16.0f / 9.0f;  // Hack for 16:9 displays
        location[1] = coord[1] * coord[2];
        location[2] = coord[2];
        gl_coord[0] = coord[0];
        gl_coord[1] = coord[1];
        gl_coord[2] = 1.0f / coord[2];
    }

public:
    // screen_x, screen_y and distance
    float coord[3];
    // real 3d coordinates
    float location[3];
    // coordinates for openGL (screen_x, screen_y, 1.0f / distance)
    float gl_coord[3];
};

class ProjectionRoom
{
public:
    ProjectionRoom();
    ~ProjectionRoom();

    void ImGUIControl(void);
    void RenderAll(void);

private:  // Functions
    // Calculates a normal based on three points
    void GetNormal(const float *a, const float *b, const float *c, float *normal);

private: // Data
    RoomEdge edge_[8];  // Front 4, back 4, counterclockwise starting top left
};

