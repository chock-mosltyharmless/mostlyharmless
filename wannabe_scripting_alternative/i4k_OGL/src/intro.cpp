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

#include "intro.h"
#include "mzk.h"

float frand();
int rand();

// -------------------------------------------------------------------
//                          INTRO SCRIPT:
// -------------------------------------------------------------------
#define EFFECT_SET_PARAMETER 0 // 3 params: index, startValue, endValue
#define EFFECT_DRAW_SPHERE   1 // 1 param: size
#define EFFECT_SET_SHADER    2 // 1 param:  shaderNumber
#define EFFECT_SET_Z_WRITE   3 // 1 param:  Boolean

#define SHADER_OBJECT     0
#define SHADER_TINT       1
#define SHADER_BACKGROUND 2

#define NUM_SCENES 12
#define TOTAL_NUM_EFFECTS 176

extern short myMuzik[];

#pragma data_seg(".sceneDurations")
const static short sceneDuration[NUM_SCENES] =
{
	127,
	64,
	32,
	2,
	32,
	4,
	32,	
	32,
	64,
	8,
	4,
	32,
};

#pragma data_seg(".sceneNumEffects")
const static unsigned char sceneNumEffects[NUM_SCENES] =
{
	5,
	4,
	9,
	27,
	27,
	7,
	16,	
	8,
	19,
	8,
	19,
	27,
};

#pragma data_seg(".sceneEffect")
const static unsigned char sceneEffect[TOTAL_NUM_EFFECTS] =
{	
	// Scene 1: Sky sweep
	EFFECT_SET_SHADER,    // background shader
	EFFECT_SET_PARAMETER, // set time
	EFFECT_SET_PARAMETER, // set background rotation
	EFFECT_SET_PARAMETER, // set background rotation
	EFFECT_DRAW_SPHERE,   // draw background
	
	// Scene 2: Floor sweep
	EFFECT_SET_PARAMETER, // set time
	EFFECT_SET_PARAMETER, // set background rotation
	EFFECT_SET_PARAMETER, // set background rotation
	EFFECT_DRAW_SPHERE,   // draw background

	// Scene 3: Balls homing in
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // x-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball
	EFFECT_SET_PARAMETER, // x-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw second ball
	
	// Scene 4: Balls touching
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // y-Position of first ball
	EFFECT_SET_PARAMETER, // x-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball
	EFFECT_SET_PARAMETER, // x-Position of second ball
	EFFECT_SET_PARAMETER, // y-Position of second ball
	EFFECT_SET_PARAMETER, // z-Rotation of tint
	EFFECT_DRAW_SPHERE,   // Draw second ball	
	EFFECT_SET_PARAMETER, // x-Position of tint
	EFFECT_SET_PARAMETER, // y-Position of tint
	EFFECT_SET_PARAMETER, // Z-Position of tint
	EFFECT_SET_SHADER,    // TINT
//	EFFECT_SET_Z_WRITE,   // Set Z-Write
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw first tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw second tint
	EFFECT_SET_PARAMETER, // color of tint
	EFFECT_SET_PARAMETER, // Z-Rotation of tint
	EFFECT_SET_PARAMETER, // x-Position of tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint	
	EFFECT_DRAW_SPHERE,   // Draw first tint 1
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw second tint 1
//	EFFECT_SET_Z_WRITE,   // Set Z-Write

	// Scene 5: Balls hovering
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // y-Position of first ball
	EFFECT_SET_PARAMETER, // x-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball
	EFFECT_SET_PARAMETER, // x-Position of second ball
	EFFECT_SET_PARAMETER, // y-Position of second ball
	EFFECT_SET_PARAMETER, // z-Rotation of tint
	EFFECT_DRAW_SPHERE,   // Draw second ball	
	EFFECT_SET_PARAMETER, // x-Position of tint
	EFFECT_SET_PARAMETER, // y-Position of tint
	EFFECT_SET_PARAMETER, // Z-Position of tint
	EFFECT_SET_SHADER,    // TINT
//	EFFECT_SET_Z_WRITE,   // Set Z-Write
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw first tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw second tint
	EFFECT_SET_PARAMETER, // color of tint
	EFFECT_SET_PARAMETER, // Z-Rotation of tint
	EFFECT_SET_PARAMETER, // x-Position of tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint	
	EFFECT_DRAW_SPHERE,   // Draw first tint 1
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw second tint 1
//	EFFECT_SET_Z_WRITE,   // Set Z-Write

	// Scene 8: Falling stone
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // y-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball

	// Scene 9: black splashin
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_SET_PARAMETER, // color
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // y-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball
	EFFECT_SET_SHADER,    // TINT
	//EFFECT_SET_Z_WRITE,   // Set Z-Write
	EFFECT_SET_PARAMETER, // Z-Position of tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint	
	EFFECT_DRAW_SPHERE,   // Draw first tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw second tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw third tint
	//EFFECT_SET_Z_WRITE,   // Set Z-Write

	// Scene 6: one ball hovering
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // y-Position of first ball
	EFFECT_SET_PARAMETER, // x-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball

	// Scene 7: black ball hovering
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_SET_PARAMETER, // color
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // y-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball
	EFFECT_SET_SHADER,    // TINT
	//EFFECT_SET_Z_WRITE,   // Set Z-Write
	EFFECT_SET_PARAMETER, // Y rotation
	EFFECT_SET_PARAMETER, // Z-Position of tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint	
	EFFECT_DRAW_SPHERE,   // Draw first tint
	EFFECT_SET_PARAMETER, // Y rotation
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw second tint
	EFFECT_SET_PARAMETER, // Y rotation
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw third tint
	//EFFECT_SET_Z_WRITE,   // Set Z-Write

	// Scene 10: Stone moving right
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // y-Position of first ball
	EFFECT_SET_PARAMETER, // x-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball

	// Scene 11: Stone jumping up
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // y-Position of first ball
	EFFECT_SET_PARAMETER, // x-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball
	EFFECT_SET_PARAMETER, // y-Position of second ball
	EFFECT_SET_PARAMETER, // x-Position of second ball
	EFFECT_DRAW_SPHERE,   // Draw second ball
	EFFECT_SET_SHADER,    // TINT
	//EFFECT_SET_Z_WRITE,   // Set Z-Write
	EFFECT_SET_PARAMETER, // z-Position of first tint
	EFFECT_SET_PARAMETER, // y-Position of first tint
	EFFECT_SET_PARAMETER, // x-Position of first tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw first tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw second tint
	//EFFECT_SET_Z_WRITE,   // Set Z-Write

	// Scene 12: Final
	EFFECT_SET_SHADER,    // Background
	EFFECT_SET_PARAMETER, // set time
	EFFECT_DRAW_SPHERE,   // draw background
	EFFECT_SET_SHADER,    // BALL
	EFFECT_SET_PARAMETER, // z-Position of first ball
	EFFECT_SET_PARAMETER, // y-Position of first ball
	EFFECT_SET_PARAMETER, // x-Position of first ball
	EFFECT_DRAW_SPHERE,   // Draw first ball
	EFFECT_SET_PARAMETER, // color of tint
	EFFECT_SET_PARAMETER, // y-Position of second ball
	EFFECT_SET_PARAMETER, // x-Position of second ball
	EFFECT_DRAW_SPHERE,   // Draw second ball
	EFFECT_SET_SHADER,    // TINT
	//EFFECT_SET_Z_WRITE,   // Set Z-Write
	EFFECT_SET_PARAMETER, // z-Position of first tint
	
	EFFECT_SET_PARAMETER, // z-Rotation of first tint
	EFFECT_SET_PARAMETER, // x-Position of first tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw first tint
	EFFECT_SET_PARAMETER, // z-Rotation of first tint
	
	EFFECT_SET_PARAMETER, // color of tint
	EFFECT_SET_PARAMETER, // y-Position of first tint
	EFFECT_SET_PARAMETER, // x-Position of first tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw first tint
	EFFECT_SET_PARAMETER, // Y-Transform of tint
	EFFECT_DRAW_SPHERE,   // Draw second tint
	//EFFECT_SET_Z_WRITE,   // Set Z-Write
};

#pragma data_seg(".sceneParam")
const static char sceneParam[3][TOTAL_NUM_EFFECTS] =
{
	// Param 1
	{
		// Scene 1: Sky sweep
		SHADER_BACKGROUND,
		0,  // time
		4,  // rotation Z
		5,  // rotation Y
		100, // draw background with size 6.25

		// Scene 2: Floor sweep
		0,  // time
		4,  // rotation Z
		5,  // rotation Y
		100, // draw background with size 6.25

		// Scene 3: balls homing in
		SHADER_BACKGROUND,
		0,  // time
		100, // draw background with size 6.25
		SHADER_OBJECT,
		10, // z-position of ball
		8, // x-position of ball
		4, // draw ball 1
		8, // x-position of ball
		4, // draw ball 2

		// Scene 4: Balls touching
		SHADER_BACKGROUND,
		0, // time
		100, // draw background with size 6.25
		SHADER_OBJECT,
		10, // z-position of ball
		9, // y-position of ball
		8, // x-position of ball
		4, // draw ball 1
		8, // x-position of ball
		9, // y-position of ball		
		4,  // rotation Z
		4, // draw ball 2
		8,  // x-position of tint
		9,  // x-position of tint
		10, // z-position of tint		
		SHADER_TINT,
		//0, // Z-Write
		6, // Y-Transform of tint
		4, // draw tint 1
		6, // Y-Transform of tint
		4, // draw tint 2
		1, // color of tint
		4,  // rotation Z
		8, // X-Position of tint		
		6, // Y-Transform of tint
		4, // draw tint 1 1
		6, // Y-Transform of tint
		4, // draw tint 2 1
		//1, // Z-Write

		// Scene 5: Balls overing
		SHADER_BACKGROUND,
		0, // time
		100, // draw background with size 6.25
		SHADER_OBJECT,
		10, // z-position of ball
		9, // y-position of ball
		8, // x-position of ball
		4, // draw ball 1
		8, // x-position of ball
		9, // y-position of ball		
		4,  // rotation Z
		4, // draw ball 2
		8,  // x-position of tint
		9,  // x-position of tint
		10, // z-position of tint		
		SHADER_TINT,
		//0, // Z-Write
		6, // Y-Transform of tint
		2, // draw tint 1
		6, // Y-Transform of tint
		4, // draw tint 2
		1, // color of tint
		4,  // rotation Z
		8, // X-Position of tint		
		6, // Y-Transform of tint
		4, // draw tint 1 1
		6, // Y-Transform of tint
		4, // draw tint 2 1
		//1, // Z-Write

		// Scene 8: Falling stone
		SHADER_BACKGROUND,    // Background
		0, // set time
		100,   // draw background
		SHADER_OBJECT,    // BALL
		10, // z-Position of first ball
		9, // y-Position of first ball
		4,   // Draw first ball

		// Scene 9: black splashin
		SHADER_BACKGROUND,    // Background
		0, // set time
		1, // color
		100,   // draw background
		SHADER_OBJECT,    // BALL
		10, // z-Position of first ball
		9, // y-Position of first ball
		4,   // Draw first ball
		SHADER_TINT,    // TINT
		//0,   // Set Z-Write
		10,  // z-Position
		6, // Y-Transform of tint
		4,   // Draw first tint
		6, // Y-Transform of tint
		4,   // Draw second tint
		6, // Y-Transform of tint
		4,   // Draw third tint
		//1,   // Set Z-Write

		// Scene 6: one ball hovering
		SHADER_BACKGROUND,    // Background
		0, // set time
		100,   // draw background
		SHADER_OBJECT,    // BALL
		10, // z-Position of first ball
		9, // y-Position of first ball
		8, // x-Position of first ball
		4,   // Draw first ball

		// Scene 6: black ball hovering
		SHADER_BACKGROUND,    // Background
		0, // set time
		1, // color
		100,   // draw background
		SHADER_OBJECT,    // BALL
		10, // z-Position of first ball
		9, // y-Position of first ball
		4,   // Draw first ball
		SHADER_TINT,    // TINT
		//0,   // Set Z-Write
		5,  // rotation Y
		10,  // z-Position
		6, // Y-Transform of tint
		4,   // Draw first tint
		5,  // rotation Y
		6, // Y-Transform of tint
		4,   // Draw second tint
		5,  // rotation Y
		6, // Y-Transform of tint
		4,   // Draw third tint
		//1,   // Set Z-Write

		// Scene 10: Stone moving right
		SHADER_BACKGROUND,    // Background
		0, // set time
		100,   // draw background
		SHADER_OBJECT,    // BALL
		10, // z-Position of first ball
		9, // y-Position of first ball
		8, // x-Position of first ball
		4,   // Draw first ball

		// Scene 11: Stone jumping up
		SHADER_BACKGROUND,    // Background
		0, // set time
		100,   // draw background
		SHADER_OBJECT,    // BALL
		10, // z-Position of first ball
		9, // y-Position of first ball
		8, // x-Position of first ball
		4,   // Draw first ball
		9, // y-Position of second ball
		8, // x-Position of second ball
		4,   // Draw second ball
		SHADER_TINT,    // TINT
		//0,   // Set Z-Write
		10, // z-Position of first tint
		9, // y-Position of first tint
		8, // x-Position of first tint
		6, // Y-Transform of tint
		4,   // Draw first tint
		6, // Y-Transform of tint
		4,   // Draw second tint
		//1,   // Set Z-Write

		// Scene 12: Final
		SHADER_BACKGROUND,    // Background
		0, // set time
		100,   // draw background
		SHADER_OBJECT,    // BALL
		10, // z-Position of first ball
		9, // y-Position of first ball
		8, // x-Position of first ball
		4,   // Draw first ball
		1, // color of tint
		9, // y-Position of second ball
		8, // x-Position of second ball
		4,   // Draw second ball
		SHADER_TINT,    // TINT
		//0,   // Set Z-Write
		10, // z-Position of first tint

		4, // z-Rotation of first tint
		9, // y-Position of first tint
		8, // x-Pos
		6, // Y-Transform of tint
		4,   // Draw first tint
		4, // z-Rotation of first tint
		1, // color of tint

		9, // y-Position of first tint
		8, // x-Position of first tint
		6, // Y-Transform of tint
		4,   // Draw first tint
		6, // Y-Transform of tint
		4,   // Draw second tint
		//1,   // Set Z-Write
	},
	
	// Param 2
	{
		// Scene 1: Sky sweep
		0, // set background shader
		0, // timeStart
		-2, // rotation Start
		-8, // rotation Start
		0, // draw background

		// Scene 2: Floor sweep
		0, // timeStart
		-2, // rotation Start
		13, // rotation Start
		0, // draw background

		// Scene 3: balls homing in
		0, // background shader
		0,  // time
		100, // draw background with size 6.25
		0, // ball shader
		-48, // z-position of ball
		-112, // x-position of ball
		0, // draw ball
		112, // x-position of ball
		0, // draw ball

		// Scene 4: Balls touching
		0, // background shader
		16, // timeStart
		0, // draw background
		0, // ball shader
		-48, // z-position of ball
		0, // y-position of ball
		-4, // x-position of ball
		0, // draw ball
		4, // x-position of ball
		0, // y-position of ball		
		20,  // rotation Z
		0, // draw ball
		4,  // x-position of tint
		0,  // y-position of tint
		-46, // z-position of tint
		0, // tint shader
		//0, // Z-Write
		4, // Y-Transform of tint
		0, // draw tint
		0, // Y-Transform of tint
		0, // draw tint
		12, // color of tint
		-28,  // rotation Z
		-4, // x-Position of tint
		4, // Y-Transform of tint
		0, // draw tint
		0, // Y-Transform of tint
		0, // draw tint
		//0, // Z-Write

		// Scene 4: Balls hovering
		0, // background shader
		17, // timeEnd
		0, // draw background
		0, // ball shader
		-48, // z-position of ball
		-1, // y-position of ball
		-6, // x-position of ball
		0, // draw ball
		6, // x-position of ball
		1, // y-position of ball
		20, // z-rotation
		0, // draw ball		
		4, // x-position of tint
		0, // y-position of tint
		-46, // z-position of tint
		0, // tint shader
		//0, // Z-Write
		16, // Y-Transform of tint
		0, // draw tint
		13, // Y-Transform of tint
		0, // draw tint
		14, // color of tint
		-28, // z-rotation
		-4, // X-Positon of tint
		16, // Y-Transform of tint
		0, // draw tint
		13, // Y-Transform of tint
		0, // draw tint
		//0, // Z-Write

		// Scene 8: Falling stone
		0,    // Background
		0, // set time
		0,   // draw background
		0,    // BALL
		-32, // z-Position of first ball
		2, // y-Position of first ball
		0,   // Draw first ball

		// Scene 9: black splashin
		0,    // Background
		2, // set time
		0, // color
		0,   // draw background
		0,    // BALL
		-32, // z-Position of first ball
		-4, // y-Position of first ball
		0,   // Draw first ball
		0,    // TINT
		//0,   // Set Z-Write
		-30, // Z-Position of tint
		-4, // Y-Transform of tint	
		0,   // Draw first tint
		0, // Y-Transform of tint
		0,   // Draw second tint
		4, // Y-Transform of tint
		0,   // Draw third tint
		//0,   // Set Z-Write

		// Scene 6: one ball hovering
		0,    // Background
		0, // set time
		0,   // draw background
		0,    // BALL
		-24, // z-Position of first ball
		-2, // y-Position of first ball
		-2, // x-Position of first ball
		0,   // Draw first ball

		// Scene 7: black ball hovering
		0,    // Background
		0, // set time
		8, // color
		0,   // draw background
		0,    // BALL
		-32, // z-Position of first ball
		-4, // y-Position of first ball
		0,   // Draw first ball
		0,    // TINT
		//0,   // Set Z-Write
		0, // Y-Rotation of tint
		-30, // z-Position of tint
		14, // Y-Transform of tint
		0,   // Draw first tint
		0, // Y-Rotation of tint
		24, // Y-Transform of tint
		0,   // Draw second tint
		10, // Y-Rotation of tint
		30, // Y-Transform of tint
		0,   // Draw third tint
		//0,   // Set Z-Write

		// Scene 10: Stone moving right
		0,    // Background
		0, // set time
		0,   // draw background
		0,    // BALL
		-48, // z-Position of first ball
		-5, // y-Position of first ball
		-8, // x-Position of first ball
		0,   // Draw first ball

		// Scene 11: Stone jumping up
		0,    // Background
		4, // set time
		0,   // draw background
		0,    // BALL
		-48, // z-Position of first ball
		14, // y-Position of second ball
		-5, // x-Position of second ball
		0,   // Draw second ball
		-5, // y-Position of first ball
		0, // x-Position of first ball
		0,   // Draw first ball
		0,    // TINT
		//0,   // Set Z-Write
		-46, // z-Position of first tint
		-5, // y-Position of first tint
		0, // x-Position of first tint
		0, // Y-Transform of tint
		0,   // Draw first tint
		4, // Y-Transform of tint
		0,   // Draw second tint
		//0,   // Set Z-Write

		// Scene 12: Final
		0,    // Background
		6, // set time
		0,   // draw background
		0,    // BALL
		-48, // z-Position of first ball
		7, // y-Position of second ball
		0, // x-Position of second ball
		0,   // Draw second ball
		0, // color of tint
		2, // y-Position of first ball
		5, // x-Position of first ball
		0,   // Draw first ball
		0,    // TINT
		//0,   // Set Z-Write
		-46, // z-Position of first tint

		32, // z-Rotation of first tint
		2, // y-Position of first tint
		5, // x-Pos
		8, // Y-Transform of tint
		0,   // Draw first tint
		0, // z-Rotation of first tint
		0, // color of tint

		-5, // y-Position of first tint
		0, // x-Position of first tint
		24, // Y-Transform of tint
		0,   // Draw first tint
		32, // Y-Transform of tint
		0,   // Draw second tint
		//0,   // Set Z-Write
	},

	// Param 3
	{
		// Scene 1: Floor sweep
		0, // set background shader
		64, // timeEnd
		-2, // rotation End
		-4, // rotation End
		0, // draw background

		// Scene 2: Sky sweep
		32, // timeEnd
		-2, // rotation End
		10, // rotation End
		0, // draw background

		// Scene 3: balls homing in
		0, // background shader
		16,  // time
		100, // draw background with size 6.25
		0, // ball shader
		-48, // z-position of ball
		-4, // x-position of ball
		0, // draw ball
		4, // x-position of ball
		0, // draw ball

		// Scene 4: Balls touching
		0, // background shader
		17, // timeEnd
		0, // draw background
		0, // ball shader
		-48, // z-position of ball
		-1, // y-position of ball
		-6, // x-position of ball
		0, // draw ball
		6, // x-position of ball
		1, // y-position of ball
		20, // z-rotation
		0, // draw ball		
		4, // x-position of tint
		0, // y-position of tint
		-46, // z-position of tint
		0, // tint shader
		//0, // Z-Write
		16, // Y-Transform of tint
		0, // draw tint
		13, // Y-Transform of tint
		0, // draw tint
		14, // color of tint
		-28, // z-rotation
		-4, // X-Positon of tint
		16, // Y-Transform of tint
		0, // draw tint
		13, // Y-Transform of tint
		0, // draw tint
		//0, // Z-Write

		// Scene 5: Balls hovering
		0, // background shader
		18, // timeEnd
		0, // draw background
		0, // ball shader
		-48, // z-position of ball
		-2, // y-position of ball
		-8, // x-position of ball
		0, // draw ball
		8, // x-position of ball
		2, // y-position of ball
		26, // z-rotation
		0, // draw ball		
		4, // x-position of tint
		0, // y-position of tint
		-46, // z-position of tint
		0, // tint shader
		//0, // Z-Write
		38, // Y-Transform of tint
		0, // draw tint
		25, // Y-Transform of tint
		0, // draw tint
		14, // color of tint
		-32, // z-rotation
		-4, // X-Positon of tint
		32, // Y-Transform of tint
		0, // draw tint
		22, // Y-Transform of tint
		0, // draw tint
		//0, // Z-Write

		// Scene 8: Falling stone
		0,    // Background
		2, // set time
		0,   // draw background
		0,    // BALL
		-32, // z-Position of first ball
		-4, // y-Position of first ball
		0,   // Draw first ball

		// Scene 9: black splashin
		0,    // Background
		2, // set time
		0, // color
		0,   // draw background
		0,    // BALL
		-32, // z-Position of first ball
		-4, // y-Position of first ball
		0,   // Draw first ball
		0,    // TINT
		//0,   // Set Z-Write
		-30, // Z-Position of tint
		24, // Y-Transform of tint	
		0,   // Draw first tint
		36, // Y-Transform of tint
		0,   // Draw second tint
		48, // Y-Transform of tint
		0,   // Draw third tint
		//0,   // Set Z-Write

		// Scene 6: one ball hovering
		0,    // Background
		2, // set time
		0,   // draw background
		0,    // BALL
		-12, // z-Position of first ball
		-2, // y-Position of first ball
		-2, // x-Position of first ball
		0,   // Draw first ball

		// Scene 7: black ball hovering
		0,    // Background
		32, // set time
		16, // color
		0,   // draw background
		0,    // BALL		
		-32, // z-Position of first ball
		-4, // y-Position of first ball
		0,   // Draw first ball
		0,    // TINT
		//0,   // Set Z-Write
		20, // Y-Rotation of tint
		-30, // z-Position of tint
		20, // Y-Transform of tint
		0,   // Draw first tint
		-20, // Y-Rotation of tint
		30, // Y-Transform of tint
		0,   // Draw second tint
		20, // Y-Rotation of tint
		36, // Y-Transform of tint
		0,   // Draw third tint
		//0,   // Set Z-Write

		// Scene 10: Stone moving right
		0,    // Background
		4, // set time
		0,   // draw background
		0,    // BALL
		-48, // z-Position of first ball
		-5, // y-Position of first ball
		0, // x-Position of first ball
		0,   // Draw first ball

		// Scene 11: Stone jumping up
		0,    // Background
		6, // set time
		0,   // draw background
		0,    // BALL
		-48, // z-Position of first ball
		7, // y-Position of second ball
		0, // x-Position of second ball
		0,   // Draw second ball
		2, // y-Position of first ball
		5, // x-Position of first ball
		0,   // Draw first ball
		0,    // TINT
		//0,   // Set Z-Write
		-46, // z-Position of first tint
		-5, // y-Position of first tint
		0, // x-Position of first tint
		24, // Y-Transform of tint
		0,   // Draw first tint
		32, // Y-Transform of tint
		0,   // Draw second tint
		//0,   // Set Z-Write
		
		// Scene 12: Final
		0,    // Background
		8, // set time
		0,   // draw background
		0,    // BALL
		-48, // z-Position of first ball
		9, // y-Position of second ball
		-2, // x-Position of second ball
		0,   // Draw second ball
		16, // color of tint
		0, // y-Position of first ball
		7, // x-Position of first ball
		0,   // Draw first ball
		0,    // TINT
		//0,   // Set Z-Write
		-46, // z-Position of first tint

		38, // z-Rotation of first tint
		2, // y-Position of first tint
		5, // x-Pos
		24, // Y-Transform of tint
		0,   // Draw first tint
		0, // z-Rotation of first tint
		0, // color of tint

		-5, // y-Position of first tint
		0, // x-Position of first tint
		28, // Y-Transform of tint
		0,   // Draw first tint
		38, // Y-Transform of tint
		0,   // Draw second tint
		//0,   // Set Z-Write
	},
};

// -------------------------------------------------------------------
//                          SHADER CODE:
// -------------------------------------------------------------------

#if 0
#pragma data_seg(".shader")
const GLchar *fragmentNoise="\
uniform sampler3D Texture0;\
vec4 noise(vec3 pos, int iterations, float reduction)\
{\
   float intensity = 1.0;\
   float size = 1.0;\
   vec4 result = vec4(0.0);\
\
   for (int k = 0; k < iterations; k++)\
   {\
      /*vec4 input = texture3D(Texture0, pos*size) - vec4(0.5, 0.5, 0.5, 0.0);*/\
	  vec4 input = texture3D(Texture0, pos*size);\
	  result += normalize(input) * intensity;\
      intensity *= reduction;\
      size *= 1.8;\
   }\
\
   return result;\
}";

#pragma data_seg(".shader")
const GLchar *fragmentMainTint="\
varying vec3 objectPosition;\
varying vec3 objectNormal;\
varying mat4 parameters;\
vec4 noise(vec3 pos, int iterations, float reduction);\
void main(void)\
{\
   vec3 relpos = objectPosition*0.5;\
   relpos.y -= parameters[1][2] * 0.1 + parameters[0][0] * 0.1;\
   vec3 pertubation = relpos + 0.04 * noise(relpos, 5, 0.6);\
   float bl = noise(pertubation, 8, 0.5).g;\
   float blackness = 1.0*smoothstep(0.0, 0.5, -objectPosition.y) - 0.6*abs(bl);\
   blackness = smoothstep(0.1, 0.8, -smoothstep(-1.0, 4.0, parameters[1][2]) + blackness * smoothstep(0.0, 0.5, abs(objectNormal.z)));\
   vec3 fgcolor = vec3(parameters[0][1] + bl);\
   gl_FragColor = vec4(fgcolor, blackness);\
   \
}";

#pragma data_seg(".shader")
const GLchar *fragmentMainObject="\
varying vec3 objectPosition;\
varying vec3 objectNormal;\
varying mat4 parameters;\
vec4 noise(vec3 pos, int iterations, float reduction);\
\
void main(void)\
{\
   vec3 relpos = 0.3 * objectPosition;\
   vec3 pertubation = relpos + 0.03 * noise(relpos + 0.02 * vec3(parameters[0][0]), 3, 0.7);\
   \
   float brightness = noise(pertubation, 8, 0.5).g * 0.3;\
   float color = abs(brightness * 3.0);\
   float whiteness = brightness;\
   \
   vec3 bumpNormal = normalize(objectNormal);\
   \
   float hemi = 0.5 + 0.5 * bumpNormal.y;\
   float hemiSpec = hemi * smoothstep(0.1, 0.7, whiteness);\
   \
   vec3 fgcolor = hemiSpec + hemi * (vec3(whiteness) + color * vec3(1.0, 0.5, 0.3));\
   gl_FragColor = vec4(fgcolor * (1.0-parameters[0][1]), 1.0);\
}";

#pragma data_seg(".shader")
const GLchar *fragmentMainBackground="\
varying vec3 objectPosition;\
varying vec3 objectNormal;\
varying mat4 parameters;\
vec4 noise(vec3 pos, int iterations, float reduction);\
\
void main(void)\
{\
   vec3 objPos = 5.0 * objectPosition;\
   vec3 pertubation = objPos;\
   vec3 noisePos = vec3(parameters[0][0]);\
   pertubation += 0.05 * noise(0.2*pertubation, 7, 0.7);\
   \
   /* trees */\
   float trees = 0.65 + 0.5 * objPos.y + 0.06 * noise(0.8 * pertubation - 0.05 * noisePos, 7, 0.7).x;\
   \
   /* moon */\
   /*float moon = 2.6 - distance(objPos, vec3(0.25, 0.0, -12.0));*/\
   float moon = 1.3 - distance(objPos, vec3(0.125, -1.0, -6.0));\
   \
   /* sky */\
   vec3 sky = cross(vec3(0.4, 0.0, 0.0), noise(pertubation - 0.03 * noisePos, 7, 0.7));\
   sky = smoothstep(-1.2, 1.0, -objPos.y) * (vec3(dot(vec3(1.0), moon + sky)) + vec3(2.4, 2.5, 3.0));\
   /*moon = 0.8 * smoothstep(0.25, 0.27, moon);*/\
   moon = 0.0*smoothstep(0.0, 0.4, moon);\
   \
   /* apply trees in front of sky */\
   vec3 color = (0.3+parameters[0][2]) * (moon + sky) * smoothstep(0.5, 0.7, trees) + vec3(smoothstep(-0.6, 0.0, -trees));\
   \
   gl_FragColor = vec4(color, 1.0);\
}";

#pragma data_seg(".shader")
const GLchar *vertexMainObject="\
varying vec3 objectPosition;\
varying vec3 objectNormal;\
varying mat4 parameters;\
\
void main(void)\
{\
   parameters = gl_ModelViewMatrix;\
   objectPosition = normalize(gl_Vertex.xyz);\
   vec4 newPos = gl_Vertex;\
   \
   float transformY = 4.0 * sin(parameters[1][2]);\
   newPos.y = (transformY + 1.0) * newPos.y + 0.25 * transformY;\
   float transformX = 0.4*(parameters[1][2]+1.0)*(parameters[1][2]) + 1.0;\
   newPos.x *= transformX;\
   newPos.z *= transformX;\
   \
   vec4 rotation1 = vec4(cos(parameters[1][0]), sin(parameters[1][0]), 0, 0);\
   vec4 rotation2 = vec4(-sin(parameters[1][0]), cos(parameters[1][0]), 0, 0);\
   newPos = vec4(dot(newPos, rotation1), dot(newPos, rotation2), newPos.z, newPos.w);\
\
   rotation1 = vec4(cos(parameters[1][1]), 0, sin(parameters[1][1]), 0);\
   rotation2 = vec4(-sin(parameters[1][1]), 0, cos(parameters[1][1]), 0);\
   newPos = vec4(dot(newPos, rotation1), newPos.y, dot(newPos, rotation2), newPos.w);\
   objectNormal = normalize(newPos.xyz);\
   \
   newPos = newPos + parameters[2];\
\
   gl_Position = gl_ProjectionMatrix*newPos;\
}";
#endif




#pragma data_seg(".shader")
const GLchar *fragmentNoise="\
uniform sampler3D t;\
vec4 n(vec3 p,int i,float r)\
{\
float n=1.0;\
float s=1.0;\
vec4 e=vec4(0.0);\
for(int k=0;k<i;k++)\
{\
e+=normalize(texture3D(t,p*s))*n;\
n*=r;\
s*=1.8;\
}\
return e;\
}";

#pragma data_seg(".shader")
const GLchar *fragmentMainTint="\
varying vec3 p;\
varying vec3 o;\
varying mat4 e;\
vec4 n(vec3,int,float);\
void main(void)\
{\
vec3 r=p*0.5;\
r.y-=0.1*(e[1][2]+e[0][0]);\
r+=0.04*n(r,5,0.6);\
float l=n(r,8,0.5).y;\
gl_FragColor = vec4(vec3(e[0][1]+l),smoothstep(0.1,0.8,-smoothstep(-1.0,4.0,e[1][2])+(smoothstep(0.0,0.5,-p.y)-0.6*abs(l))*smoothstep(0.0,0.5,abs(o.z))));\
}";

#pragma data_seg(".shader")
const GLchar *fragmentMainObject="\
varying vec3 p;\
varying vec3 o;\
varying mat4 e;\
vec4 n(vec3,int,float);\
void main(void)\
{\
float r=n(0.3*p+0.03*n(0.3*p+0.02*vec3(e[0][0]),3,0.7),8,0.5).y*0.3;\
gl_FragColor=vec4((1.0-e[0][1])*(0.5+0.5*o.y)*(vec3(r+smoothstep(0.1,0.7,r))+abs(r)*vec3(3.0,1.5,0.9)),1.0);\
}";

#pragma data_seg(".shader")
const GLchar *fragmentMainBackground="\
varying vec3 p;\
varying mat4 e;\
vec4 n(vec3,int,float);\
\
void main(void)\
{\
vec3 r=5.0*p+0.05*n(p,7,0.7);\
float t=0.65+(2.5*p+0.06*n(0.8*r-0.05*vec3(e[0][0]),7,0.7)).y;\
gl_FragColor=vec4(e[0][2]*smoothstep(0.24,-0.2,p.y)*(vec3(dot(vec3(5.0),0.26-distance(p,vec3(0.025,-0.2,-1.2))+cross(vec3(0.08,0.0,0.0),n(r-0.03*vec3(e[0][0]),7,0.7))))+vec3(2.4,2.5,3.0))*smoothstep(0.5,0.7,t)+vec3(smoothstep(0.6,0.0,t)),1.0);\
}";

#pragma data_seg(".shader")
const GLchar *vertexMainObject="\
varying vec3 p;\
varying vec3 o;\
varying mat4 e;\
\
void main(void)\
{\
e=gl_ModelViewMatrix;\
p=normalize(gl_Vertex.xyz);\
vec3 n=vec3(gl_Vertex.x*(0.4*e[1][2]*(e[1][2]+1.0)+1.0),(4.0*sin(e[1][2])+1.0)*gl_Vertex.y+sin(e[1][2]),gl_Vertex.z*(0.4*e[1][2]*(e[1][2]+1.0)+1.0));\
n=vec3(dot(n,vec3(cos(e[1][0]),sin(e[1][0]),0)),dot(n,vec3(-sin(e[1][0]),cos(e[1][0]),0)),n.z);\
n=vec3(dot(n,vec3(cos(e[1][1]),0,sin(e[1][1]))),n.y,dot(n,vec3(-sin(e[1][1]),0,cos(e[1][1]))));\
o=normalize(n);\
gl_Position=gl_ProjectionMatrix*(vec4(n,gl_Vertex.w)+e[2]);\
}";

/*const GLchar *shaderSources[5] = 
{
	vertexMainObject, fragmentNoise, fragmentMainObject, fragmentMainTint, fragmentMainBackground
};*/

// -------------------------------------------------------------------
//                          Constants:
// -------------------------------------------------------------------

#define fzn  0.125f
#define fzf  16.0f

#pragma data_seg(".projectionMatrix")
float projectionMatrix[16] = {
    4.0f, 0.00f,  0.0f,                    0.0f,
	0.0f, 8.0f,  0.0f,                    0.0f,
    0.0f, 0.00f, -(fzf+fzn)/(fzf-fzn),    -1.0f,
    0.0f, 0.00f, -2.0f*fzf*fzn/(fzf-fzn),  0.0f };

#pragma data_seg(".glNames")
const static char* glnames[]={
     "glCreateShader", "glCreateProgram", "glShaderSource", "glCompileShader", 
     "glAttachShader", "glLinkProgram", "glUseProgram",
	 "glTexImage3D"
};

// The model matrix is used to send the parameters to the hardware...
static float parameterMatrix[16];

// Name of the 32x32x32 noise texture
#define FLOAT_TEXTURE
#define NOISE_TEXTURE_SIZE 8 // try smaller?
static GLuint noiseTexture;
#ifdef FLOAT_TEXTURE
static float noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];
#else
static unsigned char noiseData[NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4];
#endif
static int noiseTmp[4];

typedef void (*GenFP)(void); // pointer to openGL functions
static GenFP glFP[8]; // pointer to openGL functions
static GLuint shaderPrograms[3];

// -------------------------------------------------------------------
//                          Code:
// -------------------------------------------------------------------

#pragma code_seg(".introInit")
void intro_init( void )
{
	// create openGL functions
	for (int i=0; i<8; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);

	// create noise Texture
#ifdef FLOAT_TEXTURE
	for (int i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData[i] = frand() - 0.5f;
	}
#else
	for (int i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE * 4; i++)
	{
		noiseData[i] = (unsigned char)rand();
	}
#endif

	// Create and link shader and stuff:
	// I will have to separate these to be able to use more than one shader...
	// TODO: I should make some sort of compiling and linking loop...
	
	// init objects:	
	GLuint shaders[5];	
	shaders[0] = ((PFNGLCREATESHADERPROC)(glFP[0]))(GL_VERTEX_SHADER);	
	for (int i = 1; i < 5; i++)
	{
		shaders[i] = ((PFNGLCREATESHADERPROC)(glFP[0]))(GL_FRAGMENT_SHADER);
	}
	/*GLuint fNoise = ((PFNGLCREATESHADERPROC)(glFP[0]))(GL_FRAGMENT_SHADER);	
	GLuint fMainBackground = ((PFNGLCREATESHADERPROC)(glFP[0]))(GL_FRAGMENT_SHADER);	
	GLuint fMainObject = ((PFNGLCREATESHADERPROC)(glFP[0]))(GL_FRAGMENT_SHADER);	
	GLuint fMainTint = ((PFNGLCREATESHADERPROC)(glFP[0]))(GL_FRAGMENT_SHADER);*/

	for (int i = 0; i < 3; i++)
	{
		shaderPrograms[i] = ((PFNGLCREATEPROGRAMPROC)glFP[1])();
	}
	
	/*for (int i = 0; i < 5; i++)
	{
		((PFNGLSHADERSOURCEPROC)glFP[2]) (shaders[i], 1, &(shaderSources[i]), NULL);
		((PFNGLCOMPILESHADERPROC)glFP[3])(shaders[i]);
	}*/
	((PFNGLSHADERSOURCEPROC)glFP[2]) (shaders[0], 1, &vertexMainObject, NULL);
	((PFNGLCOMPILESHADERPROC)glFP[3])(shaders[0]);
	((PFNGLSHADERSOURCEPROC)glFP[2]) (shaders[1], 1, &fragmentNoise, NULL);
	((PFNGLCOMPILESHADERPROC)glFP[3])(shaders[1]);
	((PFNGLSHADERSOURCEPROC)glFP[2]) (shaders[2], 1, &fragmentMainObject, NULL);
	((PFNGLCOMPILESHADERPROC)glFP[3])(shaders[2]);
	((PFNGLSHADERSOURCEPROC)glFP[2]) (shaders[3], 1, &fragmentMainTint, NULL);
	((PFNGLCOMPILESHADERPROC)glFP[3])(shaders[3]);
	((PFNGLSHADERSOURCEPROC)glFP[2]) (shaders[4], 1, &fragmentMainBackground, NULL);
	((PFNGLCOMPILESHADERPROC)glFP[3])(shaders[4]);

	// link shaders:
	for (int i = 0; i < 3; i++)
	{
		((PFNGLATTACHSHADERPROC)glFP[4])(shaderPrograms[i], shaders[0]);
		((PFNGLATTACHSHADERPROC)glFP[4])(shaderPrograms[i], shaders[1]);
		((PFNGLATTACHSHADERPROC)glFP[4])(shaderPrograms[i], shaders[i+2]);
		((PFNGLLINKPROGRAMPROC)glFP[5])(shaderPrograms[i]);
	}

	// Set texture.
	glEnable(GL_TEXTURE_3D); // automatic?
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	//glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, 0, GL_RGBA, 
	//	         GL_UNSIGNED_BYTE, noiseData);
#ifdef FLOAT_TEXTURE
	((PFNGLTEXIMAGE3DPROC) glFP[7])(GL_TEXTURE_3D, 0, GL_RGBA16F,
									NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
									0, GL_RGBA, GL_FLOAT, noiseData);
#else
	((PFNGLTEXIMAGE3DPROC) glFP[7])(GL_TEXTURE_3D, 0, GL_RGBA8,
									NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE,
									0, GL_RGBA, GL_UNSIGNED_BYTE, noiseData);
#endif

	// RLY?
	//glEnable(GL_CULL_FACE);
}

#pragma code_seg(".introDo")
void intro_do( long itime )
{
    //float ftime = 0.001f*(float)itime;
	float value;
	GLUquadric* quad = gluNewQuadric();

    // render
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

	// clear screan:
	//glClear(GL_DEPTH_BUFFER_BIT);

	//glBindTexture(GL_TEXTURE_3D, noiseTexture); // 3D noise?	

	/* Set everything to beginning */
	for (int i = 0; i < 16; i++)
	{
		parameterMatrix[i] = 0.0f;
	}

	/* Get music loudness */
	int musicPos = itime * 441 / 5; /* delay adjustment? */
	int volume = 0;
	for (int i = musicPos; i < musicPos + 1024; i++)
	{
		volume += abs(myMuzik[i]);
	}	
	parameterMatrix[2] = (float)(volume) * (1.0f/(32768.0f*4096.0f)) + 0.3f;

	/* choose scenery (looping) */
	int scene = 0;
	int curTime = itime;
	int startEffect = 0;
	while (curTime > (int)sceneDuration[scene] * 128)
	{
			curTime -= (int)sceneDuration[scene] * 128;			
			startEffect += sceneNumEffects[scene];
			scene++;

			//TODO: remove
			if (scene >= NUM_SCENES) 
			{
				scene = 0;
				startEffect = 0;
			}
	}

	/* Go through scene script */
	for (int effect = startEffect; effect < startEffect + sceneNumEffects[scene]; effect++)
	{
		switch (sceneEffect[effect])
		{
		case EFFECT_SET_PARAMETER:
			value = float(((int)sceneDuration[scene]*128-curTime)*sceneParam[1][effect] +
					      curTime * sceneParam[2][effect]) / (float)(128*(int)sceneDuration[scene]) * (1.0f / 16.0f);
			parameterMatrix[sceneParam[0][effect]] = value;
			break;

		case EFFECT_DRAW_SPHERE:
			glMatrixMode(GL_PROJECTION);	
			glLoadMatrixf(projectionMatrix);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(parameterMatrix);
			gluSphere(quad, 0.0625f*sceneParam[0][effect], 128, 128);
			break;

		case EFFECT_SET_SHADER:
			((PFNGLUSEPROGRAMPROC) glFP[6])(shaderPrograms[sceneParam[0][effect]]);	
			break;

/*		case EFFECT_SET_Z_WRITE:
			glDepthMask(sceneParam[0][effect]);
			break;*/
		}
	}

#if 0
	while (ftime > 20.0f) ftime -= 20.0f;
	if (ftime < 5.0f)
	{
		morphingBall(ftime + 10.0f);
	}
	else if (ftime < 10.0f)
	{
		fallingBall(ftime - 5.0f);
	}
	else if (ftime < 15.0f)
	{
		morphingBall(ftime + 50.0f);
	}
	else
	{
		morphingBall(ftime + 150.0f);
	}
#endif

	/*while (ftime > 5.0f) ftime -= 5.0f;
	fallingBall(ftime);*/

	//while (ftime > 200.0f) ftime -= 200.0f;
	//morphingBall(ftime);
}

