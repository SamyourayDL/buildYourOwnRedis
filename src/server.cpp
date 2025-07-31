//
// Created by dev on 7/30/25.
//
//
// Created by dev on 7/25/25.
//
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h> //sockaddr
#include <string>
#include <cassert>

#include  "utility.h"

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

        n -= rv;
        buf += rv;
    }

    return 0;
}

static int32_t one_request(int connfd) {
    char buf[4+k_max_msg];

    errno = 0;
    int rv = read_all(connfd, buf, 4);
    if (rv < 0) {
        msg(errno == 0 ? "EOF" : "read() error");
        return rv;
    }

    uint32_t len = 0;
    memcpy(&len, buf, 4);
    if (len > k_max_msg) {
        msg("msg too long");
        return -1;
    }

    rv = read_all(connfd, buf+4, len);
    if (rv < 0) {
        msg("read() error");
        return -1;
    }

    printf("client says:%.*s\n", len, buf+4);

    const char reply[] = "world";
    char wbuf[4+sizeof(reply)];

    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);

    memcpy(wbuf+4, reply, len);

    rv = write_all(connfd, wbuf, 4+len);
    if (rv < 0) {
        return -1;
    }

    return rv;
}


int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);   //AF_INET/AF_INET6 SOCK_STREAM - TCP/SOCK_DGRAM - UDP
    if (fd < 0) {
        die("socket");
    }

    int val = 1;
    int rv = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));   //in-depth TCP param
    if (rv < 0) {
        die("setsockopt");
    }

    sockaddr_in addr = {};  //holds IPv4 address and port
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);    //port and address in big-endian "Host to Network short"
    addr.sin_addr.s_addr = htonl(0);

    rv = bind(fd, (sockaddr*)&addr, sizeof(addr));
    if (rv < 0) {
        die("bind");
    }

    rv = listen(fd, SOMAXCONN);
    if (rv < 0) {
        die("listen");
    }
    while(true) {
        sockaddr_in clientAddr = {};
        socklen_t addrlen = sizeof(clientAddr);

        int connfd = accept(fd, (sockaddr*)&clientAddr, &addrlen);

        if (connfd < 0) {
            continue;
        }

        while (true) {
            int err = one_request(connfd);
            if (err < 0) {
                break;
            }
        }
    }
}