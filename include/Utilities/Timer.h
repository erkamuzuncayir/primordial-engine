#pragma once
#include <chrono>

namespace PE::Utilities {
class Timer {
public:
	Timer();
	Timer(const Timer &)			= delete;
	Timer &operator=(const Timer &) = delete;
	Timer(Timer &&)					= delete;
	Timer &operator=(Timer &&)		= delete;

	~Timer() {}

	[[nodiscard]] float TotalTime() const;	// in seconds
	[[nodiscard]] float DeltaTime() const;	// in seconds

	void Reset();  // Call before message loop.
	void Start();  // Call when unpaused.
	void Stop();   // Call when paused.
	void Tick();   // Call every frame.

private:
	std::chrono::steady_clock::time_point m_baseTime;
	std::chrono::steady_clock::time_point m_stopTime;
	std::chrono::steady_clock::time_point m_prevTime;
	std::chrono::steady_clock::duration	  m_pausedTimeSum;

	double m_deltaTime;
	bool   m_stopped;
};
}  // namespace PE::Utilities