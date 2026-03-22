#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <fstream>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <GL/glew.h>

struct Location {
    float latitude = 0.0f;
    float longitude = 0.0f;
    float altitude = 0.0f;
    std::string timestamp = "No data yet";
};

std::mutex loc_mutex;

void save_to_json(const Location& loc) {
    std::ifstream infile("locations.json");
    nlohmann::json arr = nlohmann::json::array();
    if (infile.good()) {
        infile >> arr;
    }
    nlohmann::json entry = {
        {"latitude", loc.latitude},
        {"longitude", loc.longitude},
        {"altitude", loc.altitude},
        {"timestamp", loc.timestamp}
    };
    arr.push_back(entry);
    std::ofstream outfile("locations.json");
    outfile << arr.dump(4);
}

void run_server(Location* loc) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://0.0.0.0:5555");
    std::cout << "Server started on port 5555" << std::endl;

    while (true) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);
        std::string msg(static_cast<char*>(request.data()), request.size());
        std::cout << "Received: " << msg << std::endl;

        try {
            auto j = nlohmann::json::parse(msg);
            std::lock_guard<std::mutex> lock(loc_mutex);
            loc->latitude = j["latitude"];
            loc->longitude = j["longitude"];
            loc->altitude = j.value("altitude", 0.0f);
            loc->timestamp = j.value("timestamp", "unknown");
            save_to_json(*loc);
        } catch (...) {
            std::cout << "Invalid JSON" << std::endl;
        }

        socket.send(zmq::buffer(std::string("Hello from Server!")), zmq::send_flags::none);
    }
}

void run_gui(Location* loc) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Location Monitor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Location Info");
        {
            std::lock_guard<std::mutex> lock(loc_mutex);
            ImGui::Text("Latitude:  %.6f", loc->latitude);
            ImGui::Text("Longitude: %.6f", loc->longitude);
            ImGui::Text("Altitude:  %.2f", loc->altitude);
            ImGui::Text("Time: %s", loc->timestamp.c_str());
        }
        ImGui::End();

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    static Location locationInfo;

    std::thread server_thread(run_server, &locationInfo);
    std::thread gui_thread(run_gui, &locationInfo);

    gui_thread.join();
    server_thread.join();

    return 0;
}
