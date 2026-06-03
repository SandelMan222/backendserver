#include "db.h"
#include <iostream>

PGconn* podklyuchit_bd() {
    PGconn* conn = PQconnectdb("host=localhost dbname=cellinfo user=postgres password=postgres");
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cout << "DB connection failed: " << PQerrorMessage(conn) << std::endl;
        PQfinish(conn);
        return nullptr;
    }
    return conn;
}

std::string znachenie_sql(int v) {
    return (v == 2147483647) ? "NULL" : std::to_string(v);
}

void vstavit_v_bd(PGconn* conn, const Location& loc, const std::vector<LteCell>& cells) {
    if (!conn || cells.empty()) return;
    std::string m_query =
        "INSERT INTO measurements (timestamp, latitude, longitude, altitude, accuracy) VALUES ('" +
        loc.timestamp + "', " + std::to_string(loc.latitude) + ", " +
        std::to_string(loc.longitude) + ", " + std::to_string(loc.altitude) + ", " +
        std::to_string(loc.accuracy) + ") RETURNING id;";
    
    PGresult* res = PQexec(conn, m_query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cout << "Measurement insert failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res); return;
    }
    
    int meas_id = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    
    for (const auto& cell : cells) {
        std::string c_query =
            "INSERT INTO lte_cells (meas_id, pci, earfcn, band, rsrp, rsrq, rssi, asu, cid, tac, mcc, mnc) VALUES (" +
            std::to_string(meas_id) + ", " + std::to_string(cell.pci) + ", " +
            std::to_string(cell.earfcn) + ", " + cell.band + ", " +
            znachenie_sql(cell.rsrp) + ", " + znachenie_sql(cell.rsrq) + ", " +
            znachenie_sql(cell.rssi) + ", " + znachenie_sql(cell.asu) + ", " +
            std::to_string(cell.cid) + ", " + std::to_string(cell.tac) + ", " +
            (cell.mcc.empty() ? "NULL" : "'" + cell.mcc + "'") + ", " +
            (cell.mnc.empty() ? "NULL" : "'" + cell.mnc + "'") + ");";
            
        PGresult* c_res = PQexec(conn, c_query.c_str());
        if (PQresultStatus(c_res) != PGRES_COMMAND_OK)
            std::cout << "Cell insert failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(c_res);
    }
}