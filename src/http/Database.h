#pragma once

#include <mysql/mysql.h>

#include <cstdint>
#include <source_location>
#include <string_view>
#include <vector>

class Database {
public:
    Database(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
             std::uint_fast32_t port, std::string_view unixSocket, std::uint_fast64_t clientFlag);

    Database(const Database &) = delete;

    auto consult(std::string_view statement) -> std::vector<std::vector<std::string>>;

    ~Database();

private:
    [[nodiscard]] static auto initialize(std::source_location sourceLocation = std::source_location::current())
            -> MYSQL *;

    auto connect(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
                 std::uint_fast32_t port, std::string_view unixSocket, std::uint_fast64_t clientFlag,
                 std::source_location sourceLocation = std::source_location::current()) -> void;

    auto query(std::string_view statement, std::source_location sourceLocation = std::source_location::current())
            -> void;

    [[nodiscard]] auto storeResult(std::source_location sourceLocation = std::source_location::current())
            -> MYSQL_RES *;

    [[nodiscard]] static auto getColumnCount(MYSQL_RES *result) noexcept -> std::uint_fast32_t;

    [[nodiscard]] static auto getRow(MYSQL_RES *result) noexcept -> MYSQL_ROW;

    static auto freeResult(MYSQL_RES *result) noexcept -> void;

    MYSQL *connection;
};
