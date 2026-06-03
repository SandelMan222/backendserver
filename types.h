#pragma once

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <tuple>
#include <atomic>
#include <mutex>
#include <GL/glew.h>

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
    double latitude = 0.0, longitude = 0.0, altitude = 0.0, accuracy = 0.0;
    std::string timestamp = "No data yet";
};

struct DeviceData {
    Location location;
    std::vector<LteCell> lte;
    std::vector<GsmCell> gsm;
    std::vector<NrCell>  nr;
};

struct PciHistory {
    std::deque<float> rsrp, rssi, rsrq, time_axis;
};

struct Tile {
    GLuint texId{0};
    bool loaded{false};
};

using TileKey = std::tuple<int, int, int>;

extern std::mutex data_mutex;
extern std::atomic<bool> server_running;
extern std::map<int, PciHistory> gPciHistory;
extern float gTimeCounter;

extern std::map<TileKey, Tile> tileCache;
extern std::atomic<int> tilesLoaded;
extern std::atomic<int> tilesFailed;

const int MAX_POINTS = 100000;