cmake_minimum_required(VERSION 2.6)

project(cache_client_project)

find_package(Threads)

include_directories("../" "../third_party/" "../third_party/json/include")

file(GLOB SRC_FILES "../common/*.cpp" "*.cpp")

add_executable(cache_client ${SRC_FILES})

set_target_properties(cache_client PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
