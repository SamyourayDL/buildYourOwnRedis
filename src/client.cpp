//
// Created by dev on 7/30/25.
//

#include <iostream>
#include <unistd.h> //write()
#include <sys/socket.h> //socket()
#include <netinet/in.h> //sockaddr
#include "utility.h"

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

    sockaddr_in clientAddr = {};
    socklen_t addrlen = sizeof(clientAddr);
    rv = getsockname(fd, (sockaddr*)&clientAddr, &addrlen);
    if (rv < 0) {
        die("getpeername");
    }
    print_sockaddr(clientAddr);

    char wbuf[] = "Hello"; // sizeof(wbuf) == 6 "Hello\0"
    std::cout << sizeof(wbuf);
    write(fd, wbuf, sizeof(wbuf));

    char rbuf[64] = {};
    ssize_t n = read(fd, rbuf, sizeof(rbuf)-1); //blocking by default, so we will 100% get the message
    if (n < 0) {
        die("read");
    }

    printf("server says:%s\n", rbuf);
}