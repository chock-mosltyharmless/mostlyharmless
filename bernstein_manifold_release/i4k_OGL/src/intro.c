#if 0

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

#include "config.h"
#include "intro.h"
#include "mzk.h"

// You can access the wave output for basic synchronization
extern double outwave[][2];

float frand();
int rand();

// -------------------------------------------------------------------
//                          INTRO SCRIPT:
// -------------------------------------------------------------------
// Would go here somehow

// -------------------------------------------------------------------
//                          SHADER CODE:
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

const GLchar *vertexMainObject="\
#version 330 core\n\
in vec3 in_Position;\n\
out vec3 pass_Position;\n\
void main(void)\
{\
    gl_Position = vec4(in_Position, 1.);\n\
	pass_Position = in_Position;\n\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

static GLuint shaderPrograms[1];

#ifdef SHADER_DEBUG
char err[4097];
#endif

// The vertex array and vertex buffer
unsigned int vaoID;
unsigned int vboID;
// And the actual vertices
GLfloat vertices[18];

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

void intro_init( void )
{
	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop...
	
	// init objects:
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);
	shaderPrograms[0] = glCreateProgram();
	// compile sources:
	glShaderSource(vMainObject, 1, &vertexMainObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(fMainBackground, 1, &fragmentMainBackground, NULL);
	glCompileShader(fMainBackground);

#ifdef SHADER_DEBUG
	{
		// Check programs
		int tmp, tmp2;
		glGetShaderiv(vMainObject, GL_COMPILE_STATUS, &tmp);
		if (!tmp)
		{
			glGetShaderInfoLog(vMainObject, 4096, &tmp2, err);
			err[tmp2]=0;
			MessageBox(hWnd, err, "vMainObject shader error", MB_OK);
			return;
		}
		glGetShaderiv(fMainBackground, GL_COMPILE_STATUS, &tmp);
		if (!tmp)
		{
			glGetShaderInfoLog(fMainBackground, 4096, &tmp2, err);
			err[tmp2]=0;
			MessageBox(hWnd, err, "fMainBackground shader error", MB_OK);
			return;
		}
	}
#endif

	// link shaders:
	glAttachShader(shaderPrograms[0], vMainObject);
	glAttachShader(shaderPrograms[0], fMainBackground);
	glLinkProgram(shaderPrograms[0]);
	// The in_Position is at 0:
	glBindAttribLocation(shaderPrograms[0], 0, "in_Position");
	//glBinAttribLocation(shader_id, 1, "in_Color");

#ifdef SHADER_DEBUG
	{
		int programStatus;
		glGetProgramiv(shaderPrograms[0], GL_LINK_STATUS, &programStatus);
		if (programStatus == GL_FALSE)
		{
			MessageBox(hWnd, "Could not link program", "Shader 0 error", MB_OK);
			return;
		}
	}
#endif

	// Replace with real code.
	vertices[0] = -1.0f; vertices[1] = -1.0f; vertices[2] = 1.0f; // Bottom left corner  
	vertices[3] = -1.0f; vertices[4] = 1.0f; vertices[5] = 1.0f; // Top left corner  
	vertices[6] = 1.0f; vertices[7] = 1.0f; vertices[8] = 1.0f; // Top Right corner  
	vertices[9] = 1.0f; vertices[10] = -1.0f; vertices[11] = 1.0f; // Bottom right corner  
	vertices[12] = -1.0f; vertices[13] = -1.0f; vertices[14] = 1.0f; // Bottom left corner  
	vertices[15] = 1.0f; vertices[16] = 1.0f; vertices[17] = 1.0f; // Top Right corner  

	// Set up vertex buffer and stuff
	glGenVertexArrays(1, &vaoID); // Create our Vertex Array Object  
	glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  
  
	glGenBuffers(1, &vboID); // Generate our Vertex Buffer Object  
	glBindBuffer(GL_ARRAY_BUFFER, vboID); // Bind our Vertex Buffer Object  
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
  
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // Set up our vertex attributes pointer 
	//glBindVertexArray(0);

#ifdef SHADER_DEBUG
	// Get all the errors:
	{
		enum GLenum errorValue = glGetError();
		if (errorValue != GL_NO_ERROR)
		{
			MessageBox(hWnd, "There was an error.", "Init error", MB_OK);
			return;
		}
	}
#endif
}

void intro_do( long itime )
{
	float ftime = 0.001f*(float)itime;

#ifdef USEDSOUND
#if 0
	double loudness = 1.0;
	int k;
	int musicPos = (((itime)*441)/10);
	for (k = 0; k < 4096; k++)
	{
		loudness += outwave[musicPos][k]*outwave[musicPos][k];
	}
	//parameterMatrix[15] = (float)log(loudness) * (1.f/24.f); // This makes it silent?
#endif
#endif

    // render
    glDisable( GL_CULL_FACE );

	//parameterMatrix[0] = ftime; // time	
	// get music information

	//glClearColor(0.5f, 0.3f, 0.1f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);

	// set the viewport (not neccessary?)
	//glGetIntegerv(GL_VIEWPORT, viewport);
	//glViewport(0, 0, XRES, YRES);
	
	glUseProgram(shaderPrograms[0]);

	//glBindVertexArray(vaoID);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glBindVertexArray(0);
}

#else
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

const GLchar *vertexMainObject="\
#version 330 core\n\
in vec3 in_Position;\n\
out vec3 pass_Position;\n\
void main(void)\
{\
    gl_Position = vec4(in_Position, 1.);\n\
	pass_Position = in_Position;\n\
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
    LoadTextFile("shaders/vertex_hand_particle.txt", shader_text, kMaxShaderLength);
    //glShaderSource(vHandParticle, 1, &vertexMainObject, NULL);
    glShaderSource(vHandParticle, 1, &pt, NULL);
    glCompileShader(vHandParticle);
    LoadTextFile("shaders/vertex_main_particle.txt", shader_text, kMaxShaderLength);
    glShaderSource(vMainParticle, 1, &pt, NULL);
    glCompileShader(vMainParticle);
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

#endif