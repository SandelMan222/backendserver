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

std::vector<TochkaIzmereniya> poluchit_dannye_iz_bd(int earfcn, const std::string& kriteriy) {
    std::vector<TochkaIzmereniya> rezultat;
    
    PGconn* conn = podklyuchit_bd();
    if (!conn) {
        std::cerr << "Oshibka: ne udalos podklyuchitsya k BD" << std::endl;
        return rezultat;
    }
    
    std::string selectColumn = kriteriy;
    std::string whereClause = "lc." + kriteriy + " IS NOT NULL";
    if (kriteriy == "altitude") {
        selectColumn = "m.altitude";
        whereClause = "m.altitude IS NOT NULL";
    }

    std::string zapros = "SELECT m.latitude, m.longitude, m.altitude, " + selectColumn +
                         " FROM lte_cells lc LEFT JOIN measurements m ON m.id = lc.meas_id " +
                         "WHERE lc.earfcn = " + std::to_string(earfcn) + 
                         " AND " + whereClause + " AND m.latitude IS NOT NULL;";
    
    std::cout << "SQL Zapros: " << zapros << std::endl;
    
    PGresult* res = PQexec(conn, zapros.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Query failed: " << PQerrorMessage(conn) << std::endl;
        
        std::string zapros2 = "SELECT COUNT(*) FROM lte_cells WHERE earfcn = " + std::to_string(earfcn) + ";";
        PGresult* res2 = PQexec(conn, zapros2.c_str());
        int count = std::stoi(PQgetvalue(res2, 0, 0));
        std::cout << "Vsego zapisey dlya EARFCN=" << earfcn << ": " << count << std::endl;
        PQclear(res2);
        
        std::string tableInfo = "SELECT column_name FROM information_schema.columns WHERE table_name = 'lte_cells';";
        PGresult* res3 = PQexec(conn, tableInfo.c_str());
        std::cout << "Stolbtsy v lte_cells:" << std::endl;
        for (int i = 0; i < PQntuples(res3); i++) {
            std::cout << "  - " << PQgetvalue(res3, i, 0) << std::endl;
        }
        PQclear(res3);
        
        PQclear(res);
        PQfinish(conn);
        return rezultat;
    }
    
    int numRows = PQntuples(res);
    std::cout << "Zagruzheno " << numRows << " tochek dlya EARFCN=" << earfcn << std::endl;
    
    for (int i = 0; i < numRows; ++i) {
        try {
            TochkaIzmereniya tochka;
            tochka.shirota = std::stod(PQgetvalue(res, i, 0));
            tochka.dolgota = std::stod(PQgetvalue(res, i, 1));
            tochka.vysota = std::stod(PQgetvalue(res, i, 2));
            tochka.znachenie = std::stod(PQgetvalue(res, i, 3));
            rezultat.push_back(tochka);
        } catch (const std::exception& e) {
            std::cerr << "Oshibka pri parsingu dannykh: " << e.what() << std::endl;
        }
    }
    
    PQclear(res);
    PQfinish(conn);
    return rezultat;
}