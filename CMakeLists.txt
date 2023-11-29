cmake_minimum_required(VERSION 3.27.9)

project(webServer)

add_executable(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        COMPILE_WARNING_AS_ERROR ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
)

target_sources(${PROJECT_NAME} PRIVATE
        src/coroutine/Awaiter.cpp
        src/coroutine/Generator.cpp
        src/coroutine/Result.cpp

        src/database/Database.cpp

        src/http/Http.cpp
        src/http/HttpRequest.cpp
        src/http/HttpResponse.cpp

        src/log/Exception.cpp
        src/log/Log.cpp
        src/log/Logger.cpp
        src/log/Node.cpp

        src/scheduler/Scheduler.cpp

        src/socket/Client.cpp
        src/socket/Server.cpp
        src/socket/Timer.cpp

        src/userRing/BufferRing.cpp
        src/userRing/Completion.cpp
        src/userRing/Event.cpp
        src/userRing/Submission.cpp
        src/userRing/UserRing.cpp

        src/main.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Debug>:-Og -fsanitize=address,undefined,leak>
        $<$<CONFIG:Release>:-Ofast>
)

target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        uring
        brotlienc
        mariadb
)

add_custom_command(TARGET ${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/resources
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources
        VERBATIM
)
