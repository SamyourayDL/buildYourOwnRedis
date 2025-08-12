//
// Created by dev on 8/5/25.
//

#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h> //sockaddr
#include <poll.h>
#include <fcntl.h>
#include <cassert>

#include <atomic>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include  "utility.h"

bool read_u32(const uint8_t*& cur, const uint8_t* end, uint32_t& out);
bool read_str(const uint8_t*& cur, const uint8_t* end, uint32_t size, std::string& out);
void fd_set_nb(int fd);

enum INIT_STATUS {
    OK,
    SOCKET_FAIL,
    SOCKOPT_FAIL,
    BIND_FAIL,
    LISTEN_FAIL,
    CONN_FAIL
};

enum RESP_CODE {
    RES_OK,
    RES_ERR,    //error
    RES_NX,     //key not exists in storage
};

const int POLL_TIMEOUT = -1;

class Server {
public:
    Server() = default;
    ~Server() = default;

    INIT_STATUS init(uint32_t host, uint16_t port);
    void run();
    void shutdown();
private:
    struct Conn {
        int fd = -1;
        //application intentions for the event loop
        bool want_read = false;
        bool want_write = false;
        bool want_close = false;
        //buffers
        std::vector<uint8_t> incoming;
        std::vector<uint8_t> outgoing;
    };

    struct Response {
        uint32_t status = RESP_CODE::RES_OK;
        std::vector<uint8_t> data;
    };

    Conn* handle_accept();

    static uint32_t parse_req(const uint8_t* data, uint32_t size, std::vector<std::string>& cmd);
    void do_request(const std::vector<std::string>& cmd, Response& resp);
    static void make_response(const Response& resp, std::vector<uint8_t>& out);
    bool try_one_request(Conn* conn);

    void handle_write(Conn* conn) const;
    void handle_read(Conn* conn);

    int fd = -1;
    std::map<std::string, std::string> g_data;
    std::atomic<bool> running = true;
};

#endif //REDIS_SERVER_H