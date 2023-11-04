#include <SDL_render.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#define NO_STDIO_REDIRECT
#include <SDL.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include <iostream>
#include <utility>
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
    TimerInput(Timer&& timer) :  name(), times(), color() {
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

unsigned int load_texture(std::filesystem::path file_path) {
    int width, height, colorChannels;
    unsigned char* data = stbi_load(file_path.string().c_str(), &width, &height, &colorChannels, 0);
    if (!data) {
        std::cerr << "Failed to load file: " << file_path.filename()<< std::endl;
        return -1;
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return textureID;
}

void loadFiles(std::vector<Timer>& timers) {
    if (std::filesystem::is_directory(DataDir) == 0) {
        std::filesystem::create_directory(DataDir);
    }
   
    if (!std::filesystem::exists(TimersFilePath)) {
        std::ofstream output(TimersFilePath);
        output << "";
        output.close();
    }
    else {
        std::fstream f(TimersFilePath);
        if (!f.is_open()) return;

        nlohmann::json json = nlohmann::json::parse(f);
        for (auto &j : json) {
            timers.push_back(Timer(j));
        }
        f.close(); 
    }

    if(!std::filesystem::exists(DaysFilePath)) {
        // TODO: It is opening file twice which is not so good i guess
        std::ofstream output(DaysFilePath);
        output << "";
        output.close();
        reset_timer_vec(timers);
    }else {
        std::fstream f(DaysFilePath);
        if (!f.is_open()) return;
        std::stringstream s{};
        s << f.rdbuf();
        std::string buf = s.str();
        f.close();

        auto ind = buf.find(' ');
        Timer::cur_day = stoi(buf.substr(0, ind));
        if (ind != buf.length()) {
            Timer::cur_week = stoi(buf.substr(ind, buf.length() - ind));
        }
        reset_timer_vec(timers);
    }
}

constexpr int timeSumInSec(int seconds, int minutes = 0, int hours = 0) {
    return seconds+minutes*60+hours*3600;
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

void displayTimerCircle(Timer& timer, float radius, float thickness, ImVec2 offset = {0,0}) {
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


int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("TimerWise", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    std::unordered_map<std::string, unsigned int>textures{};
    std::vector<Timer> timers{};
    size_t active_timer = -1;
    loadFiles(timers);

    unsigned int play_texture = load_texture(DataDir / "play.png");
    unsigned int config_texture = load_texture(DataDir / "config.png");
    unsigned int remove_texture = load_texture(DataDir / "remove.png");
    unsigned int pause_texture = load_texture(DataDir / "pause.png");

    TimerInput timerInput{};
    ImGuiID timerConfigPopupID = ImHashStr( "Timer Config" );
    Timer* edited_timer = nullptr;

    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        if (active_timer == -1) {
            reset_timer_vec(timers); 
        }else {
            timers[active_timer].timePassed +=
                std::chrono::duration_cast<std::chrono::milliseconds>(
                (std::chrono::steady_clock::now() - timers[active_timer].lastChecked));

            timers[active_timer].lastChecked = std::chrono::steady_clock::now();
            if (timers[active_timer].timePassed >= timers[active_timer].duration) {
                active_timer = -1;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
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

            if (active_timer != -1) {
                const float radius = 40.f;

                float avail = ImGui::GetContentRegionAvail().x;
                float off = (avail - radius) * 0.5f;
                displayTimerCircle(timers[active_timer], radius, 20.f, ImVec2(ImGui::GetCursorPosX() + off,0));
            }

            ImGui::SeparatorText("Timers for Today");
            if(ImGui::BeginTable("TodayTimers", 6)) {
                int ind = 0;
                float columnOffset = 50.f;
                float timerRadius = 30.f;
                // TODO: Change this
                static const char* days[] = {
                    "Sunday", "Monday", "Tuesday", "Wednesday", 
                    "Thursday", "Friday", "Saturday"
                };
                static auto f = [&](Timer& timer) {
                    bool valid = false;
                    if (timer.days.size() != 0) {
                        for (auto timer_day : timer.days) {
                            if(timer_day == days[Timer::get_datetime().tm_wday]) valid=true;
                        }
                    }
                    if(valid) {
                        if(active_timer==-1) return true;
                        return !(timer.name == timers[active_timer].name);
                    }else {
                        return false;
                    }
                };
                for (auto& timer : timers | std::views::filter(f)) {
                    ImGui::TableNextColumn();
                    displayTimerCircle(timer, timerRadius, 15.f, ImVec2{columnOffset,0.f});
                    // TODO: Fix weird offset
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnOffset/2));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0,0,0,0 });
                    ImGui::PushID(ind);

                    if (ImGui::ImageButton((void*)play_texture, ImVec2{ 10, 11})) {
                        timer.lastChecked = std::chrono::steady_clock::now();
                        active_timer = get_timer_from_name(timers, timer.name);
                    }
                    ImGui::SameLine(0.f, 0.f);
                    if(ImGui::ImageButton((void*)config_texture, ImVec2{11,11})) {
                        // TODO: Pass by pointer
                        timerInput = TimerInput(std::move(timer));
                        edited_timer = &timer;
                        ImGui::PushOverrideID(timerConfigPopupID);
                        ImGui::OpenPopup("Timer Config");
                        ImGui::PopID();
                    }
                    ImGui::SameLine(0.f, 0.f);
                    // TODO: Add "are you sure?" popup
                    if(ImGui::ImageButton((void*)remove_texture, ImVec2{11,11})) {
                        /*
                        //timers.erase(std::remove_if(timers.begin(), timers.end(), [timer](Timer& t) {return (t.name!=timer.name);}),timers.end());
                        auto filtered= std::remove_if(timers.begin(), timers.end(), [timer](Timer& t) {return (t.name!=timer.name);});
                        if(active_timer != -1) {
                            // TODO: Update active timer
                        }
                        timers.erase(filtered, timers.end());
                        */
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                    ind++;
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
                        if(edited_timer !=nullptr) { 
                            auto time_passed = edited_timer->timePassed;
                            *edited_timer = Timer(timerInput.name, std::chrono::seconds{ total }, 
                                Color(timerInput.color[0], timerInput.color[1], timerInput.color[2]), 
                                days, timerTypes[timerInput.timerTypeInd]);
                            edited_timer->timePassed = time_passed;
                        }else {
                            timers.push_back(Timer(timerInput.name, std::chrono::seconds{ total }, 
                                Color(timerInput.color[0], timerInput.color[1], timerInput.color[2]), 
                                days, timerTypes[timerInput.timerTypeInd]
                            ));
                        }
                    }
                    edited_timer = nullptr;
                    timerInput = TimerInput{};
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    edited_timer = nullptr;
                    timerInput = TimerInput{};
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    save_timer_vec(timers, TimersFilePath);
    Timer::save_date(DaysFilePath);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
