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

#include  "utility.h"


static void do_something(int connfd) {
    char buf[64];
    //sizeof(buf)-1 so the last buf[63] == \0 and it could be used as a cstring
    ssize_t n = read(connfd, buf, sizeof(buf)-1);   //blocking by default
    if (n < 0) {
        std::cout << "failed to read";
        return;
    }

    printf("client says:%s\n", buf);

    char wbuf[] = "world";
    write(connfd, wbuf, sizeof(wbuf));
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

        print_sockaddr(clientAddr);

        do_something(connfd);

    }
}