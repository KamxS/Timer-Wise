#pragma once

#include <string>
#include <chrono>
#include <map>
#include <vector>


class Serializable {

};

class Deserializable {

};

class Timer {
	friend class Timers;
	std::chrono::seconds duration;
	std::chrono::milliseconds current;

	std::chrono::steady_clock::time_point lastChecked;
	Timer(std::chrono::seconds dur): current(std::chrono::milliseconds(0)),duration(dur) {
		lastChecked = std::chrono::steady_clock::now();
	}
public:
	float getDuration() const {
		return duration.count();
	}

	float getCurrent() const {
		return std::chrono::duration_cast<std::chrono::seconds>(current).count();
	}
};

class Timers {
	std::map<std::string, Timer*> activeTimers;
	std::map<std::string, Timer*> inactiveTimers;
	std::vector<std::string> removeQueue;
	std::mutex timersMut;
	std::jthread timerThread;
public: 
	Timers() : inactiveTimers{}, activeTimers{}, timersMut{}, timerThread{}, removeQueue{}  {}

	void Update() {
		for (auto& timerPair : activeTimers) {
			auto name = timerPair.first;
			auto timer = timerPair.second;
			timer->current += std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now() - timer->lastChecked));
			if (timer->current >= timer->duration) {
				removeQueue.push_back(timerPair.first);
				continue;
			}
			timer->lastChecked = std::chrono::steady_clock::now();
		}
		for (auto timer : removeQueue) {
			stopTimer(timer);
		}
		removeQueue.clear();
	}

	void newTimer(std::string name, std::chrono::seconds duration) {
		auto timer = new Timer(duration);
		inactiveTimers.insert(std::make_pair(name, timer));
	}

	void startTimer(std::string name) {
		if (!inactiveTimers.contains(name)) throw std::domain_error("Couldn't find specified Timer");
		auto timer = inactiveTimers[name];
		timer->lastChecked = std::chrono::steady_clock::now();
		activeTimers.insert(std::make_pair(name,timer));
		inactiveTimers.erase(name);
	}

	void stopTimer(std::string name) {
		if (!activeTimers.contains(name)) throw std::domain_error("Couldn't find specified Timer");
		auto timer = activeTimers[name];
		activeTimers.erase(name);
		inactiveTimers.insert(std::make_pair(name, timer));
	}

	std::map<std::string, const Timer*> getTimers() {
		std::map<std::string,const Timer*> out{};

		for (auto& timer : activeTimers) {
			out.insert(std::make_pair(timer.first, timer.second));
		}
		
		for (auto& timer : inactiveTimers) {
			out.insert(std::make_pair(timer.first, timer.second));
		}
		
		return out;
	}
};
