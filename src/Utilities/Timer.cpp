#include <Utilities/Timer.h>

namespace PE::Utilities {
Timer::Timer() : m_pausedTimeSum(std::chrono::steady_clock::duration::zero()), m_deltaTime(-1.0), m_stopped(false) {
	Reset();
}

float Timer::TotalTime() const {
	auto endTime	   = m_stopped ? m_stopTime : std::chrono::steady_clock::now();
	auto totalDuration = (endTime - m_baseTime) - m_pausedTimeSum;
	return std::chrono::duration<float>(totalDuration).count();
}

float Timer::DeltaTime() const { return static_cast<float>(m_deltaTime); }

void Timer::Reset() {
	const auto now	= std::chrono::steady_clock::now();
	m_baseTime		= now;
	m_prevTime		= now;
	m_stopTime		= now;
	m_pausedTimeSum = std::chrono::steady_clock::duration::zero();
	m_stopped		= false;
}

void Timer::Start() {
	if (m_stopped) {
		auto startTime = std::chrono::steady_clock::now();

		m_pausedTimeSum += (startTime - m_stopTime);

		m_prevTime = startTime;
		m_stopped  = false;
	}
}

void Timer::Stop() {
	if (!m_stopped) {
		m_stopTime = std::chrono::steady_clock::now();
		m_stopped  = true;
	}
}

void Timer::Tick() {
	if (m_stopped) {
		m_deltaTime = 0.0;
		return;
	}

	auto currTime = std::chrono::steady_clock::now();

	m_deltaTime = std::chrono::duration<double>(currTime - m_prevTime).count();

	m_prevTime = currTime;

	if (m_deltaTime < 0.0) m_deltaTime = 0.0;
}
}  // namespace PE::Utilities