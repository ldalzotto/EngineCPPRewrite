#pragma once

struct Clock
{
	uimax framecount;
	float deltatime;

	inline static Clock allocate_default()
	{
		return Clock{ 0,0.0f };
	};

	inline void newframe()
	{
		this->framecount += 1;
	}

	inline void newupdate(float p_delta)
	{
		this->deltatime = p_delta;
	}
};

// typedef uint64 time_t;


time_t clock_currenttime_mics();

#ifdef _WIN32

#include <sysinfoapi.h>

inline time_t clock_currenttime_mics()
{
	FILETIME l_currentTime;
	GetSystemTimeAsFileTime(&l_currentTime);
	return FILETIME_to_mics(l_currentTime);
};

#endif // _WIN32