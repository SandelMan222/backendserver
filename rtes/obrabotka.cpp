#include "obrabotka.h"
#include "baza.h"
#include "globalnye.h"
#include <iostream>
#include <fstream>
#include <cmath>

void obrabotatJson(const nlohmann::json& j, DannyeUstrojstva* dannye, PGconn* soedinenie) {
    if (j.contains("location")) {
        auto l = j["location"];
        dannye->lokatsiya.shirota  = l.value("latitude",  0.0);
        dannye->lokatsiya.dolgota  = l.value("longitude", 0.0);
        dannye->lokatsiya.vysota   = l.value("altitude",  0.0);
        dannye->lokatsiya.tochnost = l.value("accuracy",  0.0);
        dannye->lokatsiya.vremya   = l.value("timestamp", "unknown");
    }

    dannye->lte.clear();
    if (j.contains("lte")) {
        for (auto& yacheyka : j["lte"]) {
            LteYacheika y;
            y.polos       = std::to_string(yacheyka.value("band", 0));
            y.mcc         = yacheyka.value("mcc", "");
            y.mnc         = yacheyka.value("mnc", "");
            y.cid         = yacheyka.value("cid", 0);
            y.earfcn      = yacheyka.value("earfcn", 0);
            y.pci         = yacheyka.value("pci", 0);
            y.tac         = yacheyka.value("tac", 0);
            y.asu         = yacheyka.value("asu", 0);
            y.cqi         = yacheyka.value("cqi", 0);
            y.rsrp        = yacheyka.value("rsrp", 0);
            y.rsrq        = yacheyka.value("rsrq", 0);
            y.rssi        = yacheyka.value("rssi", 0);
            y.rssnr       = yacheyka.value("rssnr", 0);
            y.zapazdyvanie = yacheyka.value("timing_advance", 0);
            dannye->lte.push_back(y);
        }
        if (soedinenie) vstavitDannye(soedinenie, dannye->lokatsiya, dannye->lte);
    }

    schetchik_vremeni += 1.0f;
    for (auto& y : dannye->lte) {
        auto& h = historiya_pci[y.pci];
        if (h.rsrp.size() >= MAKS_TOCHEK) {
            h.rsrp.pop_front();
            h.rssi.pop_front();
            h.rsrq.pop_front();
            h.os_vremeni.pop_front();
        }
        h.rsrp.push_back((y.rsrp == 2147483647) ? NAN : (float)y.rsrp);
        h.rssi.push_back((y.rssi == 2147483647) ? NAN : (float)y.rssi);
        h.rsrq.push_back((y.rsrq == 2147483647) ? NAN : (float)y.rsrq);
        h.os_vremeni.push_back(schetchik_vremeni);
    }

    dannye->gsm.clear();
    if (j.contains("gsm")) {
        for (auto& yacheyka : j["gsm"]) {
            GsmYacheika y;
            y.mcc          = yacheyka.value("mcc", "");
            y.mnc          = yacheyka.value("mnc", "");
            y.cid          = yacheyka.value("cid", 0);
            y.bsic         = yacheyka.value("bsic", 0);
            y.arfcn        = yacheyka.value("arfcn", 0);
            y.lac          = yacheyka.value("lac", 0);
            y.dbm          = yacheyka.value("dbm", 0);
            y.rssi         = yacheyka.value("rssi", 0);
            y.zapazdyvanie = yacheyka.value("timing_advance", 0);
            dannye->gsm.push_back(y);
        }
    }

    dannye->nr.clear();
    if (j.contains("nr")) {
        for (auto& yacheyka : j["nr"]) {
            NrYacheika y;
            y.polos        = std::to_string(yacheyka.value("band", 0));
            y.mcc          = yacheyka.value("mcc", "");
            y.mnc          = yacheyka.value("mnc", "");
            y.nci          = yacheyka.value("nci", 0LL);
            y.pci          = yacheyka.value("pci", 0);
            y.nrarfcn      = yacheyka.value("nrarfcn", 0);
            y.tac          = yacheyka.value("tac", 0);
            y.ss_rsrp      = yacheyka.value("ss_rsrp", 0);
            y.ss_rsrq      = yacheyka.value("ss_rsrq", 0);
            y.ss_sinr      = yacheyka.value("ss_sinr", 0);
            y.zapazdyvanie = yacheyka.value("timing_advance", 0);
            dannye->nr.push_back(y);
        }
    }
}

void zagruzitFajl(DannyeUstrojstva* dannye) {
    PGconn* soedinenie = podklyuchitBazu();
    std::ifstream fajl("locations.json");

    if (!fajl.good()) {
        std::cout << "locations.json ne najden" << std::endl;
        if (soedinenie) PQfinish(soedinenie);
        return;
    }

    if (soedinenie) PQexec(soedinenie, "BEGIN;");

    int schetchik = 0;
    try {
        while (fajl.peek() != EOF) {
            while (isspace(fajl.peek()) || fajl.peek() == ',') fajl.ignore();
            if (fajl.peek() == EOF) break;

            nlohmann::json j;
            fajl >> j;

            if (j.is_array()) {
                for (auto& el : j) { obrabotatJson(el, dannye, soedinenie); schetchik++; }
            } else if (j.is_object()) {
                obrabotatJson(j, dannye, soedinenie); schetchik++;
            }

            if (schetchik % 1000 == 0)
                std::cout << "Zagruzheno: " << schetchik << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Zagruzka zavershena: " << e.what() << std::endl;
    }

    if (soedinenie) { PQexec(soedinenie, "COMMIT;"); PQfinish(soedinenie); }
    std::cout << "Vsego zapisej: " << schetchik << std::endl;
}
