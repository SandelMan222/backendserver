#include "server.h"
#include "baza.h"
#include "obrabotka.h"
#include "globalnye.h"
#include <iostream>
#include <fstream>
#include <zmq.hpp>
#include <nlohmann/json.hpp>

void zapustitServer(DannyeUstrojstva* dannye) {
    PGconn* soedinenie = podklyuchitBazu();
    if (soedinenie) std::cout << "BD podklyuchena" << std::endl;

    zmq::context_t kontekst(1);
    zmq::socket_t soket(kontekst, zmq::socket_type::rep);
    soket.bind("tcp://0.0.0.0:5555");
    std::cout << "Server zapushchen na portu 5555" << std::endl;

    while (server_rabotaet) {
        zmq::message_t zapros;
        soket.recv(zapros, zmq::recv_flags::none);
        std::string soobshchenie(static_cast<char*>(zapros.data()), zapros.size());
        std::cout << "Polucheno: " << soobshchenie << std::endl;

        try {
            auto j = nlohmann::json::parse(soobshchenie);

            std::ifstream vhod("locations.json");
            nlohmann::json massiv = nlohmann::json::array();
            if (vhod.good()) vhod >> massiv;
            massiv.push_back(j);
            std::ofstream vyhod("locations.json");
            vyhod << massiv.dump(4);

            std::lock_guard<std::mutex> zamok(mutex_dannyh);
            obrabotatJson(j, dannye, soedinenie);

        } catch (const std::exception& e) {
            std::cout << "Nekorrektnyj JSON: " << e.what() << std::endl;
        }

        soket.send(zmq::buffer(std::string("OK")), zmq::send_flags::none);
    }

    if (soedinenie) PQfinish(soedinenie);
}
