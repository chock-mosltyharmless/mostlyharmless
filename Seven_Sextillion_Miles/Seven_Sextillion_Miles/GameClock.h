#pragma once

#include "stdafx.h"
#include <Winbase.h>
#include <Mmsystem.h>

class GameClock
{
public:
	GameClock(void);
	virtual ~GameClock(void);

	/// Set the counters to zero (should be done after regaining focus?)
	void reset(void);

	/// Get delta time in seconds and reset ticks
	float getDeltaTime(void);

private:
	LARGE_INTEGER lastQPCTick;
	DWORD lastTGTTick;
};

