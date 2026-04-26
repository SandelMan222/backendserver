#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <fstream>
#include <vector>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <GL/glew.h>
#include <atomic>

struct LteCell {
    std::string band, mcc, mnc;
    int cid, earfcn, pci, tac, asu, cqi, rsrp, rsrq, rssi, rssnr, timing_advance;
};

struct GsmCell {
    std::string mcc, mnc;
    int cid, bsic, arfcn, lac, dbm, rssi, timing_advance;
};

struct NrCell {
    std::string mcc, mnc, band;
    long long nci;
    int pci, nrarfcn, tac, ss_rsrp, ss_rsrq, ss_sinr, timing_advance;
};

struct Location {
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    double accuracy = 0.0;
    std::string timestamp = "No data yet";
};

struct DeviceData {
    Location location;
    std::vector<LteCell> lte;
    std::vector<GsmCell> gsm;
    std::vector<NrCell> nr;
};

std::mutex data_mutex;
std::atomic<bool> server_running{true};
DeviceData gData;

void run_server(DeviceData* data) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://0.0.0.0:5555");
    std::cout << "Server started on port 5555" << std::endl;

    while (server_running) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);
        std::string msg(static_cast<char*>(request.data()), request.size());
        std::cout << "Received: " << msg << std::endl;

        try {
            auto j = nlohmann::json::parse(msg);

            std::ifstream infile("locations.json");
            nlohmann::json arr = nlohmann::json::array();
            if (infile.good()) infile >> arr;
            arr.push_back(j);
            std::ofstream outfile("locations.json");
            outfile << arr.dump(4);

            std::lock_guard<std::mutex> lock(data_mutex);

            if (j.contains("location")) {
                auto l = j["location"];
                data->location.latitude  = l.value("latitude",  0.0);
                data->location.longitude = l.value("longitude", 0.0);
                data->location.altitude  = l.value("altitude",  0.0);
                data->location.accuracy  = l.value("accuracy",  0.0);
                data->location.timestamp = l.value("timestamp", "unknown");
            }

            data->lte.clear();
            if (j.contains("lte")) {
                for (auto& cell : j["lte"]) {
                    LteCell c;
                    c.band  = std::to_string(cell.value("band", 0));
                    c.mcc   = cell.value("mcc", "");
                    c.mnc   = cell.value("mnc", "");
                    c.cid   = cell.value("cid", 0);
                    c.earfcn= cell.value("earfcn", 0);
                    c.pci   = cell.value("pci", 0);
                    c.tac   = cell.value("tac", 0);
                    c.asu   = cell.value("asu", 0);
                    c.cqi   = cell.value("cqi", 0);
                    c.rsrp  = cell.value("rsrp", 0);
                    c.rsrq  = cell.value("rsrq", 0);
                    c.rssi  = cell.value("rssi", 0);
                    c.rssnr = cell.value("rssnr", 0);
                    c.timing_advance = cell.value("timing_advance", 0);
                    data->lte.push_back(c);
                }
            }

            data->gsm.clear();
            if (j.contains("gsm")) {
                for (auto& cell : j["gsm"]) {
                    GsmCell c;
                    c.mcc   = cell.value("mcc", "");
                    c.mnc   = cell.value("mnc", "");
                    c.cid   = cell.value("cid", 0);
                    c.bsic  = cell.value("bsic", 0);
                    c.arfcn = cell.value("arfcn", 0);
                    c.lac   = cell.value("lac", 0);
                    c.dbm   = cell.value("dbm", 0);
                    c.rssi  = cell.value("rssi", 0);
                    c.timing_advance = cell.value("timing_advance", 0);
                    data->gsm.push_back(c);
                }
            }

            data->nr.clear();
            if (j.contains("nr")) {
                for (auto& cell : j["nr"]) {
                    NrCell c;
                    c.band   = std::to_string(cell.value("band", 0));
                    c.mcc    = cell.value("mcc", "");
                    c.mnc    = cell.value("mnc", "");
                    c.nci    = cell.value("nci", 0LL);
                    c.pci    = cell.value("pci", 0);
                    c.nrarfcn= cell.value("nrarfcn", 0);
                    c.tac    = cell.value("tac", 0);
                    c.ss_rsrp= cell.value("ss_rsrp", 0);
                    c.ss_rsrq= cell.value("ss_rsrq", 0);
                    c.ss_sinr= cell.value("ss_sinr", 0);
                    c.timing_advance = cell.value("timing_advance", 0);
                    data->nr.push_back(c);
                }
            }

        } catch (const std::exception& e) {
            std::cout << "Invalid JSON: " << e.what() << std::endl;
        }

        socket.send(zmq::buffer(std::string("OK")), zmq::send_flags::none);
    }
}

void run_gui(DeviceData* data) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Cell Info Monitor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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
            if (event.type == SDL_QUIT) { running = false; server_running = false; }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Location Info");
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            ImGui::Text("Latitude:  %.6f", data->location.latitude);
            ImGui::Text("Longitude: %.6f", data->location.longitude);
            ImGui::Text("Altitude:  %.2f", data->location.altitude);
            ImGui::Text("Accuracy:  %.2f", data->location.accuracy);
            ImGui::Text("Time: %s", data->location.timestamp.c_str());
        }
        ImGui::End();

        ImGui::Begin("LTE Cells");
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            int idx = 0;
            for (auto& c : data->lte) {
                ImGui::Text("--- Cell %d ---", idx++);
                ImGui::Text("Band: %s  PCI: %d  CID: %d", c.band.c_str(), c.pci, c.cid);
                ImGui::Text("MCC: %s  MNC: %s  TAC: %d  EARFCN: %d", c.mcc.c_str(), c.mnc.c_str(), c.tac, c.earfcn);
                ImGui::Text("ASU: %d  RSRP: %d  RSRQ: %d  RSSI: %d  RSSNR: %d  CQI: %d  TA: %d",
                    c.asu, c.rsrp, c.rsrq, c.rssi, c.rssnr, c.cqi, c.timing_advance);
                ImGui::Separator();
            }
            if (data->lte.empty()) ImGui::Text("No LTE data");
        }
        ImGui::End();

        ImGui::Begin("GSM Cells");
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            int idx = 0;
            for (auto& c : data->gsm) {
                ImGui::Text("--- Cell %d ---", idx++);
                ImGui::Text("CID: %d  BSIC: %d  ARFCN: %d  LAC: %d", c.cid, c.bsic, c.arfcn, c.lac);
                ImGui::Text("MCC: %s  MNC: %s  DBM: %d  RSSI: %d  TA: %d",
                    c.mcc.c_str(), c.mnc.c_str(), c.dbm, c.rssi, c.timing_advance);
                ImGui::Separator();
            }
            if (data->gsm.empty()) ImGui::Text("No GSM data");
        }
        ImGui::End();

        ImGui::Begin("NR (5G) Cells");
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            int idx = 0;
            for (auto& c : data->nr) {
                ImGui::Text("--- Cell %d ---", idx++);
                ImGui::Text("Band: %s  PCI: %d  NCI: %lld", c.band.c_str(), c.pci, c.nci);
                ImGui::Text("MCC: %s  MNC: %s  TAC: %d  NRARFCN: %d", c.mcc.c_str(), c.mnc.c_str(), c.tac, c.nrarfcn);
                ImGui::Text("SS-RSRP: %d  SS-RSRQ: %d  SS-SINR: %d  TA: %d",
                    c.ss_rsrp, c.ss_rsrq, c.ss_sinr, c.timing_advance);
                ImGui::Separator();
            }
            if (data->nr.empty()) ImGui::Text("No NR data");
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
    static DeviceData deviceData;

    std::thread server_thread(run_server, &deviceData);
    std::thread gui_thread(run_gui, &deviceData);

    gui_thread.join();
    server_thread.join();

    return 0;
}