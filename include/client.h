//
// Created by dev on 8/5/25.
//

#ifndef REDIS_CLIENTN_H
#define REDIS_CLIENTN_H

#include <iostream>
#include <unistd.h> //write()
#include <sys/socket.h> //socket()
#include <netinet/in.h> //sockaddr
#include "utility.h"
#include <cassert>
#include <vector>
#include <string>

#include "server.h"

int32_t read_all(int connfd, uint8_t* buf, size_t n);
int32_t write_all(int connfd, const uint8_t* buf, size_t n);
void buf_append(std::vector<uint8_t>& buf, const uint8_t* data, size_t len);

using request = std::vector<std::string>;

struct Request {
    uint32_t len = 0;
    request data;
};

INIT_STATUS init();

class Client {
public:
    Client() = default;
    ~Client() = default;

    INIT_STATUS init(uint32_t host, uint16_t port);
    void shutdown();

    int32_t send_req(const Request& req);
    int32_t read_res(std::string& resp);    //returns len of response string
private:
   int fd = -1;
};

#endif //REDIS_CLIENTN_H