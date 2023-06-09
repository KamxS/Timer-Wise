#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <nlohmann/json.hpp>
#include <fstream>

#include <filesystem>

#include "Chronos.h"
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void loadFiles(Timers& timers) {
    auto path = std::filesystem::current_path() / "../data";
    if (std::filesystem::is_directory(path) == 0) {
        std::filesystem::create_directory(path);
    }

    auto filePath = path / "test.json";
    std::fstream in;
    in.open(filePath, std::fstream::in | std::fstream::out);
    if (!in.is_open()) {
        std::ofstream output(filePath);
        output << "{\"hello\":1}";
        output.close();
        return;
    }

    nlohmann::json timersJson = nlohmann::json::parse(in);
    std::cout << timersJson["hello"];
}

void displayTimer(const Timer timer, int width = -1.f) {
    float progress = timer.getCurrent() / timer.getDuration();
    ImGui::ProgressBar(progress, ImVec2(width, 0.f), timer.name.c_str());
}


int main()
{
	glfwInit();
    glfwSetErrorCallback(glfw_error_callback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create Glfw window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glViewport(0, 0, 800, 600); 
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	using namespace std::chrono_literals;
    Timers t{};
    loadFiles(t);
    
    t.newTimer("timer1", 10s);
	t.newTimer("timer2", 20s);
	t.newTimer("timer3", 60s);
	t.newTimer("timer4", 60s);
	t.newTimer("timer5", 60s);
	t.startTimer("timer1");
	//t.startTimer("timer2");
	
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    int seconds = 0;
    char name[50] = "";

    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        t.Update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        {
            ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
            ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
            ImGui::Begin("Timers", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::MenuItem("Timer")) {
                    ImGui::OpenPopup("Add Timer");
                }

    			if(ImGui::BeginPopupModal("Add Timer")) {
                    ImGui::InputText("Name", name, sizeof(name));
                    ImGui::InputInt("Seconds", &seconds);
                    if (ImGui::Button("Add")) {
                        t.newTimer(name, std::chrono::seconds{ seconds });
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                ImGui::EndMenuBar();
            }
            
            if (t.getActiveTimer().has_value()) {
                auto cur = t.getActiveTimer().value();
                ImGui::Text("Currently running: ");
                displayTimer(cur);
            }

            ImGui::Spacing();

            if(ImGui::BeginTable("Timers", 3, ImGuiTableFlags_SizingFixedFit)) {
                int row = 0;
                for (auto& timer : t.getTimers()) {
                    ImGui::TableNextRow();
				    if (t.getActiveTimer().has_value()) {
                        if (t.getActiveTimer().value().name == timer.name) {
                            continue;
                        }
                    }
                    ImGui::TableNextColumn();
                    displayTimer(timer, ImGui::GetWindowWidth() * 0.8);

                    ImGui::TableNextColumn();
                    std::string buttonLabel = "Start Timer##";
                    buttonLabel += row;
                    if (ImGui::Button(buttonLabel.c_str())) {
                        t.stopTimer();
                        t.startTimer(timer.name);
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("Config (WIP)");
                    row += 1;
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

