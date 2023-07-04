cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (TimerWise "TimerWise.cpp" "TimerWise.h")
target_link_libraries(TimerWise glfw imgui glad nlohmann_json)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TimerWise PROPERTY CXX_STANDARD 20)
endif()