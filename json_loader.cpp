#include "json_loader.h"
#include <fstream>
#include <iostream>
#include <cmath>

// Извлекает параметры сетей из JSON и сохраняет их в DeviceData, обновляя графики.
void obrabotat_json(const nlohmann::json& j, DeviceData* data, PGconn* conn) {
    if (j.contains("location")) {
        auto l = j["location"];
        data->location.latitude  = l.value("latitude",  0.0);
        data->location.longitude = l.value("longitude", 0.0);
        data->location.altitude  = l.value("altitude",  0.0);
        data->location.accuracy  = l.value("accuracy",  0.0);
        data->location.timestamp = l.value("timestamp", "unknown");
    }
    data->lte.clear();
    
    if (j.contains("lte")) {
        for (auto& cell : j["lte"]) {
            LteCell c;
            c.band   = std::to_string(cell.value("band", 0));
            c.mcc    = cell.value("mcc", "");
            c.mnc    = cell.value("mnc", "");
            c.cid    = cell.value("cid", 0);
            c.earfcn = cell.value("earfcn", 0);
            c.pci    = cell.value("pci", 0);
            c.tac    = cell.value("tac", 0);
            c.asu    = cell.value("asu", 0);
            c.cqi    = cell.value("cqi", 0);
            c.rsrp   = cell.value("rsrp", 0);
            c.rsrq   = cell.value("rsrq", 0);
            c.rssi   = cell.value("rssi", 0);
            c.rssnr  = cell.value("rssnr", 0);
            c.timing_advance = cell.value("timing_advance", 0);
            data->lte.push_back(c);
        }
        if (conn) vstavit_v_bd(conn, data->location, data->lte);
    }
    
    gTimeCounter += 1.0f;
    for (auto& c : data->lte) {
        auto& h = gPciHistory[c.pci];
        if ((int)h.rsrp.size() >= MAX_POINTS) {
            h.rsrp.pop_front(); h.rssi.pop_front();
            h.rsrq.pop_front(); h.time_axis.pop_front();
        }
        h.rsrp.push_back((c.rsrp == 2147483647) ? NAN : (float)c.rsrp);
        h.rssi.push_back((c.rssi == 2147483647) ? NAN : (float)c.rssi);
        h.rsrq.push_back((c.rsrq == 2147483647) ? NAN : (float)c.rsrq);
        h.time_axis.push_back(gTimeCounter);
    }
    
    data->gsm.clear();
    if (j.contains("gsm")) {
        for (auto& cell : j["gsm"]) {
            GsmCell c;
            c.mcc  = cell.value("mcc", "");  c.mnc = cell.value("mnc", "");
            c.cid  = cell.value("cid", 0);   c.bsic = cell.value("bsic", 0);
            c.arfcn= cell.value("arfcn", 0); c.lac  = cell.value("lac", 0);
            c.dbm  = cell.value("dbm", 0);   c.rssi = cell.value("rssi", 0);
            c.timing_advance = cell.value("timing_advance", 0);
            data->gsm.push_back(c);
        }
    }
    
    data->nr.clear();
    if (j.contains("nr")) {
        for (auto& cell : j["nr"]) {
            NrCell c;
            c.band    = std::to_string(cell.value("band", 0));
            c.mcc     = cell.value("mcc", "");   c.mnc = cell.value("mnc", "");
            c.nci     = cell.value("nci", 0LL);  c.pci = cell.value("pci", 0);
            c.nrarfcn = cell.value("nrarfcn", 0);c.tac = cell.value("tac", 0);
            c.ss_rsrp = cell.value("ss_rsrp", 0);c.ss_rsrq = cell.value("ss_rsrq", 0);
            c.ss_sinr = cell.value("ss_sinr", 0);
            c.timing_advance = cell.value("timing_advance", 0);
            data->nr.push_back(c);
        }
    }
}

void zagruzit_fayl_json(DeviceData* data) {
    PGconn* conn = podklyuchit_bd();
    std::ifstream infile("locations.json");
    if (!infile.good()) {
        std::cout << "Error: locations.json not found!" << std::endl;
        if (conn) PQfinish(conn);
        return;
    }
    if (conn) PQexec(conn, "BEGIN;");
    int count = 0;
    try {
        while (infile.peek() != EOF) {
            while (isspace(infile.peek()) || infile.peek() == ',') infile.ignore();
            if (infile.peek() == EOF) break;
            nlohmann::json j;
            infile >> j;
            if (j.is_array()) {
                for (auto& element : j) { obrabotat_json(element, data, conn); count++; }
            } else if (j.is_object()) {
                obrabotat_json(j, data, conn); count++;
            }
            if (count % 1000 == 0 && count > 0)
                std::cout << "Processed " << count << " records..." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Parsing finished or interrupted: " << e.what() << std::endl;
    }
    if (conn) { PQexec(conn, "COMMIT;"); PQfinish(conn); }
    std::cout << "Done! Total records processed: " << count << std::endl;
}