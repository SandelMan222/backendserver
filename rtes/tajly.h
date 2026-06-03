#pragma once
#include <vector>
#include <string>

// пересчёт координат в номера тайлов
int lonVX(double lon, int z);
int latVY(double lat, int z);

// обратный пересчёт
double xVLon(int x, int z);
double yVLat(int y, int z);

// путь к кэшированному тайлу
std::string putKTajlu(int z, int x, int y);

// получить тайл: из кэша или скачать
std::vector<unsigned char> poluchitTajl(int z, int x, int y);
