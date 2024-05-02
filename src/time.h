#ifndef TIME_H
#define TIME_H

#include <cstdint>

#include <SDL2/SDL_timer.h>

class Time {
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

	Time() {
		_now = SDL_GetPerformanceCounter();
	}
};

#endif // !TIME_H
