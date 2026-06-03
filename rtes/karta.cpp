#include "karta.h"
#include "tajly.h"
#include "globalnye.h"
#include "imgui.h"
#include "implot.h"
#include <GL/glew.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <cmath>
#include <map>
#include <tuple>
#include <vector>
#include <string>
#include <set>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

static int tekushijZoom = 14;
static double s_mx = 0.0, s_my = 0.0; // Смещение карты

using KlyuchTajla = std::tuple<int, int, int>;

struct DannyeTajla {
    GLuint tekstura = 0;
    bool gotov = false;
    bool oshibka = false;
};

static std::map<KlyuchTajla, DannyeTajla> g_keshTajlov;

void pokazatKartu(DannyeUstrojstva* dannye) {
    ImGui::Begin("Карта");

    ImVec2 razmer = ImGui::GetContentRegionAvail();
    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
    
    // Стандартные границы без ImRect
    ImVec2 rectMin = cursorScreenPos;
    ImVec2 rectMax = ImVec2(cursorScreenPos.x + razmer.x, cursorScreenPos.y + razmer.y);

    // Обработка ввода
    if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(rectMin, rectMax)) {
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            double scale = pow(2.0, tekushijZoom);
            s_mx -= io.MouseDelta.x / (256.0 * scale);
            s_my -= io.MouseDelta.y / (256.0 * scale);
        }
        if (io.MouseWheel != 0.0f) {
            tekushijZoom += (io.MouseWheel > 0 ? 1 : -1);
            if (tekushijZoom < 0) tekushijZoom = 0;
            if (tekushijZoom > 19) tekushijZoom = 19;
        }
    }

    ImGui::Dummy(razmer);
    ImGui::End();
}