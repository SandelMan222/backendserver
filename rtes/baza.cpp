#include "baza.h"
#include <iostream>

PGconn* podklyuchitBazu() {
    PGconn* soedinenie = PQconnectdb("host=localhost dbname=cellinfo user=postgres password=postgres");
    if (PQstatus(soedinenie) != CONNECTION_OK) {
        std::cout << "Oshibka podklyucheniya k BD: " << PQerrorMessage(soedinenie) << std::endl;
        PQfinish(soedinenie);
        return nullptr;
    }
    return soedinenie;
}

std::string sqlZnachenie(int v) {
    return (v == 2147483647) ? "NULL" : std::to_string(v);
}

void vstavitDannye(PGconn* soedinenie, const Lokatsiya& lok, const std::vector<LteYacheika>& yacheyki) {
    if (!soedinenie || yacheyki.empty()) return;

    std::string zapros_izm =
        "INSERT INTO measurements (timestamp, latitude, longitude, altitude, accuracy) VALUES ('" +
        lok.vremya + "', " +
        std::to_string(lok.shirota) + ", " +
        std::to_string(lok.dolgota) + ", " +
        std::to_string(lok.vysota) + ", " +
        std::to_string(lok.tochnost) + ") RETURNING id;";

    PGresult* rezultat = PQexec(soedinenie, zapros_izm.c_str());
    if (PQresultStatus(rezultat) != PGRES_TUPLES_OK) {
        std::cout << "Oshibka vstavki measurement: " << PQerrorMessage(soedinenie) << std::endl;
        PQclear(rezultat);
        return;
    }

    int id_izm = std::stoi(PQgetvalue(rezultat, 0, 0));
    PQclear(rezultat);

    for (const auto& yacheyka : yacheyki) {
        std::string zapros_lte =
            "INSERT INTO lte_cells (meas_id, pci, earfcn, band, rsrp, rsrq, rssi, asu, cid, tac, mcc, mnc) VALUES (" +
            std::to_string(id_izm) + ", " +
            std::to_string(yacheyka.pci) + ", " +
            std::to_string(yacheyka.earfcn) + ", " +
            yacheyka.polos + ", " +
            sqlZnachenie(yacheyka.rsrp) + ", " +
            sqlZnachenie(yacheyka.rsrq) + ", " +
            sqlZnachenie(yacheyka.rssi) + ", " +
            sqlZnachenie(yacheyka.asu) + ", " +
            std::to_string(yacheyka.cid) + ", " +
            std::to_string(yacheyka.tac) + ", " +
            (yacheyka.mcc.empty() ? "NULL" : "'" + yacheyka.mcc + "'") + ", " +
            (yacheyka.mnc.empty() ? "NULL" : "'" + yacheyka.mnc + "'") + ");";

        PGresult* r = PQexec(soedinenie, zapros_lte.c_str());
        if (PQresultStatus(r) != PGRES_COMMAND_OK)
            std::cout << "Oshibka vstavki yacheyki: " << PQerrorMessage(soedinenie) << std::endl;
        PQclear(r);
    }
}
