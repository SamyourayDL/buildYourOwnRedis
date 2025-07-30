//
// Created by dev on 7/25/25.
//
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

static void do_something(int connfd) {
    char buf[64];

    ssize_t n = read(connfd, buf, sizeof(buf)-1);
    if (n < 0) {
        std::cout << "failed to read";
        return;
    }

    printf("client says:%s\n", buf);

    char wbuf[] = "world";
    write(connfd, wbuf, sizeof(wbuf));
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cout << "failed to get fd";
        return 1;
    }

    int val = 1;
    int setSockOptErr = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (setSockOptErr < 0) {
        std::cout << "failed to set sockopt";
        return 1;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);

    int bindErr = bind(fd, (sockaddr*)&addr, sizeof(addr));
    if (bindErr < 0) {
        std::cout << "failed to bind";
        return 1;
    }

    int listenErr = listen(fd, SOMAXCONN);
    while(true) {
        sockaddr_in clientAddr = {};
        socklen_t addrlen = sizeof(clientAddr);
        int connfd = accept(fd, (sockaddr*)&clientAddr, &addrlen);
        if (connfd < 0) {
            continue;
        }

        do_something(connfd);

    }
}