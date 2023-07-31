#pragma once

#include <source_location>
#include <string_view>
#include <vector>

#include <mysql/mysql.h>

class Database {
public:
    Database(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
             unsigned int port, std::string_view unixSocket, unsigned long clientFlag);

    Database(const Database &) = delete;

    auto consult(std::string_view statement) -> std::vector<std::vector<std::string>>;

    ~Database();

private:
    [[nodiscard]] static auto initialize(std::source_location sourceLocation = std::source_location::current())
            -> MYSQL *;

    auto connect(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
                 unsigned int port, std::string_view unixSocket, unsigned long clientFlag,
                 std::source_location sourceLocation = std::source_location::current()) -> void;

    auto query(std::string_view statement, std::source_location sourceLocation = std::source_location::current())
            -> void;

    [[nodiscard]] auto storeResult(std::source_location sourceLocation = std::source_location::current())
            -> MYSQL_RES *;

    [[nodiscard]] static auto getColumnCount(MYSQL_RES *result) -> unsigned int;

    [[nodiscard]] static auto getRow(MYSQL_RES *result) -> MYSQL_ROW;

    static auto freeResult(MYSQL_RES *result) -> void;

    MYSQL *connection;
};
