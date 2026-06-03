#include "tajly.h"
#include "curl_zapros.h"
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
#ifdef __linux__
#include <unistd.h>
#endif

static const double PI = 3.14159265358979323846;
static const double RAD = PI / 180.0;
static const double DEG = 180.0 / PI;

int lonVX(double lon, int z) {
    return (int)floor((lon + 180.0) / 360.0 * pow(2.0, z));
}

int latVY(double lat, int z) {
    double rad = lat * RAD;
    return (int)floor((1.0 - asinh(tan(rad)) / PI) / 2.0 * pow(2.0, z));
}

double xVLon(int x, int z) {
    return x / pow(2.0, z) * 360.0 - 180.0;
}

double yVLat(int y, int z) {
    double n = PI - 2.0 * PI * y / pow(2.0, z);
    return DEG * atan(0.5 * (exp(n) - exp(-n)));
}

static std::filesystem::path getExecutablePath() {
#ifdef __linux__
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        return std::filesystem::path(buf);
    }
#endif
    return std::filesystem::current_path();
}

static std::filesystem::path getBuildRoot() {
    std::filesystem::path root = getExecutablePath().parent_path();
    if (root.filename() == "build") {
        root = root.parent_path();
    }
    root /= "build";
    return root;
}

std::string putKTajlu(int z, int x, int y) {
    std::filesystem::path root = getBuildRoot();
    root /= std::to_string(z);
    root /= std::to_string(x);
    root /= std::to_string(y) + ".png";
    return root.string();
}

static std::string putKTajluOld(int z, int x, int y) {
    std::filesystem::path root = getBuildRoot();
    root /= "tajly_kesh";
    root /= std::to_string(z);
    root /= std::to_string(x);
    root /= std::to_string(y) + ".png";
    return root.string();
}

std::vector<unsigned char> poluchitTajl(int z, int x, int y) {
    std::string put = putKTajlu(z, x, y);
    if (std::filesystem::exists(put)) {
        std::cout << "Loading tile from new cache: " << put << std::endl;
        std::ifstream fajl(put, std::ios::binary);
        return std::vector<unsigned char>(std::istreambuf_iterator<char>(fajl), {});
    }
    std::string putOld = putKTajluOld(z, x, y);
    if (std::filesystem::exists(putOld)) {
        std::cout << "Loading tile from old cache: " << putOld << std::endl;
        std::ifstream fajl(putOld, std::ios::binary);
        return std::vector<unsigned char>(std::istreambuf_iterator<char>(fajl), {});
    }
    std::vector<unsigned char> blob;
    if (skachatTajl(z, x, y, blob)) {
        std::filesystem::create_directories(std::filesystem::path(put).parent_path());
        std::ofstream fajl(put, std::ios::binary);
        fajl.write(reinterpret_cast<const char*>(blob.data()), blob.size());
        std::cout << "Tajl sokhranen: " << put << std::endl;
    } else {
        std::cout << "Failed to fetch tile: " << z << "/" << x << "/" << y << std::endl;
    }
    return blob;
}