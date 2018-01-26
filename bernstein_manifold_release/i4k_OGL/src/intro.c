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

#include "script_seed.h"
#include "script_move.h"
#include "script_duration.h"

// -------------------------------------------------------------------
//                          SHADERS:
// -------------------------------------------------------------------
#pragma data_seg(".fragmentMainParticle")
const GLchar fragmentMainParticle[]="\
#version 430 core\n\
in vec2 u;\
in vec4 o;\
out vec4 c;\
void main(void){\
c=smoothstep(.0,.1,min(.9-abs(u).x,.9-.9*abs(u).y-.5*abs(u).x))*o;\
}";

#pragma data_seg(".geometryMainParticle")
const GLchar geometryMainParticle[]="\
#version 430 core\n\
layout(points) in;\
layout(triangle_strip,max_vertices=4) out;\
in vec4 p[];\
in float m[];\
out vec4 o;\
out vec2 u;\
void main(){\
vec4 e=gl_in[0].gl_Position;\
float q=.001+abs(e.z-.5)*.025;\
mat2 w=min(q,.2*e.w)*mat2(.55,.2,-.1,.98);\
q=.001/q;\
q=pow(q,2.)*smoothstep(q,.0,m[0])/q;\
if (q>.01){\
o=q*p[0];\
u=vec2(-1.,1.);\
gl_Position=e+vec4(w*u,.0,.0);\
EmitVertex();\
u=vec2(1.,1.);\
gl_Position=e+vec4(w*u,.0,.0);\
EmitVertex();\
u=vec2(-1.,-1.);\
gl_Position=e+vec4(w*u,.0,.0);\
EmitVertex();\
u=vec2(1.,-1.);\
gl_Position=e+vec4(w*u,0.,0.);\
EmitVertex();\
}\
EndPrimitive();\
}";

// TODO: Use mat4 for o, c --> only one buffer!
#pragma data_seg(".vertexMainParticle")
const GLchar vertexMainParticle[]="\
#version 430 core\n\
layout (location=0) uniform mat4 r;\
layout (location=0) in vec4 o;\n\
out vec4 p;\
out float m;\
float G(vec3 e){\
e.z-=sin(e.y*cos(e.x*.4)*.3)*r[0][2]*5.5;\
e.x-=sin(e.z*.5)*r[0][2]*5.5;\
e.y-=sin(e.x*.25)*r[0][2]*5.5;\
e.x-=sin(r[0][0]*cos(e.x*2.1)*.3)*r[0][1]*.1;\
e.y-=cos(r[0][0]*cos(e.x*.5)*.4)*r[0][1]*.1;\
e.z-=sin(r[0][0]*sin(e.y*.2)*.5)*r[0][1]*.1;\
return length(e)-sin(r[3][1]*25.*length(e))*r[3][2]*5.-r[3][3]-.2*sin(r[0][0]*.0);\
}\
void main(void) {\
vec3 e=o.xyz;\
float t=G(e),q=abs(sin(e.x*r[1][2]*8.)+cos(e.z*r[1][3]*8.)-sin(e.y*r[2][0]*8.));\
m=o.a;\
p.rgb=(vec3(.6,1.1,1.2)+vec3(.3,-.4,-.8)*length(e))*(pow(1.-m,30.)*3.+1.)+pow(abs(sin(m*100.+r[0][0])),10.)-q*r[2][1]*3.;\
p.a=1.+q*r[2][1];\
q=1.+r[1][1]*0.4-smoothstep(.0,.4,abs(t));\
p*=q*(1.-abs(m-.5));\
vec3 f=e*.45+12.*(r[1][0]*sin(r[0][0]*vec3(.49,.31,.37))*sin(e.zxy*vec3(7.,5.,6.))+1.-r[1][0])*(vec3(G(e+vec3(.01,.0,.0)),G(e+vec3(.0,.01,.0)),G(e+vec3(.0,.0,.01)))-t)*q*r[0][3];\
t=sin(r[0][0]*.15)*.5;\
f.xz*=mat2(cos(t),sin(t),-sin(t),cos(t));\
f.x*=.56;\
f.x+=.25-.5*r[2][3];\
f.y+=.25-.5*r[3][0];\
f.z+=1.5-2.*r[2][2];\
gl_Position=vec4(f,f.z);\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define NUM_PARTICLES_PER_DIM 128
#define TOTAL_NUM_PARTICLES (NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM)

//const int kSceneTic = 4410; // Number of 1/44100 seconds per scene-time-unit?
#define kSceneTic (2 * AUDIO_BUFFER_SIZE)

// This is only used if SHADER_DEBUG is on, but I will leave it for now.
HWND hWnd;
#ifdef SHADER_DEBUG
char err[4097];
#endif

static GLuint shaderProgram;
// The vertex array and vertex buffer
unsigned int vaoID;
// 0 is for particle positions, 1 is for particle colors
//unsigned int vboID;
// And the actual vertices
GLfloat vertices_[TOTAL_NUM_PARTICLES * 4];
//GLfloat colors_[TOTAL_NUM_PARTICLES * 4];

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

#pragma code_seg(".intro_init")
void intro_init( void ) {
    // Load the script
    // Create and link shader and stuff:

    // init objects:
    shaderProgram = glCreateProgram();
#if 1
    GLuint gMainParticle = glCreateShader(GL_GEOMETRY_SHADER_EXT);
    GLuint fMainParticle = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint vMainParticle = glCreateShader(GL_VERTEX_SHADER);
    // compile sources:
    const char *pt = vertexMainParticle;
    glShaderSource(vMainParticle, 1, &pt, NULL);
    glCompileShader(vMainParticle);
    pt = geometryMainParticle;
    glShaderSource(gMainParticle, 1, &pt, NULL);
    glCompileShader(gMainParticle);
    pt = fragmentMainParticle;
    glShaderSource(fMainParticle, 1, &pt, NULL);
    glCompileShader(fMainParticle);
#else
    GLuint shaders[4];
#define gMainParticle (shaders[0])
#define fMainParticle (shaders[1])
#define vMainParticle (shaders[2])
#define vHandParticle (shaders[3])

    shaders[0] = glCreateShader(GL_GEOMETRY_SHADER_EXT);
    shaders[1] = glCreateShader(GL_FRAGMENT_SHADER);
    shaders[2] = glCreateShader(GL_VERTEX_SHADER);
    shaders[3] = glCreateShader(GL_VERTEX_SHADER);
    for (int i = 0; i < 4; i++) {
        glShaderSource(shaders[i], 1, &shader_codes[i], NULL);
        glCompileShader(shaders[i]);
    }
#endif

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
    glAttachShader(shaderProgram, vMainParticle);
    glAttachShader(shaderProgram, gMainParticle);
    glAttachShader(shaderProgram, fMainParticle);
    glLinkProgram(shaderProgram);

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

    //unsigned int seed = 23690984;
    unsigned int seed = 0;

    // Set vertex location
    int vertex_id = 0;
    //int color_id = 0;
    float pp[3];
    pp[2] = 1.0f;
    for (int z = 0; z < NUM_PARTICLES_PER_DIM; z++) {
        pp[1] = 1.0f;
        for (int y = 0; y < NUM_PARTICLES_PER_DIM; y++) {
            pp[0] = 1.0f;
            for (int x = 0; x < NUM_PARTICLES_PER_DIM; x++) {
                for (int dim = 0; dim < 3; dim++) {
                    vertices_[vertex_id++] = pp[dim];// + 2.0f / (float)NUM_PARTICLES_PER_DIM * jo_frand(&seed);
                }
                //color_id += 3;  // ignore RGB
                // fran
                //colors_[color_id] = jo_frand(&seed);
                //colors_[color_id] = 0.5f + 0.5f * sinf(pp[0] * pp[1] * pp[2] * (1<<24));
                vertices_[vertex_id] = 0.5f + 0.5f * sinf((pp[0] * pp[1] * pp[2] + pp[2]) * (1 << 24));
                vertices_[vertex_id] = 1.0f - vertices_[vertex_id] * vertices_[vertex_id];
                vertex_id++;
                pp[0] -= 2.0f / (float)NUM_PARTICLES_PER_DIM;
            }
            pp[1] -= 2.0f / (float)NUM_PARTICLES_PER_DIM;
        }
        pp[2] -= 2.0f / (float)NUM_PARTICLES_PER_DIM;
    }


    // Set up vertex buffer and stuff
    glGenVertexArrays(1, &vaoID); // Create our Vertex Array Object  
    glBindVertexArray(vaoID); // Bind our Vertex Array Object so we can use it  

    int maxAttrt;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrt);

    int vboID;
    glGenBuffers(1, &vboID); // Generate our Vertex Buffer Object  

                            // Vertex array position data
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vboID); // Bind our Vertex Buffer Object  
    glBufferData(GL_ARRAY_BUFFER, TOTAL_NUM_PARTICLES * 4 * sizeof(GLfloat),
        vertices_, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
    glVertexAttribPointer(0, // attribute
        4, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0); // array buffer offset

#if 0
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
#endif

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

#pragma code_seg(".intro_do")
void intro_do( long itime )
{
    //static int lastTime = 0;
    //static int timeDiff = 0;

    // Find the scene in the script
    int scene_id = 0;
    int start_time = 0;
    while (start_time + (int)(script_duration_[scene_id]) * kSceneTic < itime) {
        start_time += (int)(script_duration_[scene_id]) * kSceneTic;
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

    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set the render matrix
    float parameterMatrix[4][4];

    // Set parameters to other locations, using seed stuff
    unsigned int start_seed = script_seed_[scene_id]; 
    unsigned int seed = start_seed;
    for (int i = 1; i < 16; i++) {
        parameterMatrix[0][i] = jo_frand(&seed);
    }

    parameterMatrix[0][0] = itime / 32768.0f;
    parameterMatrix[2][2] += (itime) / 1048576.0f * script_move_[scene_id];

    //int location = glGetUniformLocation(shaderProgram, "r");
    glUniformMatrix4fv(0/*location*/, 1, GL_FALSE, &(parameterMatrix[0][0]));
    // render
    //glDisable( GL_CULL_FACE );
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // set the viewport (not neccessary?)
    //glGetIntegerv(GL_VIEWPORT, viewport);
    //glViewport(0, 0, XRES, YRES);

    // Set program 1 on seed == 0
    glUseProgram(shaderProgram);

    glDrawArrays(GL_POINTS, 0, TOTAL_NUM_PARTICLES);
}
