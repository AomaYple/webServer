#pragma once

#include <mysql/mysql.h>

#include <string>
#include <vector>

class Database {
public:
    Database(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
             unsigned int port, std::string_view unixSocket, unsigned long clientFlag);

    Database(const Database &) = delete;

    Database(Database &&) noexcept;

    auto operator=(Database &&) noexcept -> Database &;

    auto insert(std::string &&table, std::vector<std::string> &&filedS, std::vector<std::string> &&values) -> void;

    [[nodiscard]] auto consult(std::vector<std::string> &&filedS, std::vector<std::string> &&tables,
                               std::string &&condition, std::string &&limit, std::string &&offset)
            -> std::vector<std::vector<std::string>>;

    ~Database();

private:
    static auto initialize() -> MYSQL *;

    auto connect(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
                 unsigned int port, std::string_view unixSocket, unsigned long clientFlag) -> void;

    auto query(std::string &&statement) -> void;

    [[nodiscard]] auto storeResult() -> MYSQL_RES *;

    [[nodiscard]] static auto getColumnCount(MYSQL_RES *result) -> unsigned int;

    [[nodiscard]] static auto getRow(MYSQL_RES *result) -> MYSQL_ROW;

    static auto freeResult(MYSQL_RES *result) -> void;

    [[nodiscard]] static auto insertCombine(std::string &&table, std::vector<std::string> &&filedS,
                                            std::vector<std::string> &&values) -> std::string;

    auto insertQuery(std::string &&statement) -> void;

    [[nodiscard]] static auto consultCombine(std::vector<std::string> &&filedS, std::vector<std::string> &&tables,
                                             std::string &&condition, std::string &&limit, std::string &&offset)
            -> std::string;

    [[nodiscard]] auto consultQuery(std::string &&statement) -> std::vector<std::vector<std::string>>;

    MYSQL *connection;
};