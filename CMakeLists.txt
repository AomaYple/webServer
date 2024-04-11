cmake_minimum_required (VERSION 3.29.1)

project (webServer)

add_executable (${PROJECT_NAME})

set_target_properties (${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        COMPILE_WARNING_AS_ERROR ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
)

target_sources (${PROJECT_NAME} PRIVATE
        src/coroutine/Awaiter.cpp
        src/coroutine/Scheduler.cpp
        src/coroutine/Task.cpp

        src/fileDescriptor/Client.cpp
        src/fileDescriptor/FileDescriptor.cpp
        src/fileDescriptor/Server.cpp
        src/fileDescriptor/Timer.cpp

        src/http/Database.cpp
        src/http/HttpParse.cpp
        src/http/HttpRequest.cpp
        src/http/HttpResponse.cpp

        src/json/JsonArray.cpp
        src/json/JsonObject.cpp
        src/json/JsonValue.cpp

        src/log/Exception.cpp
        src/log/Log.cpp
        src/log/logger.cpp
        src/log/LogQueue.cpp

        src/ring/Completion.cpp
        src/ring/Outcome.cpp
        src/ring/Ring.cpp
        src/ring/RingBuffer.cpp
        src/ring/Submission.cpp

        src/main.cpp
)

target_compile_options (${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Debug>:-Og -fsanitize=address>
        $<$<CONFIG:Release>:-Ofast>
)

target_link_options (${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
)

target_link_libraries (${PROJECT_NAME} PRIVATE
        uring
        brotlienc
        mariadb
)

add_custom_command (TARGET ${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources/images ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/resources/images
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources/videos ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/resources/videos
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources/web ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/resources/web
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources
        VERBATIM
)
