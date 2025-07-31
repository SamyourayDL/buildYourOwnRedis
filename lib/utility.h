//
// Created by dev on 7/30/25.
//

#ifndef REDIS_UTILITY_H
#define REDIS_UTILITY_H

#include <arpa/inet.h>
#include <iostream>
#include <cerrno>   //errno global var, error_code of the last system call
#include <cstring>  //std::strerror

const size_t k_max_msg = 4096;

void inline die(const std::string& msg) {
    std::cerr << msg << ": " << std::strerror(errno);
    std::exit(1);
}

void inline print_sockaddr(const sockaddr_in& addr) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
    std::cout << "IP: " << ip_str
              << ", Port: " << ntohs(addr.sin_port) << "\n";
}

void inline msg(const char* s) {
    std::cout << s << std::endl;
}

#endif //REDIS_UTILITY_H