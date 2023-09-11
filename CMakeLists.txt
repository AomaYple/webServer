cmake_minimum_required(VERSION 3.27.4)

project(webServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})

add_executable(${PROJECT_NAME}
        src/coroutine/Awaiter.cpp
        src/coroutine/Task.cpp

        src/database/Database.cpp

        src/exception/Exception.cpp

        src/http/Http.cpp
        src/http/HttpRequest.cpp
        src/http/HttpResponse.cpp

        src/log/Log.cpp

        src/network/Client.cpp
        src/network/Server.cpp
        src/network/Timer.cpp

        src/scheduler/Scheduler.cpp

        src/userRing/BufferRing.cpp
        src/userRing/Completion.cpp
        src/userRing/Submission.cpp
        src/userRing/UserData.cpp
        src/userRing/UserRing.cpp

        src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        uring
        brotlienc
        mariadb
)

add_custom_target(TARGET ${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/web ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/web
)
