#include "StdAfx.h"
#include "GameClock.h"


GameClock::GameClock(void)
{
	reset();
}


GameClock::~GameClock(void)
{
}

void GameClock::reset(void)
{
	if (!QueryPerformanceCounter(&lastQPCTick))
	{
		// Invalidate counter...
		lastQPCTick.QuadPart = 0x7FFFFFFFFFFFFFFF;
	}
	lastTGTTick = timeGetTime();
}

float GameClock::getDeltaTime(void)
{
	// First try to get the time using QueryPerformanceCounter
	LARGE_INTEGER newQPCTick, qpcFrequency;
	float qpcDeltaTime;
	if (!QueryPerformanceCounter(&newQPCTick) ||
		!QueryPerformanceFrequency(&qpcFrequency))
	{
		qpcDeltaTime = -1.0f; // invalidate
		lastQPCTick.QuadPart = 0x7FFFFFFFFFFFFFFF;
	}
	else
	{
		LARGE_INTEGER deltaTick;
		deltaTick.QuadPart = newQPCTick.QuadPart - lastQPCTick.QuadPart;
		lastQPCTick.QuadPart = newQPCTick.QuadPart;
		qpcDeltaTime = (float)deltaTick.QuadPart / (float)qpcFrequency.QuadPart;
	}

	DWORD newTGTTick = timeGetTime();
	DWORD deltaTGTTick = newTGTTick - lastTGTTick;
	lastTGTTick = newTGTTick;
	float tgtDeltaTime = (float)deltaTGTTick * 0.001f;

	// Check whether qpcDeltaTime is within reasonable bounds
	// I use 50 ms... That is quite a lot???
	float resultTime = qpcDeltaTime;
	if (qpcDeltaTime < 0.0f && fabsf(qpcDeltaTime - tgtDeltaTime) > 0.05f)
	{
		resultTime = tgtDeltaTime;
	}

	// Maximum time step of half a second...
	if (resultTime > 0.5f) resultTime = 0.5f;

	return resultTime;
}

float getDeltaTime(void);