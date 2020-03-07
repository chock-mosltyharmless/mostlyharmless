#pragma once

#include "stdafx.h"

// Simplified room, just my Flur, 4 walls

struct RoomEdge
{
public:
    float coord[3];
};

class ProjectionRoom
{
public:
    ProjectionRoom();
    ~ProjectionRoom();

    void ImGUIControl(void);

private: // Data
    RoomEdge edge_[8];  // Front 4, back 4, counterclockwise starting top right
};

