cmake_minimum_required(VERSION 3.26.3)

project(WebServer)

set(CMAKE_CXX_STANDARD 23)

add_executable(${PROJECT_NAME} "")

target_sources(${PROJECT_NAME}
        PRIVATE
            main.cpp
        )

target_include_directories(${PROJECT_NAME}
        PRIVATE
            src
            log
        )
