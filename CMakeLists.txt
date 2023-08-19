cmake_minimum_required(VERSION 3.22)

project(WebServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_executable(${PROJECT_NAME} "")

target_sources(${PROJECT_NAME} PRIVATE
        src/base/BufferRing.cpp
        src/base/Completion.cpp
        src/base/Submission.cpp
        src/base/UserData.cpp
        src/base/UserRing.cpp

        src/event/Event.cpp
        src/event/EventLoop.cpp
        src/event/Timer.cpp

        src/exception/Exception.cpp

        src/http/Database.cpp
        src/http/Http.cpp
        src/http/HttpRequest.cpp
        src/http/HttpResponse.cpp

        src/log/Log.cpp
        src/log/message.cpp
        src/log/Node.cpp

        src/socket/Client.cpp
        src/socket/Server.cpp

        src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        uring
        brotlienc
        mariadb
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/WebServer)

add_custom_target(TARGET ${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/web ${CMAKE_CURRENT_BINARY_DIR}/WebServer/web
)
