#include <TimeSystem.h>

using namespace std::chrono;

TimePoint::TimePoint():
TimePoint(system_clock::from_time_t(0)) { }

TimePoint::TimePoint(const time_point<system_clock>& time):
time(time),
millis(duration_cast<milliseconds>(hh_mm_ss(time - floor<std::chrono::days>(time)).subseconds()).count()),
seconds(hh_mm_ss(time - floor<std::chrono::days>(time)).seconds().count()),
minutes(hh_mm_ss(time - floor<std::chrono::days>(time)).minutes().count()),
hours(hh_mm_ss(time - floor<std::chrono::days>(time)).hours().count()),
day((unsigned int) year_month_day(floor<std::chrono::days>(time)).day()),
weekDay(weekday(floor<std::chrono::days>(time)).iso_encoding()),
month((unsigned int) year_month_day(floor<std::chrono::days>(time)).month()),
year((int) year_month_day(floor<std::chrono::days>(time)).year()) { }

TimePoint Time::startup;
TimePoint Time::now;
float Time::applicationTime;
float Time::deltaTime;
float Time::timeScale;
float Time::unscaledDeltaTime;

void Time::Update() {
	auto newNow = TimePoint(system_clock::now());

	if (startup.GetTime().time_since_epoch().count() == 0) {
		startup = newNow;
		now = newNow;
	}

    unscaledDeltaTime = duration<float>(newNow.GetTime() - now.GetTime()).count();
    deltaTime = unscaledDeltaTime * timeScale;
    applicationTime += deltaTime;

	now = newNow;
}

const TimePoint& Time::Now() { return now; }
TimePoint Time::SystemTime() { return TimePoint(system_clock::now()); }
float Time::Current() { return applicationTime; }
float Time::Delta() { return deltaTime; }
void Time::SetTimeScale(float scale) { timeScale = scale; }
float Time::GetTimeScale() { return timeScale; }
float Time::UnscaledDelta() { return unscaledDeltaTime; }
