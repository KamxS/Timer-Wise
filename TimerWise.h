#pragma once

#include <fstream>
#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct Color {
public:
  float r;
  float g;
  float b;
  Color() : r(0), g(0), b(180) {}
  Color(float r, float g, float b) : r(r), g(g), b(b) {}
  Color(float *arr) : r(arr[0]), g(arr[1]), b(arr[2]) {}

  // TODO: Make ints work
  //Color(int r, int g, int b) : r(r), g(g), b(b) {}
  //Color(float r, float g, float b) : r(r * 255), g(g * 255), b(b * 255) {}
  //Color(float *arr) : r(arr[0] * 255), g(arr[1] * 255), b(arr[2] * 255) {}
};

constexpr const char* DaysOfWeek[7] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                   "Thursday", "Friday", "Saturday"};

struct Timer {
  std::chrono::seconds duration;
  std::chrono::milliseconds timePassed;

  std::chrono::steady_clock::time_point lastChecked;
  
  Timer(const nlohmann::json &j) {
    j.at("name").get_to(name);
    auto colorsJson = j.at("color");
    timerColor = Color{colorsJson[0], colorsJson[1], colorsJson[2]};
    duration = std::chrono::seconds(j.at("duration"));
    timePassed = std::chrono::seconds(j.at("timePassed"));
    j.at("days").get_to(days);
    j.at("type").get_to(type);
  }

  // TODO: Figure out how to change the order of attributes
  nlohmann::json to_json() const {
    return nlohmann::json{
        {"name", name},
        {"duration", duration.count()},
        {"timePassed",
         std::chrono::duration_cast<std::chrono::seconds>(timePassed).count()},
        {"type", type},
        {"color",
         nlohmann::json::array({timerColor.r, timerColor.g, timerColor.b})},
        {"days", days}};
  }

    std::string name;
    Color timerColor;
    std::string type;
    std::vector<std::string> days;

    Timer(std::string name, std::chrono::seconds dur, Color c,std::vector<std::string> days, std::string typ) 
        : name(name), timePassed(std::chrono::milliseconds(0)), duration(dur), type(typ), days(days), timerColor(c) {
        lastChecked = std::chrono::steady_clock::now();
    }

    float getDuration() const { return duration.count(); }

    void getDurationArr(int* arr) const {
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
        auto minutes = (std::chrono::duration_cast<std::chrono::minutes>(duration) - hours);
        arr[0] = hours.count();
        arr[1] = minutes.count();
        arr[2] = (duration - hours - minutes).count();
    }

  std::string getFormattedTimePassed() const {
    std::string out{};
    auto timePassedSecs =
        std::chrono::duration_cast<std::chrono::seconds>(timePassed);
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration - timePassedSecs);

    if (hours.count() > 0) {
      auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration - timePassedSecs - std::chrono::duration_cast<std::chrono::minutes>(hours));
      auto minutesStr = std::to_string(minutes.count());
      auto hoursStr = std::to_string(hours.count());
      if (minutesStr.length() < 2) {
        minutesStr = "0" + minutesStr;
      }
      if (hoursStr.length() < 2) {
        out = "0" + hoursStr + ":" + minutesStr;
      } else {
        out = hoursStr + ":" + minutesStr;
      }
    } else {
      auto minutes = std::chrono::duration_cast<std::chrono::minutes>(
          duration - timePassedSecs);
      auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
          duration - timePassedSecs -
          std::chrono::duration_cast<std::chrono::seconds>(minutes));
      auto secondsStr = std::to_string(seconds.count());
      auto minutesStr = std::to_string(minutes.count());
      if (secondsStr.length() < 2) {
        out = minutesStr + ":" + "0" + secondsStr;
      } else {
        out = minutesStr + ":" + secondsStr;
      }
    }
    return out;
  }

  float getTimePassed() const {
    return std::chrono::duration_cast<std::chrono::seconds>(timePassed).count();
  }

    static inline int cur_day{0};
    static inline int cur_week{0};

    static tm get_datetime() {
        time_t curtime = time(0);
        tm datetime;
        localtime_s(&datetime, &curtime);
        return datetime;
    }

    static void save_date(const std::filesystem::path &path) {
        std::fstream f(path);
        if (!f.is_open()) return;
        f << std::to_string(Timer::cur_day) + " " + std::to_string(Timer::cur_week);
        f.close();
    }

    static bool update_day() {
        tm datetime = get_datetime();
        int new_day = datetime.tm_yday;
        bool did_day_change = new_day != cur_day;
        cur_day = new_day;
        return did_day_change;
    }

    static bool update_week() {
        tm datetime = get_datetime();
        int new_week = (datetime.tm_yday + 7 -(datetime.tm_wday ? (datetime.tm_wday - 1) : 6)) /7;
        bool did_week_change = new_week != cur_week;
        cur_week = new_week;
        return did_week_change;
    }
};

size_t get_timer_from_name(std::vector<Timer>& timers, const std::string name) {
    for (auto ind = 0; ind < timers.size(); ind++) {
      if (timers[ind].name == name)
        return ind;
    }
    return -1;
}

void reset_timer_vec(std::vector<Timer>& timers) {
    bool reset_daily = Timer::update_day();
    bool reset_weekly = Timer::update_week();
    for (auto &t : timers) {
        if (!reset_daily && t.type == "daily") continue;
        if (!reset_weekly && t.type == "weekly") continue;
        t.timePassed = std::chrono::milliseconds(0);
    }
}

void save_timer_vec(std::vector<Timer>& timers, const std::filesystem::path &path) {
    std::ofstream f(path, std::ofstream::trunc);
    if (!f.is_open()) return;

    std::vector<nlohmann::json> jsonVec{};
    for (auto& timer : timers) {
      jsonVec.push_back(timer.to_json());
    }
    nlohmann::json json(jsonVec);
    f << json.dump(4);
    f.close();
}
