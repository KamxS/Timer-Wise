#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <optional>
#include <format>
#include <ctime>
#include <nlohmann/json.hpp>

// Remove !
#include <iostream>

class Timer {
	friend class Timers;
	std::chrono::seconds duration;
	std::chrono::milliseconds timePassed;
	std::vector<std::string> days;

	std::chrono::steady_clock::time_point lastChecked;
	Timer(std::string name, std::chrono::seconds dur): name(name), timePassed(std::chrono::milliseconds(0)),duration(dur),days() {
		lastChecked = std::chrono::steady_clock::now();
	}
	Timer(const nlohmann::json& j) {
		j.at("name").get_to(name);
		duration = std::chrono::seconds(j.at("duration"));
		timePassed = std::chrono::seconds(j.at("timePassed"));
		j.at("days").get_to(days);
	}
public:
	std::string name;
	float getDuration() const {
		return duration.count();
	}

	float getTimePassed() const {
		return std::chrono::duration_cast<std::chrono::seconds>(timePassed).count();
	}

	auto operator<=>(const Timer&) const = default;

	// TODO: Figure out how to change the order of attributes
	nlohmann::json toJson() const {
		return nlohmann::json{ 
			{"name", name}, 
			{"duration", duration.count()}, 
			{"timePassed", std::chrono::duration_cast<std::chrono::seconds>(timePassed).count()},
			{"type", "daily"},
			{"days",days}
		};
	}
};


class Timers {
	size_t activeTimerInd;
	std::vector<Timer> timers;
	int day;
	
	size_t get(const std::string name) const {
		auto it = std::find_if(timers.begin(), timers.end(), [&name](const Timer& timer) {return timer.name == name; });
		if (it != timers.end()) return std::distance(timers.begin(), it);
		return -1;
	}

public: 
	Timers(): activeTimerInd(-1), timers(), day(0) {};

	void Update() {
		if (activeTimerInd == -1) {
			checkTime();
			return;
		};
		Timer& activeTimer = timers[activeTimerInd];
		activeTimer.timePassed += std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now() - activeTimer.lastChecked));
		activeTimer.lastChecked = std::chrono::steady_clock::now();
		if (activeTimer.timePassed >= activeTimer.duration) {
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
	
	void resetTimers() {
		for (auto& t : timers) {
			t.timePassed = std::chrono::milliseconds(0);
		}
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
	
	// TODO: Remove chronos ???
	/*
		auto t1 = time(0);
		std::this_thread::sleep_for(std::chrono::seconds(5));
		auto t2 = time(0);
		std::cout << difftime(t2, t1) << std::endl;
	*/

	void checkTime() {
		time_t curtime = time(0);
		tm buf;
		localtime_s(&buf, &curtime);
		int curDay = buf.tm_yday;
		if (curDay == day) return;
		resetTimers();
		day = curDay;

		
	}
	
	void loadDays(const std::filesystem::path& path) {
		std::fstream f(path);
		if (!f.is_open()) return;

		const size_t length = 3;
		char buf[length] = "";
		f.read(buf, length);
		day = atoi(buf);
		f.close();
		checkTime();
	}

	void saveDays(const std::filesystem::path& path) {
		// TODO: Better saving
		std::fstream f(path);
		if (!f.is_open()) return;
		f << day;
		f.close();
	}
};

