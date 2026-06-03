#pragma once

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>

struct TochkaIzmereniya {
    double shirota;
    double dolgota;
    double vysota;
    double znachenie;
};

enum class KriteriyTeplovoyKarty {
    RSRP,
    RSRQ,
    RSSI,
    Vysota
};

struct ParametryGeneracii {
    KriteriyTeplovoyKarty kriteriy;
    int earfcn;
    int radiusMetry;
    int shirinaKarty;
    int vysotaKarty;
    bool dlyaKazhdogoTaylya;
    std::string putySohraneniya;
};

struct TsvetnayaMapa {
    int r, g, b;
};

struct StatusGeneracii {
    bool vypolnyaetsya;
    float progress;
    std::string soobshchenie;
    std::mutex mtx;
};

extern std::mutex mtkaryty;
extern std::atomic<bool> zaprosStopaGeneration;
extern StatusGeneracii statusGeneracii;
