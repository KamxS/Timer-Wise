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

// TODO: Figure out Program name and add a namespace
namespace TimerWise{}

struct Color {
public:
	float r;
	float g;
	float b;
	Color() : r(0), g(0), b(180) {}
	Color(float* arr) : r(arr[0]), g(arr[1]), b(arr[2]) {}
	Color(float r, float g, float b) : r(r), g(g), b(b) {}
};

const std::string DaysOfWeek[7] = {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};

class Timer {
	friend class Timers;
	std::chrono::seconds duration;
	std::chrono::milliseconds timePassed;
	std::vector<std::string> days;

	std::chrono::steady_clock::time_point lastChecked;
	Timer(std::string name, std::chrono::seconds dur, Color c, std::vector<std::string> days) : name(name), timePassed(std::chrono::milliseconds(0)), duration(dur), days(days), timerColor(c) {
		lastChecked = std::chrono::steady_clock::now();
	}
	Timer(const nlohmann::json& j) {
		j.at("name").get_to(name);
		auto colorsJson = j.at("color");
		timerColor = Color{colorsJson[0],colorsJson[1],colorsJson[2]};
		duration = std::chrono::seconds(j.at("duration"));
		timePassed = std::chrono::seconds(j.at("timePassed"));
		j.at("days").get_to(days);
	}

	// TODO: Figure out how to change the order of attributes
	nlohmann::json toJson() const {
		return nlohmann::json{ 
			{"name", name}, 
			{"duration", duration.count()}, 
			{"timePassed", std::chrono::duration_cast<std::chrono::seconds>(timePassed).count()},
			{"type", "daily"},
			{"color", nlohmann::json::array({timerColor.r, timerColor.g, timerColor.b})},
			{"days",days}
		};
	}
public:
	std::string name;
	Color timerColor;
	float getDuration() const {
		return duration.count();
	}

	float getTimePassed() const {
		return std::chrono::duration_cast<std::chrono::seconds>(timePassed).count();
	}

	// TODO: Figure out comparison
	//bool operator==(std::unordered_map<std::string, T> l) const {}
	//bool operator==(std::unitialized_list<std::string, T> l) const {}
};


class Timers {
	// TODO: It should not be an int
	int activeTimerInd;
	std::vector<Timer> timers;
	int day;
	
	size_t get(const std::string name) const {
		for (auto ind = 0; ind < timers.size(); ind++) {
			if (timers[ind].name == name) return ind;
		}
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

	int newTimer(std::string name, std::chrono::seconds duration, Color c, std::vector<std::string> days) {
		if (get(name) != -1 ) return 1;
		timers.push_back(Timer(name, duration,c, days));
		return 0;
	}

	void startTimer(const std::string name) {
		size_t ind = get(name);
		if (ind == -1) return;

		if (timers[ind].days.size() != 0) {
			auto curDay = getDatetime().tm_wday;
			bool isToday = false;
			for (auto& tDay : timers[ind].days) {
				if (tDay == DaysOfWeek[curDay]) isToday = true;
			}
			if (!isToday) return;
		}
		
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

	// TODO: getFiltered(false, {"(exclude or include)", {"Monday"})
	const std::vector<Timer> getFiltered(bool active, std::vector<std::string> days={} ) const {
		if (active && days.size() == 0) return timers;

		std::vector<Timer> filteredTimers{};
		std::copy_if(timers.begin(), timers.end(), std::back_inserter(filteredTimers), [&](Timer t) {
			if (!active) {
				if (getActiveTimer().has_value() && getActiveTimer().value().name == t.name) return false;
			}
			if (days.size() != 0) {
				if (t.days.size() == 0) return true;
				for (auto tDay : t.days) {
					for (auto fDay : days) {
						if (fDay == "Today") {
							auto curDay = getDatetime().tm_wday;
							if (tDay == DaysOfWeek[curDay]) return true;
						}
						if (tDay == fDay) return true;
					}
				}
				return false;
			}
			return true;
		});
		return filteredTimers;
	}

	const std::optional<Timer> getActiveTimer() const {
		if (activeTimerInd == -1) {
			return std::nullopt;
		}
		return std::make_optional(timers[activeTimerInd]);
	}
	
	const std::vector<Timer> getUnavailableTimers() const {}
	
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
		tm datetime = getDatetime();
		int curDay = datetime.tm_yday;
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

private:
	tm getDatetime() const {
		time_t curtime = time(0);
		tm buf;
		localtime_s(&buf, &curtime);
		return buf;
	}
};

