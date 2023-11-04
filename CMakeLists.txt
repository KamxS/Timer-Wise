cmake_minimum_required(VERSION 3.2)
project(TEST VERSION 1.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Put this behind Debug flag or sth
set(SDL2_NO_MWINDOWS ON)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${OPENGL_INCLUDE_DIRS})
add_subdirectory(lib)

add_executable(main main.cpp TimerWise.h stb_image.h)
target_link_libraries(main ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} imgui nlohmann_json::nlohmann_json)
set_property(TARGET main PROPERTY CXX_STANDARD 20)

# Should be automated + should be copied into assets not data directory
file(COPY assets/play.png DESTINATION ${CMAKE_BINARY_DIR}/data)
file(COPY assets/pause.png DESTINATION ${CMAKE_BINARY_DIR}/data)
file(COPY assets/config.png DESTINATION ${CMAKE_BINARY_DIR}/data)
file(COPY assets/remove.png DESTINATION ${CMAKE_BINARY_DIR}/data)
