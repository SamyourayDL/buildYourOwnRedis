//
// Created by dev on 7/30/25.
//

#include <iostream>
#include <unistd.h> //write()
#include <sys/socket.h> //socket()
#include <netinet/in.h> //sockaddr
#include "utility.h"
#include <cassert>
#include <vector>
#include <string>

static int32_t read_all(int connfd, uint8_t* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(connfd, buf, n);

        if (rv <= 0) {  //if rv == 0 then we have met EOF, impossible in our protocol
            return -1;
        }
        assert((size_t)rv <= n);

        n -= (size_t)rv;
        buf += rv;
    }

    return 0;
}

static int32_t write_all(int connfd, const uint8_t* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(connfd, buf, n);

        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);

        n -=rv;
        buf += rv;
    }

    return 0;
}

static void buf_append(std::vector<uint8_t>& buf, const uint8_t* data, size_t len) {
    buf.insert(buf.end(), data, data+len);
}

using request = std::vector<std::string>;

struct Request {
    uint32_t len = 0;
    request data;
};

enum RESP_CODE {
    RES_OK,
    RES_ERR,    //error
    RES_NX,     //key not exists in storage
};

static int32_t send_req(int fd, const Request& req) {
    uint32_t req_size = 4 + req.len;
    if (req_size > k_max_msg) {
        msg("too long msg");
        return -1;
    }
    uint32_t nstr = req.data.size();
    if (nstr > k_max_args) {
        msg("too many arguments");
        return -1;
    }

    std::vector<uint8_t> wbuf;
    buf_append(wbuf, (uint8_t*)&req_size, 4);
    buf_append(wbuf, (uint8_t*)&nstr, 4);
    for (uint32_t i = 0; i < nstr; ++i) {
        uint32_t len = req.data[i].size();
        buf_append(wbuf, (uint8_t*)&len, 4);
        buf_append(wbuf, (uint8_t*)req.data[i].data(), len);
    }

    if (int rv = write_all(fd, wbuf.data(), wbuf.size())) {
        return rv;
    }

    return 0;
}

static int32_t read_res(int fd) {
    std::vector<uint8_t> rbuf;
    rbuf.resize(4);
    errno = 0;
    //read len
    int32_t err = read_all(fd, rbuf.data(), 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf.data(), 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long response");
        return -1;
    }

    rbuf.resize(4+len);
    //read status
    errno = 0;
    err = read_all(fd, rbuf.data()+4, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint32_t status = 0;
    memcpy(&status, rbuf.data()+4, 4);
    if (status != RESP_CODE::RES_OK) {
        if (status == RESP_CODE::RES_NX) {
            msg("key not exists");
            return -1;
        }

        msg("status not ok");
        return -1;
    }

    // reply body
    err = read_all(fd, rbuf.data()+8, len-4);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    printf("len:%u data:%.*s\n", len-4, len-4, &rbuf[8]);
    return 0;
}


int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket");
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(1234);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   //127.0.0.1

    int rv = connect(fd, (sockaddr*)&address, sizeof(address));
    if (rv < 0) {
        die("connect");
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
         request {"del", "a" }
    });
    query_list.push_back( Request {
        4+8,
         request {"del", "b" }
    });
    query_list.push_back( Request {
        4+8,
         request {"get", "a" }
    });

    for (const auto& r : query_list) {
        int err = send_req(fd, r);
        if (err < 0) {
            die("err in send_req");
        }
    }

    for (int i = 0; i < query_list.size(); ++i) {
        int err = read_res(fd);
        if (err < 0) {
            die("err in read_res");
        }
    }

    close(fd);
}