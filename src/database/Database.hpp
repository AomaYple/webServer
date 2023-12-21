#pragma once

#include "../log/Exception.hpp"

#include <mysql/mysql.h>

#include <vector>

class Database {
public:
    Database() = default;

    Database(const Database &) = delete;

    auto operator=(const Database &) -> Database & = delete;

    Database(Database &&) noexcept;

    auto operator=(Database &&) noexcept -> Database &;

    ~Database();

    auto connect(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
                 unsigned int port, std::string_view unixSocket, unsigned long clientFlag,
                 std::source_location sourceLocation = std::source_location::current()) -> void;

    auto inquire(std::string_view statement) -> std::vector<std::vector<std::string>>;

private:
    static auto initialize(std::source_location sourceLocation = std::source_location::current()) -> MYSQL;

    auto destroy() noexcept -> void;

    auto query(std::string_view statement, std::source_location sourceLocation = std::source_location::current())
            -> void;

    [[nodiscard]] auto getResult(std::source_location sourceLocation = std::source_location::current()) -> MYSQL_RES *;

    [[nodiscard]] static auto getColumnCount(MYSQL_RES *result) noexcept -> unsigned int;

    [[nodiscard]] static auto getRow(MYSQL_RES *result, unsigned int columnCount) -> std::vector<std::string>;

    static auto freeResult(MYSQL_RES *result) noexcept -> void;

    static constinit std::mutex lock;

    MYSQL handle{Database::initialize()};
};
