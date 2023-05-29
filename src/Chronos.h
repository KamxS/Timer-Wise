#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <map>


class Serializable {

};

class Deserializable {

};

class Timer {
	friend class Timers;
	std::chrono::milliseconds duration;

	std::chrono::milliseconds current;
	std::chrono::steady_clock::time_point lastChecked;
	Timer(std::chrono::milliseconds dur): current(dur),duration(dur) {
		lastChecked = std::chrono::steady_clock::now();
	}
};

class Timers {
	std::map<std::string, Timer*> inactiveTimers;
	std::map<std::string, Timer*> activeTimers;
	std::vector<std::string> removeQueue;
	std::mutex timersMut;
	std::jthread timerThread;
public: 
	Timers() : inactiveTimers{}, activeTimers{}, timersMut{}, timerThread{}, removeQueue{}  {}

	void startThread() {
		timerThread = std::jthread {
			[&]() {
				while (true) {
					for (auto timer : removeQueue) {
						stopTimer(timer);
					}
					removeQueue.clear();

					std::this_thread::sleep_for(std::chrono::milliseconds{100});

					std::scoped_lock<std::mutex> lock(timersMut);
					for (auto& timerPair : activeTimers) {
						auto timer = timerPair.second;
						timer->current -= std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now() - timer->lastChecked));
						if (timer->current < std::chrono::milliseconds(0)) {
							removeQueue.push_back(timerPair.first);
							continue;
						}
						timer->lastChecked = std::chrono::steady_clock::now();
					}
				}
			}
		};
	}

	void newTimer(std::string name, std::chrono::milliseconds duration) {
		auto timer = new Timer(duration);
		inactiveTimers.insert(std::make_pair(name, timer));
	}

	void startTimer(std::string name) {
		if (!inactiveTimers.contains(name)) throw std::domain_error("Couldn't find specified Timer");
		auto timer = inactiveTimers[name];
		timer->lastChecked = std::chrono::steady_clock::now();
		std::scoped_lock<std::mutex> lock(timersMut);
		activeTimers.insert(std::make_pair(name,timer));
		inactiveTimers.erase(name);
	}

	void stopTimer(std::string name) {
		if (!activeTimers.contains(name)) throw std::domain_error("Couldn't find specified Timer");
		std::scoped_lock<std::mutex> lock(timersMut);
		auto timer = activeTimers[name];
		activeTimers.erase(name);
		inactiveTimers.insert(std::make_pair(name, timer));
	}
};
