cmake_minimum_required(VERSION 3.30.0)

project(webServer)

add_executable(${PROJECT_NAME})

set(BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        COMPILE_WARNING_AS_ERROR ON
        RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR}
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
target_sources(${PROJECT_NAME} PRIVATE
        ${SOURCES}
)

target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Debug>:-Og -fsanitize=address>
        $<$<CONFIG:Release>:-Ofast>
)

target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        uring
        brotlienc
        mariadb
)

add_custom_command(TARGET ${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${BINARY_DIR}/resources

        VERBATIM
)
