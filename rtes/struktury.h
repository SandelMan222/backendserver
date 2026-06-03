#pragma once
#include <string>
#include <vector>
#include <deque>

struct LteYacheika {
    std::string polos, mcc, mnc;
    int cid, earfcn, pci, tac, asu, cqi, rsrp, rsrq, rssi, rssnr, zapazdyvanie;
};

struct GsmYacheika {
    std::string mcc, mnc;
    int cid, bsic, arfcn, lac, dbm, rssi, zapazdyvanie;
};

struct NrYacheika {
    std::string mcc, mnc, polos;
    long long nci;
    int pci, nrarfcn, tac, ss_rsrp, ss_rsrq, ss_sinr, zapazdyvanie;
};

struct Lokatsiya {
    double shirota  = 0.0;
    double dolgota  = 0.0;
    double vysota   = 0.0;
    double tochnost = 0.0;
    std::string vremya = "No data yet";
};

struct DannyeUstrojstva {
    Lokatsiya lokatsiya;
    std::vector<LteYacheika> lte;
    std::vector<GsmYacheika> gsm;
    std::vector<NrYacheika>  nr;
};

struct HistoriyaPci {
    std::deque<float> rsrp;
    std::deque<float> rssi;
    std::deque<float> rsrq;
    std::deque<float> os_vremeni;
};
