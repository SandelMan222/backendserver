#include "zmq_server.h"
#include <zmq.hpp>
#include <iostream>
#include <fstream>
#include "json_loader.h"
#include "db.h"

void zapustit_server(DeviceData* data) {
    PGconn* conn = podklyuchit_bd();
    if (conn) std::cout << "Connected to PostgreSQL" << std::endl;
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://0.0.0.0:5555");
    std::cout << "Server started on port 5555" << std::endl;
    
    while (server_running) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);
        std::string msg(static_cast<char*>(request.data()), request.size());
        std::cout << "Received: " << msg << std::endl;
        try {
            auto j = nlohmann::json::parse(msg);
            std::ifstream infile("locations.json");
            nlohmann::json arr = nlohmann::json::array();
            if (infile.good()) infile >> arr;
            arr.push_back(j);
            std::ofstream outfile("locations.json");
            outfile << arr.dump(4);
            
            std::lock_guard<std::mutex> lock(data_mutex);
            obrabotat_json(j, data, conn);
        } catch (const std::exception& e) {
            std::cout << "Invalid JSON: " << e.what() << std::endl;
        }
        socket.send(zmq::buffer(std::string("OK")), zmq::send_flags::none);
    }
    if (conn) PQfinish(conn);
}