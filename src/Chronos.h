#pragma once

#include <string>
#include <chrono>
#include <map>
#include <set>
#include <ranges>


class Serializable {

};

class Deserializable {

};

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
};

class Timers {
	Timer* activeTimer;
	std::map<std::string, Timer*> timers;
public: 
	Timers() = default;

	void Update() {
		if (activeTimer == nullptr) return;
		activeTimer->current += std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now() - activeTimer->lastChecked));
		activeTimer->lastChecked = std::chrono::steady_clock::now();
		if (activeTimer->current >= activeTimer->duration) {
			stopTimer();
		}
	}

	void newTimer(std::string name, std::chrono::seconds duration) {
		if (timers.contains(name)) return;
		auto t = new Timer(name, duration);
		timers.insert(std::make_pair(name, t));
	}

	void startTimer(std::string name) {
		if (activeTimer != nullptr) return;
		activeTimer = timers[name];
		activeTimer->lastChecked = std::chrono::steady_clock::now();
	}

	void stopTimer() {
		activeTimer = nullptr;
	}

	const Timer* getActiveTimer() const {
		return activeTimer;
	}

	const std::map<std::string, Timer*> getTimers() const {
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
