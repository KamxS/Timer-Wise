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

struct TimerInput {
    int seconds;
    char name[20];
    int times[3];
    float color[3];
    bool isWeekly;
    std::vector<Break> breaks;

    std::vector<std::pair<std::string, bool>> weekDaysSel;

    TimerInput() : seconds(0), name(), times(), isWeekly(false), color(), breaks() {
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

constexpr char* breakTypes[2] = {"singular", "sequential"};
constexpr char* timeTypes[3] = {"seconds", "minutes", "hours"};
struct BreakInput {
    int breakTypeInd;
    int breakTiming[3];
    int breakDuration[3];
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

    // Load Textures
    std::vector<std::string> files{ "play.png", "pause.png", "cancel.png"};
    auto textures = loadTexturesFromDir(DataDir, files);

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

    TimerInput timerInput{};
    BreakInput breakInput{};

    bool show_demo_window = true;
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
                    ImGui::InputText("Name", timerInput.name, sizeof(timerInput.name));
                    ImGui::InputInt3("Time", timerInput.times);
                    ImGui::ColorEdit3("Timer Color", timerInput.color);
                    ImGui::Checkbox("Is Weekly", &timerInput.isWeekly);
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
                    if(ImGui::Button("Add Break")) {
                        ImGui::OpenPopup("Add Break");
                    }
                    if(ImGui::BeginPopupModal("Add Break")) {
                        const char* breakTypes[2] = {"singular", "sequential"};
                        const char* timeTypes[3] = {"seconds", "minutes", "hours"};
                        static_assert(sizeof(breakTypes)/sizeof(breakTypes[0]) == 2, "Only two types of breaks are supported");
                        ImGui::Combo("Break Type", &breakInput.breakTypeInd, breakTypes, 2);
                        if(breakInput.breakTypeInd== 0) {
                            ImGui::InputInt3("Break After", breakInput.breakTiming);
                        }else if(breakInput.breakTypeInd == 1) {
                            ImGui::InputInt3("Break Every", breakInput.breakTiming);
                        }
                        ImGui::InputInt3("Break Duration", breakInput.breakDuration);
                        if(ImGui::Button("Add")) {
                            bool valid = true;
                            BreakType breakType = (breakInput.breakTypeInd == 0) ? BreakType::SINGULAR : BreakType::SEQUENTIAL;
                            int breakDur = timeSumInSec(breakInput.breakDuration[2],breakInput.breakDuration[1], breakInput.breakDuration[0]); 
                            if (breakDur == 0) valid = false;
                            // TODO: User should not be able to create break that takes place after timer's time ending
                            int breakTiming = timeSumInSec(breakInput.breakTiming[2],breakInput.breakTiming[1], breakInput.breakTiming[0]); 
                            if(valid) timerInput.breaks.push_back(Break{breakType, breakDur, breakTiming});
                            breakInput = BreakInput{};
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Cancel")) {
                            breakInput = BreakInput{};
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::SameLine();
                    if(ImGui::Button("Show Breaks")) {
                        ImGui::OpenPopup("Breaks");
                    }
                    if(ImGui::BeginPopupModal("Breaks")) {
                        if(ImGui::BeginTable("##Breaks", 1)) {
                            for(auto& b: timerInput.breaks) {
                                ImGui::TableNextColumn();
                                std::string info = "Break ";
                                if(b.type == BreakType::SINGULAR) {
                                    info += "after ";
                                }else if(b.type == BreakType::SEQUENTIAL) {
                                    info += "every ";
                                }
                                // TODO: Display time not only in seconds
                                info += std::to_string(b.breakTimingSecs) + " seconds for ";
                                info += std::to_string(b.breakDurationSecs) + " seconds";
                                ImGui::Text(info.c_str());
                            }
                            ImGui::EndTable();
                        }
                        if (ImGui::Button("Close")) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::Button("Add")) {
                        bool valid = true;
                        // TODO: Proper error
                        if (sizeof(timerInput.name) == 0) valid = false;

                        std::chrono::hours hourSeconds{ timerInput.times[0] };
                        std::chrono::minutes minuteSeconds{ timerInput.times[1] };
                        std::chrono::seconds seconds{ timerInput.times[2] };
                        std::chrono::seconds total = hourSeconds + minuteSeconds + seconds;

                        // TODO: Proper error
                        if (total.count() == 0) valid = false;

                        std::string type = (timerInput.isWeekly) ? "weekly" : "daily";
                        std::cout << type << std::endl;

                        std::vector<std::string> days;
                        for (auto& day : timerInput.weekDaysSel) {
                            if (!day.second) continue;
                            days.push_back(day.first);
                        }

                        // TODO: Error when returns 1
                        if (valid) {
                            t.newTimer(timerInput.name, std::chrono::seconds{ total }, Color(timerInput.color[0], timerInput.color[1], timerInput.color[2]), days, type);
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
                
                ImGui::EndMenuBar();
            }
                    
            if (t.getActiveTimer().has_value()) {
                auto cur = t.getActiveTimer().value();
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
				for (auto& timer : t.getFiltered(false, {"Today"})) {
                    ImGui::TableNextColumn();
                    displayTimerCircle(timer, timerRadius, 15.f, ImVec2{columnOffset,0.f});
                    /*
                    std::string startBtnLabel = ">##";
                    startBtnLabel += ind;
                    if (ImGui::Button(startBtnLabel.c_str())) {
                        t.stopTimer();
                        t.startTimer(timer.name);
                    }
					*/
                    //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, 0.f);
                    // TODO: Fix weird offset
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnOffset/2));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0,0,0,0 });
                    ImGui::PushID(ind);
                    if (ImGui::ImageButton((GLuint*)textures["play.png"], ImVec2{ 10,11 })) {
                        t.stopTimer();
                        t.startTimer(timer.name);
                    }
                    ImGui::PopID();
                    ImGui::SameLine(0.f, 0.f);
                    ImGui::ImageButton((GLuint*)textures["pause.png"], ImVec2{11,11});
                    ImGui::SameLine(0.f, 0.f);
                    ImGui::ImageButton((GLuint*)textures["cancel.png"], ImVec2{11,11});
                    ImGui::PopStyleColor();
                    //ImGui::PopStyleVar();

                    /*
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
    t.saveDate(DaysFilePath);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

