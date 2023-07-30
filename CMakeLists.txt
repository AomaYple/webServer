cmake_minimum_required(VERSION 3.27)

project(WebServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_executable(${PROJECT_NAME} "")

target_sources(${PROJECT_NAME} PRIVATE
        src/main.cpp

        src/base/BufferRing.cpp
        src/base/Completion.cpp
        src/base/Submission.cpp
        src/base/UserRing.cpp

        src/database/Database.cpp
        src/database/DatabaseCallError.cpp

        src/event/Event.cpp
        src/event/EventLoop.cpp
        src/event/Timer.cpp

        src/http/Http.cpp
        src/http/HttpParseError.cpp
        src/http/Response.cpp

        src/log/Message.cpp
        src/log/Log.cpp
        src/log/Node.cpp

        src/network/Client.cpp
        src/network/Server.cpp
        src/network/UserData.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        uring
        mariadb
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/WebServer)

add_custom_target(TARGET ${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/web ${CMAKE_CURRENT_BINARY_DIR}/WebServer/web
)
