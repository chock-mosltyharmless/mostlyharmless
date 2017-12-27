#pragma once

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"

#define NUM_IFS_FUNCTIONS 4
#define MAX_IFS_OBJECTS 1000000

typedef float pos;

class Fractal
{
public:
    Fractal();
    ~Fractal();

    // Independent of camera location
    // Choose smaller max_size for more higher-resolution objects
    void CreateObjects(pos max_size);

    // Draws the whole thing from current camera location
    void Draw(void);

private:
    // dest = a * b
    // So in my notation: first b, then a
    void Multiply(pos (*dest)[6][7], pos (*a)[6][7], pos (*b)[6][7]);

    // Size independent of camera position
    pos GetObjectSize(pos (*dest)[6][7]);

    pos ifs_function_[NUM_IFS_FUNCTIONS][6][7];

    pos object_location_[MAX_IFS_OBJECTS][6][7];
    bool is_visible_[MAX_IFS_OBJECTS];
    // The number of objects that are active. If is_visible_, render.
    int num_objects_;

    // The vertex array and vertex buffer
    unsigned int vao_id_;
    // 0 is for particle positions, 1 is for particle colors
    unsigned int vbo_id_[2];
    // And the actual vertices (3 dimensions and 3 vectors per triangle)
    GLfloat vertices_[MAX_IFS_OBJECTS][3][3];
    // 4 dimensions and 3 vectors per triangle
    GLfloat colors_[MAX_IFS_OBJECTS][3][4];
    int num_triangles_;  // <= MAX_IFS_OBJECTS due to invisible objects
};

