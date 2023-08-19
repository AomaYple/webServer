#pragma once

#include <mysql/mysql.h>

#include <mutex>
#include <source_location>
#include <string_view>
#include <vector>

class Database {
public:
    Database(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
             unsigned short port, std::string_view unixSocket, unsigned long clientFlag);

    Database(const Database &) = delete;

    Database(Database &&) noexcept;

private:
    auto initialize(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto connect(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
                 unsigned short port, std::string_view unixSocket, unsigned long clientFlag,
                 std::source_location sourceLocation = std::source_location::current()) -> void;

public:
    auto consult(std::string_view statement) -> std::vector<std::vector<std::string>>;

private:
    auto query(std::string_view statement, std::source_location sourceLocation = std::source_location::current())
            -> void;

    [[nodiscard]] auto storeResult(std::source_location sourceLocation = std::source_location::current())
            -> MYSQL_RES *;

    [[nodiscard]] static auto getColumnCount(MYSQL_RES *result) noexcept -> unsigned int;

    [[nodiscard]] static auto getRow(MYSQL_RES *result) noexcept -> MYSQL_ROW;

    static auto freeResult(MYSQL_RES *result) noexcept -> void;

public:
    ~Database();

private:
    static constinit thread_local bool instance;
    static constinit std::mutex lock;

    MYSQL connection;
};
