#include "Fractal.h"
#include "intro.h"
#include "mzk.h"

Fractal::Fractal() {
}

Fractal::~Fractal() {
}

void Fractal::Multiply(pos (*dest)[6][7], pos (*a)[6][7], pos (*b)[6][7]) {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            pos sum = 0.0f;
            for (int k = 0; k < 6; k++) {
                sum += (*a)[i][k] * (*b)[k][j];
            }
            (*dest)[i][j] = sum;
        }

        // bias (column 7):
        pos sum = (*a)[i][6];
        for (int k = 0; k < 6; k++) {
            sum += (*a)[i][k] * (*b)[k][6];
        }
        (*dest)[i][6] = sum;
    }
}

pos Fractal::GetObjectSize(pos (*object)[6][7]) {
    // Manhatten: max(3 x 3)
    // Sphere^2: max(len^2 x 3)
    // Strange approximation: sum(3x3)^2
    pos size = 0.0f;
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            size += (*object)[y][x] * (*object)[y][x];
        }
    }
    return size;
}

void Fractal::CreateObjects(pos max_size) {
    // Adjust size for quadratic checks
    max_size *= max_size;

    // Create matrices
    unsigned int seed = 132089410;
    for (int i = 0; i < NUM_IFS_FUNCTIONS; i++) {
        for (int a = 0; a < 6; a++) {
            for (int b = 0; b < 7; b++) {
                ifs_function_[i][a][b] = frand(&seed) - 0.5f;
            }
        }
    }
    
    // Create initial object
    num_objects_ = 1;
    is_visible_[0] = true;
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 7; j++) {
            if (i == j) {
                object_location_[0][i][j] = 1.0f;
            } else {
                object_location_[0][i][j] = 0.0f;
            }
        }
    }

    for (int current_object = 0; current_object < num_objects_; current_object++) {
        if (GetObjectSize(&(object_location_[current_object])) > max_size) {
            // Split object
            if (num_objects_ + NUM_IFS_FUNCTIONS < MAX_IFS_OBJECTS) {
                is_visible_[current_object] = false;
                for (int ifs_id = 0; ifs_id < NUM_IFS_FUNCTIONS; ifs_id++) {
                    Multiply(&(object_location_[num_objects_]),
                             &(ifs_function_[ifs_id]),     
                             &(object_location_[current_object]));
                    is_visible_[num_objects_] = true;
                    num_objects_++;
                }
            }
        }
    }

    // Set vertex location for triangles
    // One triangle per object, each edge extends by 1 in each 3D-dimension of space
    num_triangles_ = 0;
    for (int i = 0; i < num_objects_; i++) {
        if (is_visible_[i]) {
            for (int edge = 0; edge < 3; edge++) {
                for (int dim = 0; dim < 3; dim++) {
                    vertices_[num_triangles_][edge][dim] =
                        object_location_[i][dim][6] + object_location_[i][dim][edge];
                    colors_[num_triangles_][edge][dim] =
                        object_location_[i][dim + 3][6] + object_location_[i][dim + 3][edge];
                }
                colors_[num_triangles_][edge][3] = 1.0f;
            }
            num_triangles_++;
        }
    }

    // NOTE: Thus I can only call it once, because I create a new vertex buffer array each time!
    // Set up vertex buffer and stuff
    glGenVertexArrays(1, &vao_id_); // Create our Vertex Array Object  
    glBindVertexArray(vao_id_); // Bind our Vertex Array Object so we can use it  

    // Ignore maximum number of attributes... Why?
    int maxAttrt;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrt);

    glGenBuffers(2, vbo_id_); // Generate our Vertex Buffer Object  

    // Vertex array position data
    // NOTE: With static draw I cannot change it!
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id_[0]); // Bind our Vertex Buffer Object  
    glBufferData(GL_ARRAY_BUFFER, num_triangles_ * 3 * 3 * sizeof(GLfloat),
        vertices_, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
    glVertexAttribPointer(0, // attribute
        3, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0); // array buffer offset

                   // Vertex array color data
                   // change to GL_STATIC_DRAW and single update for speed.
                   // Move the GenerateParticles copy operation to here.
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id_[1]); // Bind our Vertex Buffer Object  
    glBufferData(GL_ARRAY_BUFFER, num_triangles_ * 3 * 4 * sizeof(GLfloat),
        colors_, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
    glVertexAttribPointer(1, // attribute
        4, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0); // array buffer offset
}

void Fractal::Draw(void) {
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Un-bind buffer
    //glEnableVertexAttribArray(0);
    //glBindBuffer(GL_ARRAY_BUFFER, vbo_id_[0]); // Bind our Vertex Buffer Object
    //glEnableVertexAttribArray(1);
    //glBindBuffer(GL_ARRAY_BUFFER, vbo_id_[1]); // Bind our Vertex Buffer Object
    glDrawArrays(GL_TRIANGLES, 0, num_triangles_ * 3);
}