cmake_minimum_required(VERSION 3.26.3)

project(WebServer)

set(CMAKE_CXX_STANDARD 23)

add_executable(${PROJECT_NAME} "")

target_sources(${PROJECT_NAME}
        PRIVATE
            main.cpp
            src/Buffer.cpp
            src/Client.cpp
            src/DataBase.cpp
            src/Epoll.cpp
            src/EventLoop.cpp
            src/Http.cpp
            src/Log.cpp
            src/Server.cpp
            src/Pool.cpp
            src/Timer.cpp
        )

target_include_directories(${PROJECT_NAME}
        PRIVATE
            src
        )

target_link_libraries(${PROJECT_NAME}
        PRIVATE
            mariadb
        )
