#include "gui.h"
#include "heatmap_engine.h"
#include <SDL2/SDL.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include "implot.h"
#include "osm_map.h"

void zapustit_gui(DeviceData* data) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Cell Info Monitor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1400, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
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
                ImGui::Text("MCC: %s  MNC: %s  TAC: %d  EARFCN: %d",
                    c.mcc.c_str(), c.mnc.c_str(), c.tac, c.earfcn);
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
                ImGui::Text("CID: %d  BSIC: %d  ARFCN: %d  LAC: %d",
                    c.cid, c.bsic, c.arfcn, c.lac);
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
                ImGui::Text("MCC: %s  MNC: %s  TAC: %d  NRARFCN: %d",
                    c.mcc.c_str(), c.mnc.c_str(), c.tac, c.nrarfcn);
                ImGui::Text("SS-RSRP: %d  SS-RSRQ: %d  SS-SINR: %d  TA: %d",
                    c.ss_rsrp, c.ss_rsrq, c.ss_sinr, c.timing_advance);
                ImGui::Separator();
            }
            if (data->nr.empty()) ImGui::Text("No NR data");
        }
        ImGui::End();

        ImGui::Begin("Signal Strength by PCI");
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            if (!gPciHistory.empty()) {
                if (ImPlot::BeginPlot("RSRP by PCI", ImVec2(-1, 250))) {
                    ImPlot::SetupAxes("Time (s)", "dBm");
                    for (auto& [pci, h] : gPciHistory) {
                        std::vector<float> t(h.time_axis.begin(), h.time_axis.end());
                        std::vector<float> rsrp(h.rsrp.begin(), h.rsrp.end());
                        ImPlot::PlotLine(("PCI " + std::to_string(pci)).c_str(),
                            t.data(), rsrp.data(), (int)t.size());
                    }
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot("RSSI by PCI", ImVec2(-1, 200))) {
                    ImPlot::SetupAxes("Time (s)", "dBm");
                    for (auto& [pci, h] : gPciHistory) {
                        std::vector<float> t(h.time_axis.begin(), h.time_axis.end());
                        std::vector<float> rssi(h.rssi.begin(), h.rssi.end());
                        ImPlot::PlotLine(("PCI " + std::to_string(pci)).c_str(),
                            t.data(), rssi.data(), (int)t.size());
                    }
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot("RSRQ by PCI", ImVec2(-1, 200))) {
                    ImPlot::SetupAxes("Time (s)", "dBm");
                    for (auto& [pci, h] : gPciHistory) {
                        std::vector<float> t(h.time_axis.begin(), h.time_axis.end());
                        std::vector<float> rsrq(h.rsrq.begin(), h.rsrq.end());
                        ImPlot::PlotLine(("PCI " + std::to_string(pci)).c_str(),
                            t.data(), rsrq.data(), (int)t.size());
                    }
                    ImPlot::EndPlot();
                }
            } else {
                ImGui::Text("Waiting for data...");
            }
        }
        ImGui::End();

        ImGui::Begin("Heatmap Generator");
        {
            static int tekushchiyKriteriy = 0;
            static int tekushchiyEarfcn = 100;
            static int radiusMetry = 25;
            static int shirinaKarty = 512;
            static int vysotaKarty = 512;
            static bool dlyaKazhdogoTaylya = false;

            ImGui::Combo("Kriteriy", &tekushchiyKriteriy, "RSRP\0RSRQ\0RSSI\0Vysota\0");
            ImGui::InputInt("EARFCN", &tekushchiyEarfcn);
            ImGui::SliderInt("Radius (m)", &radiusMetry, 10, 100);
            ImGui::SliderInt("Width", &shirinaKarty, 256, 1024);
            ImGui::SliderInt("Height", &vysotaKarty, 256, 1024);
            ImGui::Checkbox("Generate per Tile", &dlyaKazhdogoTaylya);

            ImGui::Separator();

            if (ImGui::Button("Start Generation", ImVec2(200, 0))) {
                ParametryGeneracii par;
                par.kriteriy = static_cast<KriteriyTeplovoyKarty>(tekushchiyKriteriy);
                par.earfcn = tekushchiyEarfcn;
                par.radiusMetry = radiusMetry;
                par.shirinaKarty = shirinaKarty;
                par.vysotaKarty = vysotaKarty;
                par.dlyaKazhdogoTaylya = dlyaKazhdogoTaylya;
                par.putySohraneniya = "./build";
                zapustit_generaciyu_v_potoke(par);
            }

            ImGui::SameLine();

            if (ImGui::Button("Stop Generation", ImVec2(200, 0))) {
                ostanavliv_generaciyu();
            }

            ImGui::Separator();

            {
                std::lock_guard<std::mutex> lock(statusGeneracii.mtx);
                if (statusGeneracii.vypolnyaetsya) {
                    ImGui::ProgressBar(statusGeneracii.progress, ImVec2(-1, 0));
                }
                ImGui::TextWrapped("Status: %s", statusGeneracii.soobshchenie.c_str());
            }
        }
        ImGui::End();

        otrisovat_okno_karty();

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    for (auto& [k, t] : tileCache)
        if (t.texId) glDeleteTextures(1, &t.texId);

    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();
}