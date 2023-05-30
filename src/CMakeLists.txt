cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (Chronos "Chronos.cpp" "Chronos.h")
target_link_libraries(Chronos glfw imgui glad nlohmann_json)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Chronos PROPERTY CXX_STANDARD 20)
endif()