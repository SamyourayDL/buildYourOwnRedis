//
// Created by dev on 8/5/25.
//

#include "server.h"

int main() {
    Server s;
    int rv = s.init(0, 1234);
    switch (rv) {
        case INIT_STATUS::OK:
            std::cout << "Server initialised successfully!" << std::endl;
            break;
        case INIT_STATUS::SOCKET_FAIL:
            die("socket() failed");
        case INIT_STATUS::SOCKOPT_FAIL:
            die("sockopt() failed");
        case INIT_STATUS::BIND_FAIL:
            die("bind() failed");
        case INIT_STATUS::LISTEN_FAIL:
            die("listen() failed");
        default:
            die("unknown init() error");
    }
    s.run();
}