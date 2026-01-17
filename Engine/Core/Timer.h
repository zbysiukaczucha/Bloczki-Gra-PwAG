#pragma once
#include <cstdint>

class Timer
{
public:
	Timer();

	double GetTotalTime() const;
	float  GetDeltaTime() const;

	void Reset();
	void Start();
	void Pause();
	void Tick();

	// Doesn't cap DT to a minimum of 30fps. Use for performance measurements
	void TickUncapped();

private:
	double secondsPerCount_;
	double deltaTime_;

	std::int64_t baseTime_;
	std::int64_t pausedTime_;
	std::int64_t stopTime_;
	std::int64_t prevTime_;
	std::int64_t currentTime_;

	bool isPaused_;
};
