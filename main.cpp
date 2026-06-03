#include <iostream>
#include <thread>
#include <curl/curl.h>

#include "types.h"
#include "db.h"
#include "json_loader.h"
#include "zmq_server.h"
#include "osm_map.h"
#include "gui.h"

// Инициализация глобальных переменных
std::mutex data_mutex;
std::atomic<bool> server_running{true};
std::map<int, PciHistory> gPciHistory;
float gTimeCounter = 0.0f;

int main() {
    init_stepeni2();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    static DeviceData deviceData;
    zagruzit_fayl_json(&deviceData);

    std::thread server_thread(zapustit_server, &deviceData);
    std::thread gui_thread(zapustit_gui, &deviceData);

    gui_thread.join();
    server_thread.join();

    curl_global_cleanup();
    return 0;
}