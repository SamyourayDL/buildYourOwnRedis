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

#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <map>

static void fd_set_nb(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl F_GETFL error");
    }

    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl F_SETFL error");
    }
}

static bool read_u32(const uint8_t*& cur, const uint8_t* end, uint32_t& out) {  //const uint8_t*& cur - ptr on const by reference
    if (cur + 4 > end) {
        return false;
    }

    memcpy(&out, cur, 4);
    cur += 4;
    return true;
}

static bool read_str(const uint8_t*& cur, const uint8_t* end, uint32_t size, std::string& out) {
    if (cur + size > end) {
        return false;
    }

    out.assign(cur, cur+size);
    cur += size;
    return true;
}

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

static Conn* handle_accept(int fd) {
    sockaddr_in clientAddr = {};
    socklen_t addrlen = sizeof(clientAddr);

    int connfd = accept(fd, (sockaddr*)&clientAddr, &addrlen);

    if (connfd < 0) {
        NULL;
    }

    fd_set_nb(connfd); //!!!!!!!!!!!!!!!!! set non-blocking read and write

    Conn* conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;

    return conn;
}




// +----------+------+-----+------+-----+------+-----+-----+------+
// | req_size | nstr | len | str1 | len | str2 | ... | len | strn |
// +----------+------+-----+------+-----+------+-----+-----+------+

static uint32_t parse_req(const uint8_t* data, uint32_t size, std::vector<std::string>& cmd) {
    const uint8_t* end = data + size;

    uint32_t nstr = 0;
    if (!read_u32(data, end, nstr)) {
        return -1;
    }
    if (nstr > k_max_args) {
        return -1;
    }

    for (uint32_t i = 0; i < nstr; ++i) {
        uint32_t len = 0;
        if (!read_u32(data, end, len)) {
            return -1;
        }
        cmd.emplace_back();
        if (!read_str(data, end, len, cmd.back())) {
            return -1;
        }
    }

    if (data != end) {
        return -1;
    }

    return 0;
}

enum RESP_CODE {
    RES_OK,
    RES_ERR,    //error
    RES_NX,     //key not exists in storage
};

struct Response {
    uint32_t status = RESP_CODE::RES_OK;
    std::vector<uint8_t> data;
};


static std::map<std::string, std::string> g_data;

static void do_request(const std::vector<std::string>& cmd, Response& resp) {
    if (cmd.size() == 2 && cmd[0] == "get") {
        auto it = g_data.find(cmd[1]);
        if (it == g_data.end()) {
            resp.status = RESP_CODE::RES_NX;
            return;
        }
        const std::string& val = it->second;
        resp.data.assign(val.cbegin(), val.cend());
    }
    else if (cmd.size() == 3 && cmd[0] == "set") {
        g_data[cmd[1]] = cmd[2];
    }
    else if (cmd.size() == 2 && cmd[0] == "del") {
        g_data.erase(cmd[1]);
    }
    else {
        resp.status = RESP_CODE::RES_ERR;
    }
}

static void make_response(const Response& resp, std::vector<uint8_t>& out) {
    uint32_t len = 4 + resp.data.size();
    out.insert(out.end(), (const uint8_t*)&len, (const uint8_t*)&len+4);
    out.insert(out.end(), (const uint8_t*)&resp.status, (const uint8_t*)&resp.status+4);
    out.insert(out.end(), resp.data.data(), resp.data.data() + len-4);
}

static bool try_one_request(Conn* conn) {
    if (conn->incoming.size() < 4) {
        return false;
    }

    uint32_t len = 0;   //request_size
    memcpy(&len, conn->incoming.data(), 4);
    if (len > k_max_msg) {
        conn->want_close = true;
        return false;
    }

    if (conn->incoming.size()-4 < len) {
        return false;
    }

    const uint8_t* request = conn->incoming.data()+4;

    //parse_req
     std::vector<std::string> cmd;
     if (parse_req(request, len ,cmd) < 0) {
         conn->want_close = true;
         return false;
     }
    Response resp;
    do_request(cmd, resp);
    make_response(resp, conn->outgoing);

    //remove processed message from buffer
    conn->incoming.erase(conn->incoming.begin(), conn->incoming.begin()+4+len); //clear this request data

    return true;
}

static void handle_write(Conn* conn);

static void handle_read(Conn* conn) {
    uint8_t buf[64 * 1024];
    int rv = read(conn->fd, buf, sizeof(buf));
    if (rv <= 0) {  // rv == 0 if EOF
        conn->want_close = true;
        return;
    }

    //add data too incoming buffer
    conn->incoming.insert(conn->incoming.end(), buf, buf+rv);   //pointers are also iterators

    //3.Try to parse the incoming buffer
    //4.Handle data
    //5.Remove the message from incoming
    while (try_one_request(conn)) {}    //pipelined requests

    if (conn->outgoing.size() > 0) {
        conn->want_read = false;
        conn->want_write = true;

        return handle_write(conn);  //optimstic handle write
    }
}

static void handle_write(Conn* conn) {
    assert(conn->outgoing.size() > 0);

    ssize_t rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    if (rv < 0 && errno == EAGAIN) {
        // if we write and client not ready to read we try to write in overfilled buff
        return; // actually not ready
    }
    if (rv < 0) {
        conn->want_close = true;
        return;
    }

    conn->outgoing.erase(conn->outgoing.begin(), conn->outgoing.begin()+rv);

    if (conn->outgoing.size() == 0) {
        conn->want_read = true;
        conn->want_write = false;
    }
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

    std::vector<Conn*> fd2conn; //fd starts from 0-..., so mapping just by index

    std::vector<pollfd> poll_args;

    fd_set_nb(fd);  //non-blocking accept
    while(true) {
        poll_args.clear();
        pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);

        for (auto conn : fd2conn) {
            if (!conn) {
                continue;
            }

            pollfd pfd = {conn->fd, POLLERR, 0};

            if (conn->want_read) {
                pfd.events |= POLLIN;
            }
            if (conn->want_write) {
                pfd.events |= POLLOUT;
            }

            poll_args.push_back(pfd);
        }

        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        if (rv < 0 && errno == EINTR) {
            continue;   //not an error
        }
        if (rv < 0) {
            die("poll");
        }

        if (poll_args[0].revents) {
            if (Conn* conn = handle_accept(fd)) {
                if (fd2conn.size() <= (size_t)conn->fd) {
                    fd2conn.resize(conn->fd+1);
                }
                fd2conn[conn->fd] = conn;
            }
        }

        for (size_t i = 1; i < poll_args.size(); ++i) {
            uint32_t ready = poll_args[i].revents;
            Conn* conn = fd2conn[poll_args[i].fd];

            if (ready & POLLIN) {
                handle_read(conn);
            }
            if (ready & POLLOUT) {
                handle_write(conn);
            }
            if ((ready & POLLERR) || conn->want_close) {
                close(conn->fd);
                fd2conn[conn->fd] = NULL;
                delete conn;
            }
        }
    }
}