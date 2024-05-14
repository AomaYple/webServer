#pragma once

#include <memory>
#include <mysql/mysql.h>
#include <source_location>
#include <vector>

class Database {
    struct Deleter {
        auto operator()(MYSQL *handle) const noexcept -> void;
    };

public:
    explicit Database(std::source_location sourceLocation = std::source_location::current());

    Database(const Database &) = delete;

    Database(Database &&) = default;

    auto operator=(const Database &) = delete;

    auto operator=(Database &&) -> Database & = default;

    ~Database() = default;

    auto connect(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
                 unsigned int port, std::string_view unixSocket, unsigned long clientFlag,
                 std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto inquire(std::string_view statement) const -> std::vector<std::vector<std::string>>;

private:
    static auto close(MYSQL *handle) noexcept -> void;

    auto query(std::string_view statement, std::source_location sourceLocation = std::source_location::current()) const
        -> void;

    [[nodiscard]] auto getResult(std::source_location sourceLocation = std::source_location::current()) const
        -> MYSQL_RES *;

    [[nodiscard]] static auto getColumnCount(MYSQL_RES *result) noexcept -> unsigned int;

    [[nodiscard]] static auto getRow(MYSQL_RES *result, unsigned int columnCount) -> std::vector<std::string>;

    static auto freeResult(MYSQL_RES *result) noexcept -> void;

    static constinit std::mutex lock;

    std::unique_ptr<MYSQL, Deleter> handle;
};
