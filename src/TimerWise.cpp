#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <filesystem>
#include "TimerWise.h"


// TODO: Those should be configurable
const std::filesystem::path DataDir = std::filesystem::current_path() / "../data";
const std::filesystem::path TimersFilePath = std::filesystem::current_path() / DataDir / "timers.json";
const std::filesystem::path DaysFilePath = std::filesystem::current_path() / DataDir / "days.txt";

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}


void loadFiles(Timers& timers) {
    if (std::filesystem::is_directory(DataDir) == 0) {
        std::filesystem::create_directory(DataDir);
    }
   
    if (!std::filesystem::exists(TimersFilePath)) {
        std::ofstream output(TimersFilePath);
        output << "";
        output.close();
    }
    else {
        timers.loadTimers(TimersFilePath);
    }

    if(!std::filesystem::exists(DaysFilePath)) {
        std::ofstream output(DaysFilePath);
        output << "";
        output.close();
    }else {
        timers.loadDays(DaysFilePath);
    }
}

// Circle code taken from: https://github.com/ocornut/imgui/issues/2020
auto ProgressCircle(float progress, float radius, float thickness, const ImVec4& color) {
    ImVec2 offset{ 20,20 };
    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    auto&& pos = ImGui::GetCursorPos();
    pos += offset;
    ImVec2 size{ radius * 2, radius * 2 };
    
    const float a_min = 0;
    const float a_max = IM_PI * 2.0f * progress;
    const auto&& centre = ImVec2(pos.x + radius, pos.y + radius);

    // Timer
    /*
    ImGui::SetCursorPosX(pos.x + radius);
    ImGui::SetCursorPosY(pos.y + radius);
    ImGui::Text("00:30");
    */

    window->DrawList->PathClear();

    // Circle's Shadow
     for (auto i = 0; i <= 100; i++) {
        const float a = a_min + ((float)i / 100.f) * (IM_PI * 2.f - a_min);
        window->DrawList->PathLineTo({ centre.x + ImCos(a - (IM_PI/2)) * radius, centre.y + ImSin(a - (IM_PI/2)) * radius });
    }
    window->DrawList->PathStroke(ImGui::GetColorU32(ImVec4(0.2,0.2,0.2,0.5)), false, thickness);

    // Hitbox
    const ImRect bb{ pos , pos + size + offset * 2};
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, 0)) return;
   
    // Circle
    for (auto i = 0; i <= 100; i++) {
        const float a = a_min + ((float)i / 100.f) * (a_max - a_min);
        window->DrawList->PathLineTo({ centre.x + ImCos(a - (IM_PI/2)) * radius, centre.y + ImSin(a - (IM_PI/2)) * radius });
    }
    window->DrawList->PathStroke(ImGui::GetColorU32(color), false, thickness);
}

void displayTimerCircle(const Timer timer, float radius, float thickness) {
    float progress = timer.getTimePassed() / timer.getDuration();
    ProgressCircle(progress, radius, thickness, ImVec4(timer.timerColor.r,timer.timerColor.g,timer.timerColor.b,1.f));
}

struct NewTimerOptions {
    int seconds;
    char name[50];
    int times[3];
    float color[3];
    std::vector<std::pair<std::string, bool>> weekDaysSel;

    NewTimerOptions() : seconds(0), name(), times(), color() {
         weekDaysSel = {
            {"Monday",true},
            {"Tuesday",true},
            {"Wednesday",true},
            {"Thursday",true},
            {"Friday",true},
            {"Saturday",true},
            {"Sunday",true}
        };
    }
};

int main()
{
	glfwInit();
    glfwSetErrorCallback(glfw_error_callback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "TimerWise", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create Glfw window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glViewport(0, 0, 800, 600); 
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    Timers t{};
    loadFiles(t);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    NewTimerOptions options{};

    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        t.Update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

        {
            ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
            ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
            
            // Remove this flag after getting rid of show_demo_window option
            int ifNoTitleBarFlag = (show_demo_window) ? 0 : ImGuiWindowFlags_NoTitleBar;
            ImGui::Begin("TimeWise", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar | ifNoTitleBarFlag);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::MenuItem("Timer")) {
                    ImGui::OpenPopup("Add Timer");
                }

                // TODO: Better UI for Timer Adding
    			if(ImGui::BeginPopupModal("Add Timer")) {
                    ImGui::InputText("Name", options.name, sizeof(options.name));
                    ImGui::InputInt3("Time", options.times);
                    ImGui::ColorEdit3("Timer Color", options.color);
                   
					if (ImGui::BeginTable("Days", 7, ImGuiTableFlags_Borders, ImVec2(ImGui::GetWindowWidth()*0.4, 1.5))) {
                        for (auto& day : options.weekDaysSel) {
                            ImGui::TableNextColumn();
							if(ImGui::Selectable(day.first.substr(0,3).c_str(), day.second, ImGuiSelectableFlags_DontClosePopups)) day.second = !day.second;
                        }
                    }
                    ImGui::EndTable();

                    if (ImGui::Button("Add")) {
                        if (sizeof(options.name) == 0) {
                            // TODO: Proper error
                            ImGui::CloseCurrentPopup();
                        }
                        std::chrono::hours hourSeconds{ options.times[0] };
                        std::chrono::minutes minuteSeconds{ options.times[1] };
                        std::chrono::seconds seconds{ options.times[2] };
                        std::chrono::seconds total = hourSeconds + minuteSeconds + seconds;

                        std::vector<std::string> days;
                        
                        for (auto& day : options.weekDaysSel) {
                            if (!day.second) continue;
                            days.push_back(day.first);
                        }
                       
                        // TODO: Error when returns 1
                        t.newTimer(options.name, std::chrono::seconds{ total }, Color(options.color[0], options.color[1], options.color[2]), days);
                        options = NewTimerOptions{};
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

                    ImGui::EndPopup();
                }

                ImGui::EndMenuBar();
            }

            // TODO: Repair centering
            if (t.getActiveTimer().has_value()) {
                auto cur = t.getActiveTimer().value();
                const float radius = 30.f;

                float avail = ImGui::GetContentRegionAvail().x;
                float off = (avail - radius) * 0.5f;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
                displayTimerCircle(cur, radius, 15.f);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
                ImGui::Text(cur.name.c_str());
            }
            
            ImGui::SeparatorText("Today's Timers");
            if(ImGui::BeginTable("TodayTimers", 6)) {
                int ind = 0;
				for (auto& timer : t.getFiltered(false, {"Today"})) {
                    ImGui::TableNextColumn();
                    displayTimerCircle(timer, 20.f, 10.f);

                    ImGui::Text(timer.name.c_str());

                    std::string startBtnLabel = ">##";
                    startBtnLabel += ind;
                    if (ImGui::Button(startBtnLabel.c_str())) {
                        t.stopTimer();
                        t.startTimer(timer.name);
                    }
                    ImGui::SameLine();

					std::string configBtnLabel = "o##";
                    configBtnLabel += ind;
                    if (ImGui::Button(configBtnLabel.c_str())) {}
                    ImGui::SameLine();

					std::string deleteBtnLabel = "X##";
                    deleteBtnLabel += ind;
                    if (ImGui::Button(deleteBtnLabel.c_str())) {}

                    ind += 1;
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

    t.saveTimers(TimersFilePath);
    t.saveDays(DaysFilePath);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

