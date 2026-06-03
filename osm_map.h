#pragma once

#include <vector>
#include "types.h"
#include "implot.h"

void init_stepeni2();
double dolgota_v_x(double lon, int z);
double shirota_v_y(double lat, int z);
double x_v_dolgotu(double x, int z);
double y_v_shirotu(double y, int z);
size_t kolbek_zapisi_curl(void *data, size_t size, size_t nmemb, void *userp);
std::vector<unsigned char> skachat_tayl(int z, int x, int y);
GLuint zagruzit_teksturu(const std::vector<unsigned char> &blob);
Tile& poluchit_tayl(int z, int tx, int ty);
void otrisovat_okno_karty();

extern int POW2[19];
extern double mapCenterLat;
extern double mapCenterLon;
extern int mapZoom;
extern const char* TILE_SERVERS[];
extern const ImPlotAxisFlags MAP_AXIS_FLAGS;