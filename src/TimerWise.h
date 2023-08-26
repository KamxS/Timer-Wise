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
	Timer(std::string name, std::chrono::seconds dur, Color c, std::string typ, std::vector<std::string> days) : name(name), timePassed(std::chrono::milliseconds(0)), duration(dur), type(typ), days(days), timerColor(c) {
		lastChecked = std::chrono::steady_clock::now();
	}
	Timer(const nlohmann::json& j) {
		j.at("name").get_to(name);
		auto colorsJson = j.at("color");
		timerColor = Color{colorsJson[0],colorsJson[1],colorsJson[2]};
		duration = std::chrono::seconds(j.at("duration"));
		timePassed = std::chrono::seconds(j.at("timePassed"));
		j.at("days").get_to(days);
		j.at("type").get_to(type);
	}

	// TODO: Figure out how to change the order of attributes
	nlohmann::json toJson() const {
		return nlohmann::json{ 
			{"name", name}, 
			{"duration", duration.count()}, 
			{"timePassed", std::chrono::duration_cast<std::chrono::seconds>(timePassed).count()},
			{"type", type},
			{"color", nlohmann::json::array({timerColor.r, timerColor.g, timerColor.b})},
			{"days",days}
		};
	}
public:
	std::string name;
	Color timerColor;
	std::string type;
	float getDuration() const {
		return duration.count();
	}

	std::string getFormattedTimePassed() const {
		std::string out{};
		auto timePassedSecs = std::chrono::duration_cast<std::chrono::seconds>(timePassed);
		auto hours = std::chrono::duration_cast<std::chrono::hours>(duration - timePassedSecs);
		
		if (hours.count()> 0) {
			auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration - timePassedSecs - std::chrono::duration_cast<std::chrono::minutes>(hours));
			auto minutesStr = std::to_string(minutes.count());
			auto hoursStr = std::to_string(hours.count());
			if (minutesStr.length() < 2) {
				minutesStr = "0" + minutesStr;
			}
			if (hoursStr.length() < 2) {
				out = "0" + hoursStr + ":" + minutesStr;
			}
			else {
				out = hoursStr + ":" + minutesStr;
			}
		}
		else {
			auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration - timePassedSecs);
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration - timePassedSecs - std::chrono::duration_cast<std::chrono::seconds>(minutes));
			auto secondsStr = std::to_string(seconds.count());
			auto minutesStr = std::to_string(minutes.count());
			if (secondsStr.length() < 2) {
				out = minutesStr + ":" + "0" + secondsStr;
			}
			else {
				out = minutesStr + ":" + secondsStr;
			}
		}
		return out;
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
	int week;
	
	size_t get(const std::string name) const {
		for (auto ind = 0; ind < timers.size(); ind++) {
			if (timers[ind].name == name) return ind;
		}
		return -1;
	}

public: 
	Timers(): activeTimerInd(-1), timers(), day(0), week(0) {};

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

	int newTimer(std::string name, std::chrono::seconds duration, Color c, std::vector<std::string> days, std::string type) {
		if (get(name) != -1 ) return 1;
		timers.push_back(Timer(name, duration, c, type, days));
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
	
	// TODO: Change this
	void resetTimers(std::string type = "all") {
		for (auto& t : timers) {
			if (type == "daily" && t.type != "daily") continue;
			if (type == "weekly" && t.type != "weekly") continue;
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
		int curWeek = (datetime.tm_yday + 7 - (datetime.tm_wday ? (datetime.tm_wday - 1) : 6)) / 7;
		if (curWeek != week) {
			resetTimers();
		}
		else {
			resetTimers("daily");
		}
		day = curDay;
		week = curWeek;
	}
	
	void loadDate(const std::filesystem::path& path) {
		std::fstream f(path);
		if (!f.is_open()) return;
		std::stringstream s{};
		s << f.rdbuf();
		std::string buf = s.str();
		f.close();

		auto ind = buf.find(' ');
		day = stoi(buf);

		day = stoi(buf.substr(0,ind));
		if (ind != buf.length()) {
			week = stoi(buf.substr(ind, buf.length() - ind));
		}
		checkTime();
	}

	void saveDate(const std::filesystem::path& path) {
		// TODO: Better saving
		std::fstream f(path);
		if (!f.is_open()) return;
		f << std::to_string(day) + " " + std::to_string(week);
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

