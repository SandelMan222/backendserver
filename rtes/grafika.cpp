#include "grafika.h"
#include "globalnye.h"
#include "karta.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <SDL.h>
#include <GL/glew.h>
#include <vector>
#include <string>

void zapustitGrafiku(DannyeUstrojstva* dannye) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* okno = SDL_CreateWindow("Cell Info Monitor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1200, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_GLContext kontekst_gl = SDL_GL_CreateContext(okno);
    SDL_GL_MakeCurrent(okno, kontekst_gl);
    SDL_GL_SetSwapInterval(1);
    glewExperimental = GL_TRUE;
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(okno, kontekst_gl);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool rabotaet = true;
    while (rabotaet) {
        SDL_Event sobytie;
        while (SDL_PollEvent(&sobytie)) {
            ImGui_ImplSDL2_ProcessEvent(&sobytie);
            if (sobytie.type == SDL_QUIT) { rabotaet = false; server_rabotaet = false; }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Location Info");
        {
            std::lock_guard<std::mutex> zamok(mutex_dannyh);
            ImGui::Text("Shirota:   %.6f", dannye->lokatsiya.shirota);
            ImGui::Text("Dolgota:   %.6f", dannye->lokatsiya.dolgota);
            ImGui::Text("Vysota:    %.2f", dannye->lokatsiya.vysota);
            ImGui::Text("Tochnost:  %.2f", dannye->lokatsiya.tochnost);
            ImGui::Text("Vremya: %s", dannye->lokatsiya.vremya.c_str());
        }
        ImGui::End();

        ImGui::Begin("LTE Yacheyki");
        {
            std::lock_guard<std::mutex> zamok(mutex_dannyh);
            int nomer = 0;
            for (auto& y : dannye->lte) {
                ImGui::Text("--- Yacheyka %d ---", nomer++);
                ImGui::Text("Polos: %s  PCI: %d  CID: %d", y.polos.c_str(), y.pci, y.cid);
                ImGui::Text("MCC: %s  MNC: %s  TAC: %d  EARFCN: %d", y.mcc.c_str(), y.mnc.c_str(), y.tac, y.earfcn);
                ImGui::Text("ASU: %d  RSRP: %d  RSRQ: %d  RSSI: %d  RSSNR: %d  CQI: %d  TA: %d",
                    y.asu, y.rsrp, y.rsrq, y.rssi, y.rssnr, y.cqi, y.zapazdyvanie);
                ImGui::Separator();
            }
            if (dannye->lte.empty()) ImGui::Text("Net dannyh LTE");
        }
        ImGui::End();

        ImGui::Begin("GSM Yacheyki");
        {
            std::lock_guard<std::mutex> zamok(mutex_dannyh);
            int nomer = 0;
            for (auto& y : dannye->gsm) {
                ImGui::Text("--- Yacheyka %d ---", nomer++);
                ImGui::Text("CID: %d  BSIC: %d  ARFCN: %d  LAC: %d", y.cid, y.bsic, y.arfcn, y.lac);
                ImGui::Text("MCC: %s  MNC: %s  DBM: %d  RSSI: %d  TA: %d",
                    y.mcc.c_str(), y.mnc.c_str(), y.dbm, y.rssi, y.zapazdyvanie);
                ImGui::Separator();
            }
            if (dannye->gsm.empty()) ImGui::Text("Net dannyh GSM");
        }
        ImGui::End();

        ImGui::Begin("NR (5G) Yacheyki");
        {
            std::lock_guard<std::mutex> zamok(mutex_dannyh);
            int nomer = 0;
            for (auto& y : dannye->nr) {
                ImGui::Text("--- Yacheyka %d ---", nomer++);
                ImGui::Text("Polos: %s  PCI: %d  NCI: %lld", y.polos.c_str(), y.pci, y.nci);
                ImGui::Text("MCC: %s  MNC: %s  TAC: %d  NRARFCN: %d", y.mcc.c_str(), y.mnc.c_str(), y.tac, y.nrarfcn);
                ImGui::Text("SS-RSRP: %d  SS-RSRQ: %d  SS-SINR: %d  TA: %d",
                    y.ss_rsrp, y.ss_rsrq, y.ss_sinr, y.zapazdyvanie);
                ImGui::Separator();
            }
            if (dannye->nr.empty()) ImGui::Text("Net dannyh NR");
        }
        ImGui::End();

        ImGui::Begin("Sila signala po PCI");
        {
            std::lock_guard<std::mutex> zamok(mutex_dannyh);
            if (!historiya_pci.empty()) {
                if (ImPlot::BeginPlot("RSRP po PCI", ImVec2(-1, 250))) {
                    ImPlot::SetupAxes("Vremya (s)", "dBm");
                    for (auto& [pci, h] : historiya_pci) {
                        std::vector<float> t(h.os_vremeni.begin(), h.os_vremeni.end());
                        std::vector<float> rsrp(h.rsrp.begin(), h.rsrp.end());
                        std::string metka = "PCI " + std::to_string(pci);
                        ImPlot::PlotLine(metka.c_str(), t.data(), rsrp.data(), (int)t.size());
                    }
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot("RSSI po PCI", ImVec2(-1, 200))) {
                    ImPlot::SetupAxes("Vremya (s)", "dBm");
                    for (auto& [pci, h] : historiya_pci) {
                        std::vector<float> t(h.os_vremeni.begin(), h.os_vremeni.end());
                        std::vector<float> rssi(h.rssi.begin(), h.rssi.end());
                        std::string metka = "PCI " + std::to_string(pci);
                        ImPlot::PlotLine(metka.c_str(), t.data(), rssi.data(), (int)t.size());
                    }
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot("RSRQ po PCI", ImVec2(-1, 200))) {
                    ImPlot::SetupAxes("Vremya (s)", "dBm");
                    for (auto& [pci, h] : historiya_pci) {
                        std::vector<float> t(h.os_vremeni.begin(), h.os_vremeni.end());
                        std::vector<float> rsrq(h.rsrq.begin(), h.rsrq.end());
                        std::string metka = "PCI " + std::to_string(pci);
                        ImPlot::PlotLine(metka.c_str(), t.data(), rsrq.data(), (int)t.size());
                    }
                    ImPlot::EndPlot();
                }
            } else {
                ImGui::Text("Ozhidanie dannyh...");
            }
        }
        ImGui::End();

        pokazatKartu(dannye);

        ImGui::Render();
        int display_w, display_h;
        SDL_GL_GetDrawableSize(okno, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.07f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(okno);
    }

    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(kontekst_gl);
    SDL_DestroyWindow(okno);
    SDL_Quit();
}
