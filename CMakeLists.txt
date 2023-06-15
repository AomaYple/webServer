cmake_minimum_required(VERSION 3.22.1)

project(WebServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_executable(${PROJECT_NAME} "")

target_sources(${PROJECT_NAME} PRIVATE
        main.cpp
        src/Buffer.cpp
        src/Client.cpp
        src/DataBase.cpp
        src/Epoll.cpp
        src/EventLoop.cpp
        src/Http.cpp
        src/Log.cpp
        src/Pool.cpp
        src/Server.cpp
        src/Timer.cpp
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        include
        )

target_link_libraries(${PROJECT_NAME} PRIVATE
        mysqlclient
        )

