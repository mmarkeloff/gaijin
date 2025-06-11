cmake_minimum_required(VERSION 2.6)

project(cache_test_project)

find_package(GTest REQUIRED)
find_package(Threads)

enable_testing()
include_directories("../" "../third_party/" "../third_party/json/include")

file(GLOB_RECURSE SRC_FILES "../common/*.cpp")

add_executable(cache_test "test.cpp" "../cache-clnt/cache_clnt.cpp" "../cache-service/async_tcp_srv.cpp" ${SRC_FILES})

set_target_properties(cache_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(cache_test GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cache_test)
