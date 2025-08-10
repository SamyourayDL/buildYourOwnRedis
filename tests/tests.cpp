//
// Created by dev on 8/8/25.
//

#include <gtest/gtest.h>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <random>
#include "server.h"
#include "client.h"

class ServerTest : public testing::Test {
protected:
    Client client_;
    Server server_;
    uint32_t host = 0;
    uint16_t port = 1234;
    std::thread server_thread;

    void SetUp() override {
        auto sstatus = server_.init(host, port);
        ASSERT_EQ(sstatus, INIT_STATUS::OK);

        auto cstatus = client_.init(INADDR_LOOPBACK, port);
        ASSERT_EQ(cstatus, INIT_STATUS::OK);

        server_thread = std::thread([this](){
            server_.run();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        server_.shutdown();
        client_.shutdown();
        server_thread.join();
    }
};

auto const seed = 1234;
std::string random_string(int len);

TEST_F(ServerTest, BasicCommands) {
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
         request {"get", "b" }
    });
    query_list.push_back( Request {
        4+8,
         request {"del", "a" }
    });
    query_list.push_back( Request {
        4+8,
         request {"get", "a" }
    });
    query_list.push_back( Request {
        4+8,
         request {"del", "b" }
    });

    for (const auto& r : query_list) {
        int err = client_.send_req(r);
        ASSERT_GE(err, 0);
    }

    std::string resp;
    int32_t len = client_.read_res(resp);
    ASSERT_EQ(len, 0);
    ASSERT_EQ(resp, "");
    resp.clear();

    len = client_.read_res(resp);
    ASSERT_EQ(len, 3);
    ASSERT_EQ(resp, "kek");
    resp.clear();

    len = client_.read_res(resp);
    ASSERT_EQ(len, -1*RESP_CODE::RES_NX);
    ASSERT_EQ(resp, "");
    resp.clear();

    len = client_.read_res(resp);
    ASSERT_EQ(len, 0);
    ASSERT_EQ(resp, "");
    resp.clear();

    len = client_.read_res(resp);
    ASSERT_EQ(len, -1*RESP_CODE::RES_NX);
    ASSERT_EQ(resp, "");
    resp.clear();

    len = client_.read_res(resp);
    ASSERT_EQ(len, 0);
    ASSERT_EQ(resp, "");
    resp.clear();
}

TEST_F(ServerTest, LongestPossibleMessage) {
    std::string long_str(k_max_msg-12-3-1-4, 'a');

    Request set = {
        12 + 3 + (uint32_t)long_str.length() + 1,
        request {"set", long_str, "b"}
    };
    Request get {
        8+3+(uint32_t)long_str.length(),
         request {"get", long_str }
    };

    int rv = client_.send_req(set);
    ASSERT_GE(rv, 0);

    rv = client_.send_req(get);
    ASSERT_GE(rv, 0);

    std::string resp;
    int32_t len = client_.read_res(resp);
    ASSERT_EQ(len, 0);
    ASSERT_EQ(resp, "");
    resp.clear();

    len = client_.read_res(resp);
    ASSERT_EQ(len, 1);
    ASSERT_EQ(resp, "b");
    resp.clear();
}

TEST_F(ServerTest, TooLongMessage) {
    std::string long_str(k_max_msg-12-3-1-4 + 1, 'a');

    Request set = {
        12 + 3 + (uint32_t)long_str.length() + 1,
        request {"set", long_str, "b"}
    };

    int rv = client_.send_req(set);
    ASSERT_EQ(rv, -1);
}

TEST_F(ServerTest, RandomTests) {
    const int tests_num = 10000;
    std::unordered_set<std::string> keys;

    std::mt19937 urbg {seed};
    std::uniform_int_distribution<int> distr {1, k_max_msg/10};
    for (int i = 0; i < tests_num; ++i) {
        uint32_t len1 = distr(urbg);
        std::string random_key = random_string(len1);

        while (keys.contains(random_key)) {
            random_key = random_string(len1);
        }
        keys.insert(random_key);

        uint32_t len2 = distr(urbg);
        std::string random_value = random_string(len2);


        ASSERT_LE(12+3+len1+len2, k_max_msg-5);

        //get->set->get->del->get->set->set->get
        std::vector<Request> query_list;
        query_list.push_back( Request {
            8+3+len1,
             request {"get", random_key }
        });
        query_list.push_back( Request {
            12+3+len1+len2,
             request {"set", random_key , random_value}
        });
        query_list.push_back( Request {
            8+3+len1,
             request {"get", random_key }
        });
        query_list.push_back( Request {
            8+3+len1,
             request {"del", random_key }
        });
        query_list.push_back( Request {
            8+3+len1,
             request {"get", random_key }
        });
        query_list.push_back( Request {
            12+3+len1+len2,
             request {"set", random_key , random_value}
        });
        query_list.push_back( Request {
            12+3+len1,
             request {"set", random_key , ""}
        });
        query_list.push_back( Request {
            8+3+len1,
             request {"get", random_key }
        });

        for (const auto& r : query_list) {
            int err = client_.send_req(r);
            ASSERT_GE(err, 0);
        }

        std::string resp;   //get
        int32_t len = client_.read_res(resp);
        ASSERT_EQ(len, -1*RESP_CODE::RES_NX);
        ASSERT_EQ(resp, "");
        resp.clear();

        len = client_.read_res(resp);   //set
        ASSERT_EQ(len, 0);
        ASSERT_EQ(resp, "");
        resp.clear();

        len = client_.read_res(resp);   //get
        ASSERT_EQ(len, len2);
        ASSERT_EQ(resp, random_value);
        resp.clear();

        len = client_.read_res(resp);   //del
        ASSERT_EQ(len, 0);
        ASSERT_EQ(resp, "");
        resp.clear();

        len = client_.read_res(resp);   //get
        ASSERT_EQ(len, -1*RESP_CODE::RES_NX);
        ASSERT_EQ(resp, "");
        resp.clear();

        len = client_.read_res(resp);   //set
        ASSERT_EQ(len, 0);
        ASSERT_EQ(resp, "");
        resp.clear();

        len = client_.read_res(resp);   //set
        ASSERT_EQ(len, 0);
        ASSERT_EQ(resp, "");
        resp.clear();

        len = client_.read_res(resp);   //get
        ASSERT_EQ(len, 0);
        ASSERT_EQ(resp, "");
        resp.clear();
    }

}

TEST_F(ServerTest, MultipleConnections) {
    const int CLIENTS = 10;
    std::vector<std::thread> threads;

    for(int i = 0; i < CLIENTS; ++i) {
        threads.emplace_back([i, this]() {
            Client c_;
            auto cstatus = c_.init(INADDR_LOOPBACK, port);
            ASSERT_EQ(cstatus, INIT_STATUS::OK);

            std::string key = "key" + std::to_string(i);
            std::string val = "val" + std::to_string(i);

            std::vector<Request> query_list;
            query_list.push_back( Request {
                11+12,
            request {"set", key , val}
            });
            query_list.push_back( Request {
                7+8,
                request {"get", key }
            });

            for (const auto& r : query_list) {
                int err = client_.send_req(r);
                ASSERT_GE(err, 0);
            }

            std::string resp;
            int32_t len = client_.read_res(resp);
            ASSERT_EQ(len, 0);
            ASSERT_EQ(resp, "");
            resp.clear();

            len = client_.read_res(resp);
            ASSERT_EQ(len, 4);
            ASSERT_EQ(resp, val);
            resp.clear();
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

std::string random_string(int len) {
    static const char alphabet[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::string result;

    result.reserve(len);

    auto const hes = std::random_device{}();
    std::mt19937 urbg {hes};
    std::uniform_int_distribution<int> distr {0, sizeof(alphabet)-1};
    for (int i = 0; i < len; ++i) {
        result += alphabet[distr(urbg)];
    }

    return result;
}