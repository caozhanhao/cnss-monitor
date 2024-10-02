cmake_minimum_required(VERSION 3.28)
project(busin)

set(CMAKE_CXX_STANDARD 17)

include_directories(include thr)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(busin src/main.cpp
        src/email.cpp
        src/cnss.cpp)
target_link_libraries(busin Threads::Threads OpenSSL::SSL CURL::libcurl)