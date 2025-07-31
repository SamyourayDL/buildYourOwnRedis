//
// Created by dev on 7/30/25.
//

#include <iostream>
#include <unistd.h> //write()
#include <sys/socket.h> //socket()
#include <netinet/in.h> //sockaddr
#include "utility.h"
#include <cassert>

static int32_t read_all(int connfd, char* buf, size_t n) {
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

static int32_t write_all(int connfd, const char* buf, size_t n) {
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

static int32_t query(int fd, const char* text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        msg("too long request body");
        return -1;
    }

    char wbuf[4+k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(wbuf+4, text, len);

    if (int rv = write_all(fd, wbuf, 4+len)) {
        return rv;
    }

    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_all(fd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }
    // reply body
    err = read_all(fd, rbuf+4, len);
    if (err) {
        msg("read() error");
        return err;
    }
    // do something
    printf("server says: %.*s\n", len, &rbuf[4]);
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

    int err = query(fd, "hello1");
    if (err < 0) {
        die("err in query1");
    }

    err = query(fd, "hello2");
    if (err < 0) {
        die("err in query2");
    }

    close(fd);
}