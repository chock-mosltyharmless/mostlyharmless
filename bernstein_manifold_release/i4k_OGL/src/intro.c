//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"

#include <stdio.h>

#include "config.h"
#include "intro.h"
#include "mzk.h"

// -------------------------------------------------------------------
//                          SHADERS:
// -------------------------------------------------------------------
const GLchar *fragmentMainBackground="\
#version 330 core\n\
in vec3 pass_Position;\n\
out vec4 out_Color;\n\
\n\
void main(void)\n\
{\n\
   out_Color = vec4(pass_Position, 1.0);\n\
}";

const GLchar *vertexMainParticle=
"#version 330 core\n\
layout (location=0) in vec4 position_;\n\
layout (location=1) in vec4 color_;\n\
out vec4 particle_color_;\
out float particle_magic_;\
uniform mat4 r;\
float GetImplicit(vec3 pos) {\
    float time = r[0][0];\
    float first_amount = r[0][1];\
    float second_amount = r[0][2];\
    pos.z -= sin(pos.y * cos(pos.x * 0.4) * 0.3) * second_amount * 5.5;\
    pos.x -= sin(pos.z*0.5) * second_amount * 5.5;\
    pos.y -= sin(pos.x*.25) * second_amount * 5.5;\
    pos.x -= sin(time * cos(pos.x * 2.1) * 0.3) * first_amount * 0.1;\
    pos.y -= cos(time * cos(pos.x * 0.5) * 0.4) * first_amount * 0.1;\
    pos.z -= sin(time * sin(pos.y * 0.2) * 0.5) * first_amount * 0.1;\
    float dist = r[3][1] * 25.0;\
    float dist2 = r[3][2] * 5.0;\
    float dist3 = r[3][3];\
    float implicit = length(pos) - sin(dist * length(pos)) * dist2 - dist3 - 0.2 * sin(time*0.);\
    return implicit;\
}\
void main(void) {\
    float time = r[0][0];\
    float yrot = sin(time*0.15)*0.5;\
    mat2 yrotmat = mat2(cos(yrot),sin(yrot),-sin(yrot),cos(yrot));\
    vec3 pos = position_.xyz;\
    float implicit = GetImplicit(pos.xyz);\
    float amount = 1. + r[1][1]*0.4 - smoothstep(0.0, 0.4, abs(implicit));\
    const float dd = 0.01;\
    float dx_implicit = GetImplicit(pos.xyz + vec3(dd, 0., 0.)) - implicit;\
    float dy_implicit = GetImplicit(pos.xyz + vec3(0., dd, 0.)) - implicit;\
    float dz_implicit = GetImplicit(pos.xyz + vec3(0., 0., dd)) - implicit;\
    vec3 f = pos.xyz * 0.45;\
    f.x += 12. * (r[1][0] * sin(time*0.49) * sin(pos.z*7.) + 1. - r[1][0]) * dx_implicit * amount * r[0][3];\
    f.y += 12. * (r[1][0] * sin(time*0.31) * sin(pos.x*5.) + 1. - r[1][0]) * dy_implicit * amount * r[0][3];\
    f.z += 12. * (r[1][0] * sin(time*0.37) * sin(pos.y*6.) + 1. - r[1][0]) * dz_implicit * amount * r[0][3];\
    f.xz = f.xz * yrotmat;\
    f.x *= 0.56;\
    f.z += 1.5 - 2. * r[2][2];\
    f.x += 0.25 - 0.5 * r[2][3];\
    f.y += 0.25 - 0.5 * r[3][0];\
    gl_Position = vec4(f, f.z);\
    particle_magic_ = color_.a;\
    particle_color_.r = 0.6 + length(pos.xyz) * .3;\
    particle_color_.g = 1.1 - length(pos.xyz) * .4;\
    particle_color_.b = 1.2 - length(pos.xy) * .7 - length(pos.z) * 0.3;\
    particle_color_.rgb *= pow(1. - particle_magic_, 30.) * 3. + 1.0;\
    particle_color_.rgb += pow(abs(sin(particle_magic_*100. + time)),10.);\
    particle_color_.a = 1.0;\
    float darkener = abs(sin(pos.x * r[1][2] * 8.) + cos(pos.z * r[1][3] * 8.) - sin(pos.y * r[2][0] * 8.));\
    particle_color_.a += darkener * r[2][1];\
    particle_color_.rgb -= vec3(darkener) * r[2][1] * 3.0;\
    particle_color_.rgb = max(vec3(0.), particle_color_.rgb);\
    particle_color_ *= vec4(amount) * (1.0 - abs(particle_magic_ - 0.5));\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define NUM_PARTICLES_PER_DIM 128
#define TOTAL_NUM_PARTICLES (NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM)

// Scripting data (loaded from file)
#define kMaxNumScenes 1000
//const int kSceneTic = 4410; // Number of 1/44100 seconds per scene-time-unit?
#define kSceneTic (2 * AUDIO_BUFFER_SIZE)
int num_scenes_ = 0;
int script_[kMaxNumScenes][3]; // Duration, seed, movement

                               // This is only used if SHADER_DEBUG is on, but I will leave it for now.
HWND hWnd;
#ifdef SHADER_DEBUG
char err[4097];
#endif

static GLuint shaderPrograms[2];
// The vertex array and vertex buffer
unsigned int vaoID;
// 0 is for particle positions, 1 is for particle colors
unsigned int vboID[2];
// And the actual vertices
GLfloat vertices_[TOTAL_NUM_PARTICLES * 3];
GLfloat colors_[TOTAL_NUM_PARTICLES * 4];

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

// Multiplices two 4x4 matrices
void matrixMult(float src1[4][4], float src2[4][4], float dest[4][4])
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            dest[i][j] = 0.0f;
            for (int k = 0; k < 4; k++)
            {
                dest[i][j] += src1[i][k] * src2[k][j];
            }
        }
    }
}

// Note that the character data must be allocated.
void LoadTextFile(const char *filename, char *data, int data_size) {
    FILE *fid = fopen(filename, "rb");
    int num_read = fread(data, 1, data_size - 1, fid);
    data[num_read] = 0;
    fclose(fid);
}

// Create the particle locations and move them to the GPU
void GenerateParticles(void) {
    unsigned int seed = 23690984;

    // Set vertex location
    int vertex_id = 0;
    int color_id = 0;
    for (int z = 0; z < NUM_PARTICLES_PER_DIM; z++) {
        float zp = 1.0f - 2.0f * (float)z / (float)NUM_PARTICLES_PER_DIM;
        for (int y = 0; y < NUM_PARTICLES_PER_DIM; y++) {
            float yp = -1.0f + 2.0f * (float)y / (float)NUM_PARTICLES_PER_DIM;
            for (int x = 0; x < NUM_PARTICLES_PER_DIM; x++) {
                float xp = -1.0f + 2.0f * (float)x / (float)NUM_PARTICLES_PER_DIM;
                vertices_[vertex_id++] = xp + 2.0f * jo_frand(&seed) / (float)NUM_PARTICLES_PER_DIM;
                vertices_[vertex_id++] = yp + 2.0f * jo_frand(&seed) / (float)NUM_PARTICLES_PER_DIM;
                vertices_[vertex_id++] = zp + 2.0f * jo_frand(&seed) / (float)NUM_PARTICLES_PER_DIM;
                colors_[color_id++] = 0.9f;
                colors_[color_id++] = 0.7f;
                colors_[color_id++] = 0.5f;
                // fran
                colors_[color_id++] = jo_frand(&seed);
                //colors_[color_id - 1] = 0.5f;
            }
        }
    }
}

void intro_init( void ) {
    // Load the script
    num_scenes_ = 0;
    FILE *fid = fopen("script.txt", "r");
    while (num_scenes_ < kMaxNumScenes &&
        fscanf(fid, "%d %d %d\n", &(script_[num_scenes_][0]),
            &(script_[num_scenes_][1]), &(script_[num_scenes_][2])) > 0) {
        num_scenes_++;
    }
    fclose(fid);

    // Create and link shader and stuff:

    // init objects:
    GLuint vMainParticle = glCreateShader(GL_VERTEX_SHADER);
    GLuint gMainParticle = glCreateShader(GL_GEOMETRY_SHADER_EXT);
    GLuint fMainParticle = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint vHandParticle = glCreateShader(GL_VERTEX_SHADER);
    shaderPrograms[0] = glCreateProgram();
    shaderPrograms[1] = glCreateProgram();
    // compile sources:
#define kMaxShaderLength 100000
    GLchar shader_text[kMaxShaderLength + 1];
    const GLchar *pt = shader_text;
    glShaderSource(vMainParticle, 1, &vertexMainParticle, NULL);
    glCompileShader(vMainParticle);
    LoadTextFile("shaders/vertex_hand_particle.txt", shader_text, kMaxShaderLength);
    glShaderSource(vHandParticle, 1, &pt, NULL);
    glCompileShader(vHandParticle);
    LoadTextFile("shaders/geometry_main_particle.txt", shader_text, kMaxShaderLength);
    glShaderSource(gMainParticle, 1, &pt, NULL);
    glCompileShader(gMainParticle);
    LoadTextFile("shaders/fragment_main_particle.txt", shader_text, kMaxShaderLength);
    glShaderSource(fMainParticle, 1, &pt, NULL);
    glCompileShader(fMainParticle);

#ifdef SHADER_DEBUG
    // Check programs
    int tmp, tmp2;
    glGetShaderiv(vMainParticle, GL_COMPILE_STATUS, &tmp);
    if (!tmp)
    {
        glGetShaderInfoLog(vMainParticle, 4096, &tmp2, err);
        err[tmp2]=0;
        MessageBox(hWnd, err, "vMainParticle shader error", MB_OK);
        return;
    }
    glGetShaderiv(vHandParticle, GL_COMPILE_STATUS, &tmp);
    if (!tmp)
    {
        glGetShaderInfoLog(vHandParticle, 4096, &tmp2, err);
        err[tmp2]=0;
        MessageBox(hWnd, err, "vHandParticle shader error", MB_OK);
        return;
    }
    glGetShaderiv(gMainParticle, GL_COMPILE_STATUS, &tmp);
    if (!tmp)
    {
        glGetShaderInfoLog(gMainParticle, 4096, &tmp2, err);
        err[tmp2]=0;
        MessageBox(hWnd, err, "gMainParticle shader error", MB_OK);
        return;
    }
    glGetShaderiv(fMainParticle, GL_COMPILE_STATUS, &tmp);
    if (!tmp)
    {
        glGetShaderInfoLog(fMainParticle, 4096, &tmp2, err);
        err[tmp2]=0;
        MessageBox(hWnd, err, "fMainParticle shader error", MB_OK);
        return;
    }
#endif

    // link shaders:
    glAttachShader(shaderPrograms[0], vMainParticle);
    glAttachShader(shaderPrograms[0], gMainParticle);
    glAttachShader(shaderPrograms[0], fMainParticle);
    glLinkProgram(shaderPrograms[0]);
    glAttachShader(shaderPrograms[1], vHandParticle);
    glAttachShader(shaderPrograms[1], gMainParticle);
    glAttachShader(shaderPrograms[1], fMainParticle);
    glLinkProgram(shaderPrograms[1]);

#ifdef SHADER_DEBUG
    int programStatus;
    glGetProgramiv(shaderPrograms[0], GL_LINK_STATUS, &programStatus);
    if (programStatus == GL_FALSE)
    {
        MessageBox(hWnd, "Could not link program", "Shader 0 error", MB_OK);
        return;
    }
    glGetProgramiv(shaderPrograms[1], GL_LINK_STATUS, &programStatus);
    if (programStatus == GL_FALSE)
    {
        MessageBox(hWnd, "Could not link program", "Shader 1 error", MB_OK);
        return;
    }
#endif

    // Fill the vertices_ and colors_ array with reasonable values
    GenerateParticles();

    // Set up vertex buffer and stuff
    glGenVertexArrays(1, &vaoID); // Create our Vertex Array Object  
    glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  

    int maxAttrt;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrt);

    glGenBuffers(2, vboID); // Generate our Vertex Buffer Object  

                            // Vertex array position data
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vboID[0]); // Bind our Vertex Buffer Object  
    glBufferData(GL_ARRAY_BUFFER, TOTAL_NUM_PARTICLES * 3 * sizeof(GLfloat),
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
    glBindBuffer(GL_ARRAY_BUFFER, vboID[1]); // Bind our Vertex Buffer Object  
    glBufferData(GL_ARRAY_BUFFER, TOTAL_NUM_PARTICLES * 4 * sizeof(GLfloat),
        colors_, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
    glVertexAttribPointer(1, // attribute
        4, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0); // array buffer offset

#ifdef SHADER_DEBUG
                   // Get all the errors:
    GLenum errorValue = glGetError();
    if (errorValue != GL_NO_ERROR)
    {
        char *errorString = (char *)gluErrorString(errorValue);
        MessageBox(hWnd, errorString, "Init error", MB_OK);
        return;
    }
#endif
}

void intro_do( long itime )
{
    static int lastTime = 0;
    static int timeDiff = 0;

    // Find the scene in the script
    int scene_id = 0;
    int start_time = 0;
    while (scene_id < num_scenes_ - 1 &&
        start_time + script_[scene_id][0] * kSceneTic < itime) {
        start_time += script_[scene_id][0] * kSceneTic;
        scene_id++;
    }

#if 0
    // smooth the time
    timeDiff = (100 * timeDiff + (itime - lastTime) * 28) / 128;
    itime = lastTime + timeDiff;
    lastTime = itime;
#else
    itime -= start_time;
#endif

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set the render matrix
    float parameterMatrix[4][4];
    parameterMatrix[0][0] = itime / 44100.0f;

    // Set parameters to other locations, using seed stuff
    unsigned int start_seed = script_[scene_id][1]; 
    unsigned int seed = start_seed;
    parameterMatrix[0][1] = jo_frand(&seed);
    parameterMatrix[0][2] = jo_frand(&seed);
    parameterMatrix[0][3] = jo_frand(&seed);

    parameterMatrix[1][0] = jo_frand(&seed);
    parameterMatrix[1][1] = jo_frand(&seed);
    parameterMatrix[1][2] = jo_frand(&seed);
    parameterMatrix[1][3] = jo_frand(&seed);

    parameterMatrix[2][0] = jo_frand(&seed);
    parameterMatrix[2][1] = jo_frand(&seed);
    parameterMatrix[2][2] = jo_frand(&seed);

    //parameterMatrix[2][2] += (itime - last_effect_reset_time_) * 0.000001f * movement_speed_;
    parameterMatrix[2][2] += (itime) * 0.000001f * script_[scene_id][2];

    parameterMatrix[2][3] = jo_frand(&seed);

    parameterMatrix[3][0] = jo_frand(&seed);
    parameterMatrix[3][1] = jo_frand(&seed);
    parameterMatrix[3][2] = jo_frand(&seed);
    parameterMatrix[3][3] = jo_frand(&seed);

    int location = glGetUniformLocation(shaderPrograms[0], "r");
    glUniformMatrix4fv(location, 1, GL_FALSE, &(parameterMatrix[0][0]));
    // render
    glDisable( GL_CULL_FACE );
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // set the viewport (not neccessary?)
    //glGetIntegerv(GL_VIEWPORT, viewport);
    //glViewport(0, 0, XRES, YRES);

    // Set program 1 on seed == 0
    glUseProgram(shaderPrograms[start_seed == 0]);

    glDrawArrays(GL_POINTS, 0, TOTAL_NUM_PARTICLES);
}
