#pragma once

#include <chrono>

class TimePoint {
	std::chrono::time_point<std::chrono::system_clock> time;

	uint16_t millis;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t weekDay;
	uint8_t month;
	int16_t year;
public:
	TimePoint();
	TimePoint(const std::chrono::time_point<std::chrono::system_clock>& time);

	inline std::chrono::time_point<std::chrono::system_clock> GetTime() const {
		return time;
	}

	inline int Milliseconds() const {
		return millis;
	}
	inline int Seconds() const {
		return seconds;
	}
	inline int Minutes() const {
		return minutes;
	}
	inline int Hours() const {
		return hours;
	}
	inline int Day() const {
		return day;
	}
	inline int WeekDay() const {
		return weekDay;
	}
	inline int Month() const {
		return month;
	}
	inline int Year() const {
		return year;
	}
};

class Time {
	friend class Engine;
private:
	static TimePoint startup;
	static TimePoint now;
	static float applicationTime;
	static float deltaTime;

    static float timeScale;
    static float unscaledDeltaTime;
public:
	static const TimePoint& Now();
	static TimePoint SystemTime();
	static float Current();
	static float Delta();

    static void SetTimeScale(float scale);
    static float GetTimeScale();
    static float UnscaledDelta();

    static void Update();
};
