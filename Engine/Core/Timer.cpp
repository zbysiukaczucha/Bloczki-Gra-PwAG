#include "Timer.h"

#include <algorithm>

#include "Application.h"

// Cap delta time to a minimum of 30fps
constexpr double DELTA_CAP = 1.0 / 30.0;

Timer::Timer() :
	secondsPerCount_(0.0),
	deltaTime_(0.0),
	baseTime_(0),
	pausedTime_(0),
	stopTime_(0),
	prevTime_(0),
	currentTime_(0),
	isPaused_(false)
{
	LARGE_INTEGER countsPerSecond;
	QueryPerformanceFrequency(&countsPerSecond);
	secondsPerCount_ = 1.0l / static_cast<double>(countsPerSecond.QuadPart);

#ifdef _DEBUG
	// checking if the cast is good, just a learning experience

	std::int64_t cStyleTester;
	QueryPerformanceFrequency((LARGE_INTEGER*)&cStyleTester);

	std::int64_t cppStyleTester;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&cppStyleTester));

	int breakpoint = 0;
#endif
}

double Timer::GetTotalTime() const
{
	if (isPaused_)
	{
		return ((stopTime_ - pausedTime_) - baseTime_) * secondsPerCount_;
	}
	else
	{
		return ((currentTime_ - pausedTime_) - baseTime_) * secondsPerCount_;
	}
}

float Timer::GetDeltaTime() const
{
	return static_cast<float>(deltaTime_);
}

void Timer::Reset()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	baseTime_ = currentTime.QuadPart;
	prevTime_ = currentTime.QuadPart;
	stopTime_ = 0;
	isPaused_ = false;
}

void Timer::Start()
{
	if (isPaused_)
	{
		LARGE_INTEGER startTime;
		QueryPerformanceCounter(&startTime);

		pausedTime_ += (startTime.QuadPart - stopTime_);
		stopTime_	 = 0;
		isPaused_	 = false;
	}
}

void Timer::Pause()
{
	if (!isPaused_)
	{
		LARGE_INTEGER currTime;
		QueryPerformanceCounter(&currTime);
		stopTime_ = currTime.QuadPart;
		isPaused_ = true;
	}
}

void Timer::Tick()
{
	if (isPaused_)
	{
		deltaTime_ = 0.0;
		return;
	}
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	currentTime_ = currentTime.QuadPart;

	deltaTime_ = (currentTime_ - prevTime_) * secondsPerCount_;
	prevTime_  = currentTime_;

	deltaTime_ = std::max<double>(deltaTime_, 0.0);		  // ensure this is non-negative
	deltaTime_ = std::min<double>(deltaTime_, DELTA_CAP); // ensure delta-time isn't larger than 1/30 of a second
}

void Timer::TickUncapped()
{
	if (isPaused_)
	{
		deltaTime_ = 0.0;
		return;
	}
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	currentTime_ = currentTime.QuadPart;

	deltaTime_ = (currentTime_ - prevTime_) * secondsPerCount_;
	prevTime_  = currentTime_;

	deltaTime_ = std::max<double>(deltaTime_, 0.0); // ensure this is non-negative
}
