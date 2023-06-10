#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <optional>
#include <format>
#include <nlohmann/json.hpp>

// Remove !
#include <iostream>

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

	// TODO: Figure out how to change the order of attributes
	nlohmann::json toJson() const {
		return nlohmann::json{ 
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
	Timers(): activeTimerInd(-1), timers() {};

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

	void loadTimers(const std::filesystem::path& path) {
		std::fstream f(path);
		if (!f.is_open()) return;
		nlohmann::json json = nlohmann::json::parse(f);
		for (auto& j : json) {
			timers.push_back(Timer(j));
		}
		f.close();
	}

	void saveTimers(const std::filesystem::path& path) const {
		// TODO: Better saving
		std::fstream f(path);
		if (!f.is_open()) return;

		std::vector<nlohmann::json> jsonVec{};
		for (auto& t : getTimers()) {
			jsonVec.push_back(t.toJson());
		}
		nlohmann::json json(jsonVec);
		f << json.dump(4);
		f.close();
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

