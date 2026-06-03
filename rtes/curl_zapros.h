#pragma once
#include <vector>
#include <string>

// callback для libcurl
size_t onOtvet(void* data, size_t size, size_t nmemb, void* userp);

// скачать тайл с OSM сервера
bool skachatTajl(int z, int x, int y, std::vector<unsigned char>& blob);
