#ifndef TIMER_H
#define TIMER_H

#include <cstdint>

#include <SDL3/SDL_timer.h>

class Timer {
private:
	uint64_t _now;
	uint64_t _last;
	float _deltaTime;

public:
	void tick() {
		_last = _now;
		_now = SDL_GetPerformanceCounter();
		_deltaTime = ((_now - _last) / (float)SDL_GetPerformanceFrequency());
	}

	float deltaTime() const {
		return _deltaTime;
	}

	Timer() {
		_now = SDL_GetPerformanceCounter();
	}
};

#endif // !TIMER_H
