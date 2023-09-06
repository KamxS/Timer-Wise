#include <unordered_set>
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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// TODO: Use as Nvim extension?
// TODO: Those should be configurable
const std::filesystem::path DataDir = std::filesystem::current_path() / "data";
const std::filesystem::path TimersFilePath = DataDir / "timers.json";
const std::filesystem::path DaysFilePath = DataDir / "days.txt";
constexpr char* timerTypes[2] = {"daily", "weekly"};
constexpr char* timeUnits[3] = {"seconds", "minutes", "hours"};

struct TimerInput {
    char name[20];
    int times[3];
    float color[3];
    int timerTypeInd;
    std::unordered_map<std::string, bool>weekDaysSel;

    TimerInput() :  name(), times(), timerTypeInd(0), color() {
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
    TimerInput(const Timer& timer) :  name(), times(), color() {
        strcpy_s(name, timer.name.c_str());
        timerTypeInd = (timer.type == "daily") ? 0 : 1;
        timer.getDurationArr(times);
        color[0] = timer.timerColor.r;
        color[1] = timer.timerColor.g;
        color[2] = timer.timerColor.b;
        weekDaysSel = {
            {"Monday",false},
            {"Tuesday",false},
            {"Wednesday",false},
            {"Thursday",false},
            {"Friday",false},
            {"Saturday",false},
            {"Sunday",false}
        };
        for(auto& day: timer.days) {
            weekDaysSel[day] = true;
        }
    }
};

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
        // TODO: It is opening file twice which is not efficient
        std::ofstream output(DaysFilePath);
        output << "";
        output.close();
        timers.checkTime();
        timers.saveDate(DaysFilePath);
    }else {
        timers.loadDate(DaysFilePath);
    }
}

constexpr int timeSumInSec(int seconds, int minutes = 0, int hours = 0) {
    std::chrono::seconds secs(seconds);
    std::chrono::minutes mins(minutes);
    std::chrono::hours hs(hours);
    return (hs + mins + secs).count();
}

std::unordered_map<std::string, unsigned int> loadTexturesFromDir(std::filesystem::path dir, std::vector<std::string>& names) {
    std::unordered_map<std::string, unsigned int> textures{};
    for (auto name : names) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int width, height, colorChannels;
        unsigned char* data = stbi_load((dir / name).string().c_str(), &width, &height, &colorChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            textures[name] = textureID;
        }
        else {
            std::cerr << "Failed to load file: " << name << std::endl;
        }
        stbi_image_free(data);
    }
    return textures;
}

// Circle code taken from: https://github.com/ocornut/imgui/issues/2020
auto ProgressCircle(float progress, float radius, float thickness, std::string time, const ImColor& color, const ImColor& textColor={255,255,255}) {
    ImVec2 offset{ 0,20 };
    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    auto&& pos = ImGui::GetCursorPos();
    pos += offset;
    ImVec2 size{ radius * 2, radius * 2 };
    
    const float a_min = 0;
    const float a_max = IM_PI * 2.0f * progress;
    const auto&& center = ImVec2(pos.x + radius, pos.y + radius);

    window->DrawList->PathClear();

    // Circle's Shadow
    for (auto i = 0; i <= 100; i++) {
        const float a = a_min + ((float)i / 100.f) * (IM_PI * 2.f - a_min);
        window->DrawList->PathLineTo({ center.x + ImCos(a - (IM_PI/2)) * radius, center.y + ImSin(a - (IM_PI/2)) * radius });
    }
    window->DrawList->PathStroke(ImGui::GetColorU32(ImVec4(0.2,0.2,0.2,0.5)), false, thickness);

    // Counter
    auto timeSize = ImGui::CalcTextSize(time.c_str())/2;
    window->DrawList->AddText({ center.x-timeSize.x, center.y-timeSize.y }, textColor, time.c_str());

    // Hitbox
    const ImRect bb{ pos , pos + size + offset * 2};
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, 0)) return;
   
    // Circle
    for (auto i = 0; i <= 100; i++) {
        const float a = a_min + ((float)i / 100.f) * (a_max - a_min);
        window->DrawList->PathLineTo({ center.x + ImCos(a - (IM_PI/2)) * radius, center.y + ImSin(a - (IM_PI/2)) * radius });
    }
    window->DrawList->PathStroke(color, false, thickness);
}

void displayTimerCircle(const Timer& timer, float radius, float thickness, ImVec2 offset = {0,0}) {
    float progress = timer.getTimePassed() / timer.getDuration();
    ImGui::SetCursorPos(ImGui::GetCursorPos() + offset - ImVec2(radius,0));
    auto counterCol = (timer.type == "weekly") ? ImColor{255, 216, 0} : ImColor{255, 255, 255};
    ProgressCircle(progress, radius, thickness, timer.getFormattedTimePassed(), ImColor(timer.timerColor.r, timer.timerColor.g, timer.timerColor.b), counterCol);

    std::string text = timer.name;
    if (text.length() > 17) {
        text = text.substr(0, 17) + "...";
    }
    auto textOff = ImGui::CalcTextSize(text.c_str())*0.5f;
    ImGui::SetCursorPos(ImGui::GetCursorPos() + offset - textOff);
    ImGui::Text(text.c_str());
}

int main()
{
    std::cout << std::filesystem::current_path().string() << std::endl;
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

    // Load Textures
    std::vector<std::string> files{ "play.png", "pause.png", "cancel.png", "config.png"};
    auto textures = loadTexturesFromDir(DataDir, files);

    Timers timers{};
    loadFiles(timers);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    TimerInput timerInput{};
    ImGuiID timerConfigPopupID = ImHashStr( "Timer Config" );
    std::string editedTimer = "";

    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        timers.update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

        {
            ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
            ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
            
            // Remove this flag after getting rid of show_demo_window option
            int ifNoTitleBarFlag = (show_demo_window) ? 0 : ImGuiWindowFlags_NoTitleBar;
            ImGui::Begin("TimerWise", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar | ifNoTitleBarFlag);


            if (ImGui::BeginMenuBar()) {
                if (ImGui::MenuItem("Timer")) {
                    ImGui::PushOverrideID(timerConfigPopupID);
                    ImGui::OpenPopup("Timer Config");
                    ImGui::PopID();
                }
                ImGui::EndMenuBar();
            }
                    
            if (timers.getActiveTimer().has_value()) {
                auto cur = timers.getActiveTimer().value();
                const float radius = 40.f;

                float avail = ImGui::GetContentRegionAvail().x;
                float off = (avail - radius) * 0.5f;
                displayTimerCircle(cur, radius, 20.f, ImVec2(ImGui::GetCursorPosX() + off,0));
            }
            
            ImGui::SeparatorText("Timers for Today");
            if(ImGui::BeginTable("TodayTimers", 6)) {
                int ind = 0;
                float columnOffset = 50.f;
                float timerRadius = 30.f;
				for (auto& timer : timers.getFiltered(false, {"Today"})) {
                    ImGui::TableNextColumn();
                    displayTimerCircle(timer, timerRadius, 15.f, ImVec2{columnOffset,0.f});
                                        // TODO: Fix weird offset
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnOffset/2));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0,0,0,0 });
                    ImGui::PushID(ind);
                    if (ImGui::ImageButton((GLuint*)textures["play.png"], ImVec2{ 10,11 })) {
                        timers.stopTimer();
                        timers.startTimer(timer.name);
                    }
                    ImGui::SameLine(0.f, 0.f);
                    if(ImGui::ImageButton((GLuint*)textures["config.png"], ImVec2{11,11})) {
                        timerInput = TimerInput(timer);
                        editedTimer = timer.name;
                        ImGui::PushOverrideID(timerConfigPopupID);
                        ImGui::OpenPopup("Timer Config");
                        ImGui::PopID();
                    }
                    ImGui::SameLine(0.f, 0.f);
                    if(ImGui::ImageButton((GLuint*)textures["cancel.png"], ImVec2{11,11})) {
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopID();

                    /*
                    std::string startBtnLabel = ">##";
                    startBtnLabel += ind;
                    if (ImGui::Button(startBtnLabel.c_str())) {
                        t.stopTimer();
                        t.startTimer(timer.name);
                    }
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, 0.f);
                    ImGui::PopStyleVar();
					std::string configBtnLabel = "o##";
                    configBtnLabel += ind;
                    if (ImGui::Button(configBtnLabel.c_str())) {}
                    ImGui::SameLine();

					std::string deleteBtnLabel = "X##";
                    deleteBtnLabel += ind;
                    if (ImGui::Button(deleteBtnLabel.c_str())) {}
					*/

                    ind += 1;
                }
                ImGui::EndTable();
            }
            ImGui::PushOverrideID(timerConfigPopupID);
            if(ImGui::BeginPopupModal("Timer Config")) {
                ImGui::InputText("Name", timerInput.name, sizeof(timerInput.name));
                ImGui::InputInt3("Time", timerInput.times);
                ImGui::ColorEdit3("Timer Color", timerInput.color);
                static_assert(sizeof(timerTypes)/sizeof(timerTypes[0]) == 2, "Only two types of timers are supported");
                ImGui::Combo("Timer Type", &timerInput.timerTypeInd, timerTypes, 2);
                ImGui::Text("Timer available at: ");
                if (ImGui::BeginTable("Days", 7, ImGuiTableFlags_Borders, ImVec2(ImGui::GetWindowWidth()*0.4, 1.5))) {
                    for (auto& day : timerInput.weekDaysSel) {
                        ImGui::TableNextColumn();
                        if(ImGui::Selectable(day.first.substr(0,3).c_str(), day.second, ImGuiSelectableFlags_DontClosePopups)) day.second = !day.second;
                    }
                    ImGui::EndTable();
                }

                // TODO: Breaks
                // Break every/after x h/min/secs for x h/min/secs
                if (ImGui::Button("Save")) {
                    bool valid = true;
                    // TODO: Proper error
                    if (sizeof(timerInput.name) == 0) valid = false;

                    int total = timeSumInSec(timerInput.times[2], timerInput.times[1], timerInput.times[0]);
                    // TODO: Proper error
                    if (total == 0) valid = false;

                    std::vector<std::string> days;
                    for (auto& day : timerInput.weekDaysSel) {
                        if (!day.second) continue;
                        days.push_back(day.first);
                    }

                    // TODO: Error when returns 1
                    if (valid) {
                            timers.newTimer(timerInput.name, std::chrono::seconds{ total }, 
                                Color(timerInput.color[0], timerInput.color[1], timerInput.color[2]), 
                                days, timerTypes[timerInput.timerTypeInd]
                            );
                    }
                    timerInput = TimerInput{};
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    timerInput = TimerInput{};
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
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

    timers.saveTimers(TimersFilePath);
    timers.saveDate(DaysFilePath);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

