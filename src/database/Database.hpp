#pragma once

#include <mysql/mysql.h>

#include <mutex>
#include <source_location>
#include <string_view>
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
                 std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    auto consult(std::string_view statement) noexcept -> std::vector<std::vector<std::string>>;

private:
    [[nodiscard]] static auto initialize(std::source_location sourceLocation = std::source_location::current()) noexcept
            -> MYSQL;

    auto destroy() noexcept -> void;

    auto query(std::string_view statement,
               std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    [[nodiscard]] auto getResult(std::source_location sourceLocation = std::source_location::current()) noexcept
            -> MYSQL_RES *;

    [[nodiscard]] static auto getColumnCount(MYSQL_RES &result) noexcept -> unsigned int;

    [[nodiscard]] static auto getRow(MYSQL_RES &result, unsigned int columnCount) noexcept -> std::vector<std::string>;

    static auto freeResult(MYSQL_RES &result) noexcept -> void;

    static constinit std::mutex lock;

    MYSQL handle{Database::initialize()};
};
