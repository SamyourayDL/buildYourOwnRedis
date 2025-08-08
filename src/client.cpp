//
// Created by dev on 8/5/25.
//
#include "client.h"

int32_t read_all(int connfd, uint8_t* buf, size_t n) {
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

int32_t write_all(int connfd, const uint8_t* buf, size_t n) {
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

void buf_append(std::vector<uint8_t>& buf, const uint8_t* data, size_t len) {
    buf.insert(buf.end(), data, data+len);
}


INIT_STATUS Client::init(uint32_t host, uint16_t port) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return INIT_STATUS::SOCKET_FAIL;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(host);   //INADDR_LOOPBACK - 127.0.0.1

    int rv = connect(fd, (sockaddr*)&address, sizeof(address));
    if (rv < 0) {
        return INIT_STATUS::CONN_FAIL;
    }

    return INIT_STATUS::OK;
}

void Client::shutdown() {
    if (!(fd < 0)) {
        close(fd);
    }
}

int32_t Client::send_req(const Request& req) {
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

int32_t Client::read_res(std::string& resp) {
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
            // msg("key not exists");
            return -1 * RESP_CODE::RES_NX;  //special error
        }

        msg("status not OK");
        return -1;
    }

    // reply body
    err = read_all(fd, rbuf.data()+8, len-4);   //syscall made even if len-4 = 0
    if (err) {
        msg("read() error");
        return err;
    }

    resp.assign((const char*)rbuf.data()+8, len-4);
    // do something
    // printf("len:%u data:%.*s\n", len-4, len-4, &rbuf[8]);
    return (int32_t)len-4;
}