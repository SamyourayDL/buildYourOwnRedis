add_executable(tests
        tests.cpp
)

target_link_libraries(tests
        gtest_main
        server_lib
        client_lib
)

target_include_directories(tests PUBLIC
        ${INCLUDE_DIR}
        ${LIB_DIR}
)

include(GoogleTest)
gtest_discover_tests(tests)
