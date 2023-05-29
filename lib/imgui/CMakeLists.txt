message(STATUS "Fetching Imgui")

FetchContent_MakeAvailable(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    option(IMGUI_EXAMPLES OFF CACHE INTERNAL "Build ImGui examples")
    option(IMGUI_DEMO OFF CACHE INTERNAL "Include the ImGui demo window implementation in library")

    add_subdirectory(${imgui_SOURCE_DIR} ${imgui_BINARY_DIR})
endif()

add_library(imgui
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui.h
  ${imgui_SOURCE_DIR}/imconfig.h
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_internal.h
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PUBLIC glfw)
