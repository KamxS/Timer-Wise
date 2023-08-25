include(FetchContent)

FetchContent_Declare(
	glfw	
	GIT_REPOSITORY "https://github.com/glfw/glfw.git"
	GIT_TAG 3.3-stable
)
add_subdirectory(glfw)

FetchContent_Declare(
	glad
	GIT_REPOSITORY "https://github.com/Dav1dde/glad.git"
)
add_subdirectory(glad)

FetchContent_Declare(
	imgui	
	GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
)
add_subdirectory(imgui)

FetchContent_Declare(
	json 
	URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
add_subdirectory(nlohmann_json)

