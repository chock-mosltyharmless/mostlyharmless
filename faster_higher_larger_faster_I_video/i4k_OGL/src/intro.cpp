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
#include <emmintrin.h>

#include "config.h"
#include "intro.h"
#include "mzk.h"
#include "Parameter.h"

float frand();
int rand();
void srand(unsigned int seed);

#ifndef PI
#define PI 3.1415f
#endif

#define SHADER_DEBUG

inline int ftoi_fast(float f)
{
    return _mm_cvtt_ss2si(_mm_load_ss(&f));     // SSE1 instructions for float->int
}

extern double accumulated_drum_volume;
extern bool has_ended;

// -------------------------------------------------------------------
//                          SHADER CODE:
// -------------------------------------------------------------------

#pragma data_seg(".fragment_main_background")
static const GLchar *fragmentMainBackground=
"uniform sampler2D Texture0;\n"
"varying vec4 color;\n"
"void main(void) {\n"
"    vec4 col = texture2D(Texture0, gl_TexCoord[0].xy);\n"
"    gl_FragColor = col * color;\n"
"}";

#pragma data_seg(".vertex_main_object")
static const GLchar *vertexMainObject=
"#version 120\n"
"varying vec4 color;\n"
"void main(void) {\n"
    "gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "color = gl_Color;\n"
    "gl_Position = vec4(gl_Vertex.x, gl_Vertex.y, 0.5, 1.0);\n"
"}";

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

HWND hWnd;

#define MAX_GL_NAME_LENGTH 16
#pragma data_seg(".gl_names")
#ifdef SHADER_DEBUG
#define NUM_GL_NAMES 10
const static char* glnames[NUM_GL_NAMES]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D", "glGetShaderiv","glGetShaderInfoLog",
};
#else
#define NUM_GL_NAMES 8
const static char glnames[NUM_GL_NAMES][MAX_GL_NAME_LENGTH]={
	{"glCreateShader"}, {"glCreateProgram"}, {"glShaderSource"}, {"glCompileShader"}, 
	{"glAttachShader"}, {"glLinkProgram"}, {"glUseProgram"},
	{"glTexImage3D"}
};
#endif

#define glCreateShader ((PFNGLCREATESHADERPROC)glFP[0])
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)glFP[1])
#define glShaderSource ((PFNGLSHADERSOURCEPROC)glFP[2])
#define glCompileShader ((PFNGLCOMPILESHADERPROC)glFP[3])
#define glAttachShader ((PFNGLATTACHSHADERPROC)glFP[4])
#define glLinkProgram ((PFNGLLINKPROGRAMPROC)glFP[5])
#define glUseProgram ((PFNGLUSEPROGRAMPROC)glFP[6])
#define glTexImage3D ((PFNGLTEXIMAGE3DPROC)glFP[7])
#define glGetShaderiv ((PFNGLGETSHADERIVPROC)glFP[8])
#define glGetShaderInfoLog ((PFNGLGETSHADERINFOLOGPROC)glFP[9])

// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];

// Offscreen textures with width and height halved each
// offscreen_texture_[0] always has the size of the backbuffer.
#define NUM_OFFSCREEN_TEXTURES 6
static GLuint offscreen_texture_[NUM_OFFSCREEN_TEXTURES];
static GLuint offscreen_size_[NUM_OFFSCREEN_TEXTURES][2];
static GLuint white_texture_;

typedef void (*GenFP)(void); // pointer to openGL functions
static GenFP glFP[NUM_GL_NAMES]; // pointer to openGL functions
static GLuint shaderProgram;


// This datastructure holds the transformations that shall be rendered. The first four are the core ones.
// If one of the size values is too large, it is not rendered (see disabled structure), and the 4 children are created
const int kMaxNumTransforms = 100000;  // Not active ones, but total ones
const float kMaxSize = 0.4f;
int num_transforms_;  // Not active ones, but total ones
bool transform_is_used_[kMaxNumTransforms];
float transform_[kMaxNumTransforms][2][3];  // Last row is implicitly 0 0 1
float color_[kMaxNumTransforms][3];  // may be larger than 1?

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

static hsv   rgb2hsv(rgb in);
static rgb   hsv2rgb(hsv in);

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, v is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
        if( in.g >= max )
            out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
        else
            out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}


rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    // Convert h to range 0..360 (Note this is slow for large values...)
    while (in.h < 0) in.h += 360.0;
    while (in.h >= 360.0) in.h -= 360.0;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}

#ifdef SHADER_DEBUG
static char err[4097];
#endif
#pragma code_seg(".intro_init")
void intro_init( void )
{
	// create openGL functions
	for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop...
	
	// init objects:	
	GLuint vMainObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fMainBackground = glCreateShader(GL_FRAGMENT_SHADER);	
	shaderProgram = glCreateProgram();
	// compile sources:
	glShaderSource(vMainObject, 1, &vertexMainObject, NULL);
	glCompileShader(vMainObject);
	glShaderSource(fMainBackground, 1, &fragmentMainBackground, NULL);
	//glShaderSource(fMainBackground, 1, &fotzeShader, NULL);
	glCompileShader(fMainBackground);

#ifdef SHADER_DEBUG
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
#endif

	// link shaders:
	glAttachShader(shaderProgram, vMainObject);
	glAttachShader(shaderProgram, fMainBackground);
	glLinkProgram(shaderProgram);

    // Create a rendertarget texture
    for (int i = 0; i < NUM_OFFSCREEN_TEXTURES; i++) {
        glGenTextures(1, offscreen_texture_ + i);
        glBindTexture(GL_TEXTURE_2D, offscreen_texture_[i]);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        offscreen_size_[i][0] = XRES >> i;
        offscreen_size_[i][1] = YRES >> i;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
            offscreen_size_[i][0], offscreen_size_[i][1],
            0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }

    // Create a purely white texture
    unsigned int white[4*4]={
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
    };
    glGenTextures(1, &white_texture_);
    glBindTexture(GL_TEXTURE_2D, white_texture_);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, white);

	// RLY?
	//glEnable(GL_CULL_FACE);
}

void DrawQuad(float transform[2][3], float red, float green, float blue, float alpha)
{
    // set up matrices
    glEnable(GL_TEXTURE_2D);

    glColor4f(red, green, blue, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f * transform[0][0] + -1.0f * transform[0][1] + transform[0][2],
        -1.0f * transform[1][0] + -1.0f * transform[1][1] + transform[1][2],
        0.99f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(+1.0f * transform[0][0] + -1.0f * transform[0][1] + transform[0][2],
        +1.0f * transform[1][0] + -1.0f * transform[1][1] + transform[1][2],
        0.99f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(+1.0f * transform[0][0] + +1.0f * transform[0][1] + transform[0][2],
        +1.0f * transform[1][0] + +1.0f * transform[1][1] + transform[1][2],
        0.99f);
    glTexCoord2f(0.0, 1.0f);
    glVertex3f(-1.0f * transform[0][0] + +1.0f * transform[0][1] + transform[0][2],
        -1.0f * transform[1][0] + +1.0f * transform[1][1] + transform[1][2],
        0.99f);
    glEnd();
}

// I am missing tilt...
void DrawFunction(float rotation, float width, float height, float x, float y,
                  float red, float green, float blue) {
    if (num_transforms_ >= kMaxNumTransforms) return;
    rotation *= 3.1416f * 2.0f;
    transform_[num_transforms_][0][0] = (float)cos(rotation) * width;
    transform_[num_transforms_][0][1] = -(float)sin(rotation) * height;
    transform_[num_transforms_][1][0] = (float)sin(rotation) * width * 16.0f / 9.0f;
    transform_[num_transforms_][1][1] = (float)cos(rotation) * height * 16.0f / 9.0f;
    transform_[num_transforms_][0][2] = x;
    transform_[num_transforms_][1][2] = y * 16.0f / 9.0f;
    transform_is_used_[num_transforms_] = true;
    color_[num_transforms_][0] = red;
    color_[num_transforms_][1] = green;
    color_[num_transforms_][2] = blue;
    num_transforms_++;
    //DrawQuad(transform, red, green, blue, alpha);
}

// first is executed before last, so I do last*first.
void MultiplyTransforms(float first[2][3], float last[2][3], float result[2][3]) {
    result[0][0] = last[0][0] * first[0][0] + last[0][1] * first[1][0];
    result[0][1] = last[0][0] * first[0][1] + last[0][1] * first[1][1];
    result[0][2] = last[0][0] * first[0][2] + last[0][1] * first[1][2] + last[0][2];
    result[1][0] = last[1][0] * first[0][0] + last[1][1] * first[1][0];
    result[1][1] = last[1][0] * first[0][1] + last[1][1] * first[1][1];
    result[1][2] = last[1][0] * first[0][2] + last[1][1] * first[1][2] + last[1][2];
}

// Applies two colors in succession (dot product)
void MultiplyColors(float first[3], float last[3], float result[3]) {
    for (int i = 0; i < 3; i++) {
        result[i] = first[i] * last[i];
    }
}

void DrawAll(float zoom) {
    // Split up the large stuff
    // Note that the ending criterion is not pre-determined, as num_transforms_ may increase.
    for (int i = 0; i < num_transforms_; i++) {
        if (num_transforms_ > kMaxNumTransforms - 4) {
            break;  // No more space left
        }
        if (transform_is_used_[i]) {
            if (fabsf(transform_[i][0][0]) > kMaxSize ||
                fabsf(transform_[i][0][1]) > kMaxSize ||
                fabsf(transform_[i][1][0]) > kMaxSize ||
                fabsf(transform_[i][1][1]) > kMaxSize) {
                transform_is_used_[i] = false;
                MultiplyTransforms(transform_[0], transform_[i], transform_[num_transforms_]);
                MultiplyColors(color_[0], color_[i], color_[num_transforms_]);
                transform_is_used_[num_transforms_] = true;
                num_transforms_++;
                MultiplyTransforms(transform_[1], transform_[i], transform_[num_transforms_]);
                MultiplyColors(color_[1], color_[i], color_[num_transforms_]);
                transform_is_used_[num_transforms_] = true;
                num_transforms_++;
                MultiplyTransforms(transform_[2], transform_[i], transform_[num_transforms_]);
                MultiplyColors(color_[2], color_[i], color_[num_transforms_]);
                transform_is_used_[num_transforms_] = true;
                num_transforms_++;
                MultiplyTransforms(transform_[3], transform_[i], transform_[num_transforms_]);
                MultiplyColors(color_[3], color_[i], color_[num_transforms_]);
                transform_is_used_[num_transforms_] = true;
                num_transforms_++;
            }
        }
    }

    // Draw all that is there
    for (int i = 0; i < num_transforms_; i++) {
        if (transform_is_used_[i]) {
            for (int j = 0; j < 6; j++) transform_[i][0][j] *= zoom;
            DrawQuad(transform_[i], color_[i][0], color_[i][1], color_[i][2], 1.0f);
        }
    }
}

#pragma code_seg(".intro_do")
void intro_do( long itime )
{
	float ftime = 0.001f * (float)itime;

	/* Set everything to beginning */
	for (int i = 0; i < 16; i++)
	{
		parameterMatrix[i] = 0.0f;
	}

	glDisable(GL_BLEND);

	// draw background:
	glMatrixMode(GL_MODELVIEW);	


    parameterMatrix[0] = (float)sin(accumulated_drum_volume*0.1199235712612);  // Optimized to synchronize with music
    parameterMatrix[1] = (float)cos(accumulated_drum_volume*0.1199235712612);
    parameterMatrix[2] = 0.07f;
	parameterMatrix[15] = ftime; // time
	glLoadMatrixf(parameterMatrix);

	// draw offscreen
//	glUseProgram(shaderProgram);
//	glBindTexture(GL_TEXTURE_3D, noiseTexture);
//	glRectf(-1., -1., 1., 1.);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    //glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glBlendFunc(GL_ONE, GL_ONE);

#if 0
    // draw offscreen
    glBindTexture(GL_TEXTURE_2D, white_texture_);
    glUseProgram(shaderProgram);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    //glRectf(-1.0, -1.0, 0.0, 0.0);
    float transform[2][3] = {{0.5f, 0.0f, 0.0f},{0.0f,0.5f, 0.0f}};
    DrawQuad(transform, 0.7f, 0.9f, 1.0f, 1.0f);

    // copy to front
    glBindTexture(GL_TEXTURE_2D, offscreen_texture_[0]);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, XRES, YRES);   //Copy back buffer to texture
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glColor4f(1.0, 0.5, 0.5, 1.0);
    float identity[2][3] = {{1.0f, 0.0f, 0.0f},{0.0f, 1.0f, 0.0f}};
    DrawQuad(identity, 1.0f, 1.0f, 1.0f, 1.0f);
#endif

    // TODO: Make the first 1 or 2 passes to the smaller backbuffer
    int num_passes = 10;
    int last_offscreen_id = -1;
    glUseProgram(shaderProgram);
    for (int pass = 0; pass < num_passes; pass++) {
        srand(1);
        int offscreen_id = ((num_passes - pass) << 0) - 1;  // So that last pass is on offscreen_id;
        if (offscreen_id >= NUM_OFFSCREEN_TEXTURES) offscreen_id = NUM_OFFSCREEN_TEXTURES - 1;
        if (offscreen_id < 0) offscreen_id = 0;

        // In the first pass, use highlight
        glViewport(0, 0, offscreen_size_[offscreen_id][0], offscreen_size_[offscreen_id][1]); 

        // Copy backbuffer to texture
        if (last_offscreen_id >= 0) {
            glBindTexture(GL_TEXTURE_2D, offscreen_texture_[last_offscreen_id]);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                offscreen_size_[last_offscreen_id][0],
                offscreen_size_[last_offscreen_id][1]);  // Copy backbuffer to texture
        } else {
            glBindTexture(GL_TEXTURE_2D, white_texture_);
        }

        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;
        glClearColor(red, green, blue, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw one iteration of the IFS
        float zoom = 1.0f;
        if (pass == num_passes - 1) zoom = 1.0f;
#if 0
        float transformation[2][3];
        for (int i = 0; i < 12; i++) {
            float size = 0.7f + 0.3f * sinf(rand() * 0.001f);
            transformation[0][0] = zoom * 0.7f * sinf(rand() * 0.001f) * size;
            transformation[0][1] = zoom * 0.7f * sinf(rand() * 0.001f) * size;
            transformation[0][2] = zoom * 0.3f * sinf(rand() * 0.001f) * sinf(rand() * 0.001f);
            transformation[1][0] = zoom * 0.7f * sinf(rand() * 0.001f) * size;
            transformation[1][1] = zoom * 0.7f * sinf(rand() * 0.001f) * size;
            transformation[1][2] = zoom * 0.3f * sinf(rand() * 0.001f) * sinf(rand() * 0.001f);
            hsv incolor = {(float)(rand() % 1000), 0.3f, 0.6f};
            rgb outcolor = hsv2rgb(incolor);
            DrawQuad(transformation, (float)outcolor.r, (float)outcolor.g, (float)outcolor.b, 1.0f);
        }
#else 
        // 2 3 4 5 6 8   9 12 13 14 15 16      17 18 19 20 21 22
        // 2:1.060(136) 3:1.120(143) 4:0.450(58) 5:1.040(133) 6:0.390(50) 8:0.340(44) 9:0.360(46) 12:0.310(40)
        // 13:0.210(27) 14:0.620(79) 15:0.510(65) 16:0.010(1) 17:0.470(60) 18:0.580(74) 19:0.550(70)
        num_transforms_ = 0;  // Reset
        DrawFunction(params.getParam(2, 1.06f),
            params.getParam(5, 1.04f),
            params.getParam(9, 0.36f),
            params.getParam(14, 0.62f) - 0.5f,
            params.getParam(17, 0.47f) - 0.5f,
            1.0f, 1.0f, 1.0f);
        DrawFunction(params.getParam(3, 1.12f),
            params.getParam(6, 0.39f),
            params.getParam(12, 0.31f),
            params.getParam(15, 0.51f) - 0.5f,
            params.getParam(18, 0.58f) - 0.5f,
            0.9f, 1.0f, 1.1f);
        DrawFunction(params.getParam(4, 0.45f),
            params.getParam(8, 0.34f),
            params.getParam(13, 0.21f),
            params.getParam(16, 0.01f) - 0.5f,
            params.getParam(19, 0.55f) - 0.5f,
            1.1f, 1.0f, 0.9f);
        DrawFunction(0.0f,
            0.01f,
            0.01f,
            0.0f,
            0.0f,
            1.0f, 1.0f, 1.0f);
        DrawAll(zoom);
#endif

        last_offscreen_id = offscreen_id;
    }
}

// Here I do something if keys are pressed on the Midi device
void registerParameterChange(int keyID) {
}