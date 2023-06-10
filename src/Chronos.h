#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <optional>
#include <format>
#include <nlohmann/json.hpp>

class Timer {
	friend class Timers;
	std::chrono::seconds duration;
	// TODO: Time left represents the time that PASSED not left
	std::chrono::milliseconds timeLeft;
	std::vector<std::string> days;

	std::chrono::steady_clock::time_point lastChecked;
	Timer(std::string name, std::chrono::seconds dur): name(name), timeLeft(std::chrono::milliseconds(0)),duration(dur),days() {
		lastChecked = std::chrono::steady_clock::now();
	}
	Timer(const nlohmann::json& j) {
		j.at("name").get_to(name);
		duration = std::chrono::seconds(j.at("duration"));
		timeLeft = std::chrono::seconds(j.at("timeLeft"));
		j.at("days").get_to(days);
	}
public:
	std::string name;
	float getDuration() const {
		return duration.count();
	}

	float getLeftTime() const {
		return std::chrono::duration_cast<std::chrono::seconds>(timeLeft).count();
	}

	auto operator<=>(const Timer&) const = default;

	void toJson(nlohmann::json& j) const {
		j = nlohmann::json{ 
			{"name", name}, 
			{"duration", duration.count()}, 
			{"timeLeft", std::chrono::duration_cast<std::chrono::seconds>(timeLeft).count()},
			{"type", "daily"},
			{"days",days}
		};
	}
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
		activeTimer.timeLeft += std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now() - activeTimer.lastChecked));
		activeTimer.lastChecked = std::chrono::steady_clock::now();
		if (activeTimer.timeLeft >= activeTimer.duration) {
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

	void loadTimers(const nlohmann::json& j) {
		timers.push_back(Timer(j));
	}

	void saveTimers() {

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
