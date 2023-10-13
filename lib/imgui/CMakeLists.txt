add_library(imgui
  ./imgui.cpp
  ./imgui.h
  ./imconfig.h
  ./imgui_demo.cpp
  ./imgui_draw.cpp
  ./imgui_internal.h
  ./imgui_tables.cpp
  ./imgui_widgets.cpp
  ./backends/imgui_impl_sdl2.cpp
  ./backends/imgui_impl_sdl2.h
  ./backends/imgui_impl_opengl3.cpp
  ./backends/imgui_impl_opengl3.h
)
target_include_directories(imgui
    PUBLIC .
    PUBLIC ./backends
)
target_link_libraries(imgui ${SDL2_LIBRARIES})
