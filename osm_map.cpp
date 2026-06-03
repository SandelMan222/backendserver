#include "osm_map.h"
#include <cmath>
#include <numbers>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "imgui.h"

int POW2[19];
double mapCenterLat = 54.9884;
double mapCenterLon = 82.8981;
int mapZoom = 12;

std::map<TileKey, Tile> tileCache;
std::atomic<int> tilesLoaded{0};
std::atomic<int> tilesFailed{0};

static const int TILE_PX = 256;
static const double MAP_PI  = std::numbers::pi_v<double>;
static const double MAP_PI2 = MAP_PI * 2.0;
static const double MAP_RAD = MAP_PI / 180.0;
static const double MAP_DEG = 180.0 / MAP_PI;

const char* TILE_SERVERS[] = {
    "https://a.tile.openstreetmap.fr/osmfr",
    "https://b.tile.openstreetmap.fr/osmfr",
    "https://c.tile.openstreetmap.fr/osmfr",
    nullptr
};

const ImPlotAxisFlags MAP_AXIS_FLAGS =
    ImPlotAxisFlags_NoLabel      | ImPlotAxisFlags_NoGridLines  |
    ImPlotAxisFlags_NoTickMarks  | ImPlotAxisFlags_NoTickLabels |
    ImPlotAxisFlags_NoInitialFit | ImPlotAxisFlags_NoMenus      |
    ImPlotAxisFlags_NoHighlight;

void init_stepeni2() { 
    for (int i = 0; i < 19; ++i) POW2[i] = (1 << i); 
}

double dolgota_v_x(double lon, int z) { 
    return (lon + 180.0) / 360.0 * POW2[z]; 
}

double shirota_v_y(double lat, int z) { 
    return (1.0 - asinh(tan(lat * MAP_RAD)) / MAP_PI) / 2.0 * POW2[z]; 
}

double x_v_dolgotu(double x, int z) { 
    return x / POW2[z] * 360.0 - 180.0; 
}

// Преобразование Y-координаты обратно в широту (обратная проекция Меркатора).
double y_v_shirotu(double y, int z) {
    const double n = MAP_PI - MAP_PI2 * y / POW2[z];
    return MAP_DEG * atan(0.5 * (exp(n) - exp(-n)));
}

size_t kolbek_zapisi_curl(void *data, size_t size, size_t nmemb, void *userp) {
    size_t n = size * nmemb;
    auto &b = *static_cast<std::vector<unsigned char>*>(userp);
    auto *p = static_cast<unsigned char*>(data);
    b.insert(b.cend(), p, p + n);
    return n;
}

std::string dir_taylov(int z, int x) {
    return "tiles/" + std::to_string(z) + "/" + std::to_string(x);
}

std::string put_k_taylu(int z, int x, int y) {
    return dir_taylov(z, x) + "/" + std::to_string(y) + ".png";
}

std::vector<unsigned char> skachat_tayl(int z, int x, int y) {
    std::string path = put_k_taylu(z, x, y);
    if (std::filesystem::exists(path)) {
        std::ifstream f(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(f), {}};
    }
    
    for (int si = 0; TILE_SERVERS[si]; ++si) {
        std::ostringstream ss;
        ss << TILE_SERVERS[si] << "/" << z << "/" << x << "/" << y << ".png";
        std::string url = ss.str();
        std::vector<unsigned char> blob;
        CURL *curl = curl_easy_init();
        
        if (!curl) continue;
        curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS,     1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT,      "OSMViewer/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,        20L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,      (void*)&blob);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  kolbek_zapisi_curl);
        
        CURLcode res = curl_easy_perform(curl);
        long http = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK && http == 200 && !blob.empty()) {
            std::filesystem::create_directories(dir_taylov(z, x));
            std::ofstream f(path, std::ios::binary);
            f.write(reinterpret_cast<char*>(blob.data()), blob.size());
            tilesLoaded++;
            return blob;
        }
        tilesFailed++;
        std::cerr << "[tile] fail " << url << " curl=" << curl_easy_strerror(res) << " http=" << http << "\n";
    }
    return {};
}

GLuint zagruzit_teksturu(const std::vector<unsigned char> &blob) {
    if (blob.empty()) return 0;
    int w, h, ch;
    unsigned char *px = stbi_load_from_memory(blob.data(), (int)blob.size(), &w, &h, &ch, STBI_rgb_alpha);
    if (!px) return 0;
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);
    return id;
}

Tile& poluchit_tayl(int z, int tx, int ty) {
    TileKey key{z, tx, ty};
    auto it = tileCache.find(key);
    if (it != tileCache.end()) return it->second;
    Tile t;
    auto blob = skachat_tayl(z, tx, ty);
    t.texId  = zagruzit_teksturu(blob);
    t.loaded = (t.texId != 0);
    tileCache[key] = t;
    return tileCache[key];
}

void otrisovat_okno_karty() {
    ImGui::SetNextWindowSize({700, 500}, ImGuiCond_FirstUseEver);
    ImGui::Begin("OSM Map");

    ImGui::Text("Tiles loaded: %d  |  failed: %d  |  in memory: %zu",
        tilesLoaded.load(), tilesFailed.load(), tileCache.size());
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear")) tileCache.clear();
    ImGui::SameLine();
    if (ImGui::SliderInt("Zoom", &mapZoom, 0, 18)) tileCache.clear();

    ImVec2 winSize = ImGui::GetContentRegionAvail();
    if (winSize.x < 10) winSize.x = 10;
    if (winSize.y < 10) winSize.y = 10;

    double cx = dolgota_v_x(mapCenterLon, mapZoom);
    double cy = shirota_v_y(mapCenterLat, mapZoom);
    double halfTX = winSize.x * 0.5 / TILE_PX;
    double halfTY = winSize.y * 0.5 / TILE_PX;

    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0,0));
    ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.15f,0.15f,0.15f,1.0f));

    if (ImPlot::BeginPlot("##map", winSize,
        ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
        ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText))
    {
        ImPlot::SetupAxes(nullptr, nullptr, MAP_AXIS_FLAGS, MAP_AXIS_FLAGS | ImPlotAxisFlags_Invert);
        
        ImPlot::SetupAxisLimits(ImAxis_X1, cx-halfTX, cx+halfTX, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, cy-halfTY, cy+halfTY, ImGuiCond_Always);

        int maxT = POW2[mapZoom];
        int tx0 = std::max(0, (int)std::floor(cx - halfTX) - 1);
        int tx1 = std::min(maxT-1, (int)std::ceil (cx + halfTX) + 1);
        int ty0 = std::max(0, (int)std::floor(cy - halfTY) - 1);
        int ty1 = std::min(maxT-1, (int)std::ceil (cy + halfTY) + 1);

        for (int ty = ty0; ty <= ty1; ++ty)
            for (int tx = tx0; tx <= tx1; ++tx) {
                Tile &tile = poluchit_tayl(mapZoom, tx, ty);
                if (!tile.loaded) continue;
                
                ImPlot::PlotImage("##t",
                    (ImTextureID)(intptr_t)tile.texId,
                    ImPlotPoint(tx,     ty),
                    ImPlotPoint(tx+1.0, ty+1.0),
                    ImVec2(0,1), ImVec2(1,0));
            }

        if (ImPlot::IsPlotHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
            double tpx = 2.0 * halfTX / winSize.x;
            double tpy = 2.0 * halfTY / winSize.y;
            mapCenterLon = x_v_dolgotu(cx - delta.x * tpx, mapZoom);
            mapCenterLat = y_v_shirotu(cy - delta.y * tpy, mapZoom);
        }

        if (ImPlot::IsPlotHovered()) {
            ImGuiIO &io = ImGui::GetIO();
            if (io.MouseWheel != 0) {
                mapZoom = std::clamp(mapZoom + (io.MouseWheel > 0 ? 1 : -1), 0, 18);
                tileCache.clear();
            }
        }

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImPlot::PopStyleVar();

    ImGui::End();
}