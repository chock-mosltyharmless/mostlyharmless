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
c=smoothstep(.0,.1,min(.9-abs(u).r,.9-.9*abs(u).g-.5*abs(u).r))*o;\
}";

#pragma data_seg(".geometryMainParticle")
const GLchar geometryMainParticle[]="\
#version 430 core\n\
layout(points) in;\
layout(triangle_strip,max_vertices=4) out;\
in vec4 p[];\
out vec4 o;\
out vec2 u;\
void main(){\
vec4 e=gl_in[0].gl_Position;\
float q=.001+abs(e.b-.5)*.025;\
mat2 w=min(q,.2*e.w)*mat2(.55,.2,-.1,.98);\
q=.001/q;\
q=pow(q,2.)*smoothstep(q,.0,p[0].a)/q;\
if (q>.01){\
o=q*p[0];\
o.a=0.12*q;\
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
float y(vec3 e){\
e.b-=sin(e.g*cos(e.r*.4)*.3)*r[0][2]*5.5;\
e.r-=sin(e.b*.5)*r[0][2]*5.5;\
e.g-=sin(e.r*.25)*r[0][2]*5.5;\
e.r-=sin(r[0][0]*cos(e.r*2.1)*.3)*r[0][1]*.1;\
e.g-=cos(r[0][0]*cos(e.r*.5)*.4)*r[0][1]*.1;\
e.b-=sin(r[0][0]*sin(e.g*.2)*.5)*r[0][1]*.1;\
return length(e)-sin(r[3][1]*25.*length(e))*r[3][2]*5.-r[3][3]-.2*sin(r[0][0]*.0);\
}\
void main(void) {\
vec3 e=o.rgb;\
float t=y(e),q=abs(sin(e.r*r[1][2]*8.)+cos(e.b*r[1][3]*8.)-sin(e.g*r[2][0]*8.));\
p.rgb=(vec3(.6,1.1,1.2)-vec3(-.3,.4,.8)*length(e))*(pow(1.-o.a,30.)*3.+1.)+pow(abs(sin(o.a*100.+r[0][0])),10.)-q*r[2][1]*3.;\
q=1.+r[1][1]*0.4-smoothstep(.0,.4,abs(t));\
p*=q*(1.-abs(o.a-.5));\
p.a=o.a;\
vec3 f=e*.45+12.*(r[1][0]*sin(r[0][0]*vec3(.49,.31,.37))*sin(e.brg*vec3(7.,5.,6.))+1.-r[1][0])*(vec3(y(e+vec3(.01,.0,.0)),y(e+vec3(.0,.01,.0)),y(e+vec3(.0,.0,.01)))-t)*q*r[0][3];\
t=sin(r[0][0]*.15)*.5;\
f.rb*=mat2(cos(t),sin(t),-sin(t),cos(t));\
f.r=f.r*.56+.25-.5*r[2][3];\
f.g+=.25-.5*r[3][0];\
f.b+=1.5-2.*r[2][2];\
gl_Position=vec4(f,f.b);\
}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define NUM_PARTICLES_PER_DIM 128
#define TOTAL_NUM_PARTICLES (NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM * NUM_PARTICLES_PER_DIM)

#define kSceneTic (2 * AUDIO_BUFFER_SIZE)

// This is only used if SHADER_DEBUG is on, but I will leave it for now.
#ifdef SHADER_DEBUG
extern HWND hWnd;
char err[4097];
#endif

// The vertex array and vertex buffer
// And the actual vertices
static GLfloat vertices_[TOTAL_NUM_PARTICLES * 4];

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

#ifndef NO_INTRO_CODE
#pragma code_seg(".intro_init")
void intro_init( void ) {
#include "intro_init.c"
}

#pragma code_seg(".intro_do")
void intro_do( long itime ) {
#include "intro_do.c"
}
#endif