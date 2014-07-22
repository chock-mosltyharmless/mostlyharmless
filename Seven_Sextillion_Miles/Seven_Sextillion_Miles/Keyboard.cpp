#include "StdAfx.h"
#include "Keyboard.h"


Keyboard::Keyboard(void)
{
	for (int i = 0; i < LAST_ID; i++)
	{
		keyState[i] = false;
	}
}


Keyboard::~Keyboard(void)
{
}

void Keyboard::prepare(void)
{
	keyState[UP] = GetAsyncKeyState('w') || GetAsyncKeyState('W');
	keyState[DOWN] = GetAsyncKeyState('s') || GetAsyncKeyState('S');
	keyState[LEFT] = GetAsyncKeyState('a') || GetAsyncKeyState('A');
	keyState[RIGHT] = GetAsyncKeyState('d') || GetAsyncKeyState('D');
}

bool Keyboard::keyDown(int key)
{
	if (key < 0 || key > LAST_ID) return false; // Error?

	return keyState[key];
}