#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <optional>

class Timer {
	friend class Timers;
	std::chrono::seconds duration;
	std::chrono::milliseconds current;

	std::chrono::steady_clock::time_point lastChecked;
	Timer(std::string name, std::chrono::seconds dur): name(name), current(std::chrono::milliseconds(0)),duration(dur) {
		lastChecked = std::chrono::steady_clock::now();
	}
public:
	std::string name;
	float getDuration() const {
		return duration.count();
	}

	float getCurrent() const {
		return std::chrono::duration_cast<std::chrono::seconds>(current).count();
	}

	auto operator<=>(const Timer&) const = default;
};


class Timers {
	size_t activeTimerInd;
	std::vector<Timer> timers;
	
	size_t get(const std::string name) const {
		auto it = std::find_if(timers.begin(), timers.end(), [&name](const Timer& timer) {return timer.name == name; });
		if (it != timers.end()) return std::distance(timers.begin(), it);
		return -1;
	}

public: 
	Timers() = default;

	void Update() {
		if (activeTimerInd == -1) return;
		Timer& activeTimer = timers[activeTimerInd];
		activeTimer.current += std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now() - activeTimer.lastChecked));
		activeTimer.lastChecked = std::chrono::steady_clock::now();
		if (activeTimer.current >= activeTimer.duration) {
			stopTimer();
		}
	}

	void newTimer(std::string name, std::chrono::seconds duration) {
		if (get(name) != -1 ) return;
		timers.push_back(Timer(name, duration));
	}

	void startTimer(const std::string name) {
		size_t ind = get(name);
		if (ind == -1) return;
		timers[ind].lastChecked = std::chrono::steady_clock::now();
		activeTimerInd = ind;
	}

	void stopTimer() {
		activeTimerInd = -1;
	}

	const std::optional<Timer> getActiveTimer() const {
		if (activeTimerInd == -1) {
			return std::nullopt;
		}
		return std::make_optional(timers[activeTimerInd]);
	}

	const std::vector<Timer> getTimers() const {
		return timers;
	}
};


/*
	timer {
		time: 60
		timeLeft: 32
		type: daily
		days: ["Monday","Friday"]
	},
	timer2 {
		time: 60
		timeLeft: 32
		type: daily
		days: [] // Everyday
	},
	timer3 {
		time: 260 
		timeLeft: 120 
		type: weekly 
		days: [] 
	}

*/
