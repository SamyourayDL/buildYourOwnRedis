//
// Created by dev on 8/5/25.
//

#include "client.h"
#include "server.h"

int main() {
    Client c;
    int rv = c.init(INADDR_LOOPBACK, 1234);
    switch (rv) {
        case INIT_STATUS::OK:
            std::cout << "Client initialized successfully" << std::endl;
            break;
        case INIT_STATUS::SOCKET_FAIL:
            die("socket() failed");
        case INIT_STATUS::CONN_FAIL:
            die("connect() failed");
        default:
            die("unknown init() error");
    }

    std::vector<Request> query_list;
    query_list.push_back( Request {
        7+12,
         request {"set", "a" , "kek"}
    });
    query_list.push_back( Request {
        4+8,
         request {"get", "a" }
    });
    query_list.push_back( Request {
        4+8,
         request {"get", "b" }
    });
    query_list.push_back( Request {
        4+8,
         request {"del", "a" }
    });
    query_list.push_back( Request {
        4+8,
         request {"del", "b" }
    });

    for (const auto& r : query_list) {
        int err = c.send_req(r);
        if (err < 0) {
            die("err in send_req");
        }
    }

    for (int i = 0; i < query_list.size(); ++i) {
        std::string resp;
        int32_t len = c.read_res(resp);
        if (len < 0) {
            if (len == -1 * RESP_CODE::RES_NX) {
                msg("key not exists");
                continue;
            }
            else {
                die("err in read_res");
            }
        }

        std::cout << "len: " << len << "data: " << resp << std::endl;
    }

    c.shutdown();
}