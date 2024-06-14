#pragma once

#include <memory>
#include <mysql/mysql.h>
#include <source_location>
#include <vector>

class Database {
    struct Deleter {
        auto operator()(MYSQL *handle) const noexcept -> void;
    };

    class Result {
        struct Deleter {
            auto operator()(MYSQL_RES *handle) const noexcept -> void;
        };

    public:
        explicit Result(const std::unique_ptr<MYSQL, Database::Deleter> &databaseHandle,
                        std::source_location sourceLocation = std::source_location::current());

        Result(const Result &) = delete;

        Result(Result &&) = default;

        auto operator=(const Result &) -> Result & = delete;

        auto operator=(Result &&) -> Result & = default;

        ~Result() = default;

        [[nodiscard]] auto get() const -> std::vector<std::vector<std::string>>;

    private:
        [[nodiscard]] auto getColumnCount() const noexcept -> unsigned int;

        [[nodiscard]] auto getRow(unsigned int columnCount) const -> std::vector<std::string>;

        std::unique_ptr<MYSQL_RES, Deleter> handle;
    };

public:
    explicit Database(std::source_location sourceLocation = std::source_location::current());

    Database(const Database &) = delete;

    Database(Database &&) = default;

    auto operator=(const Database &) -> Database & = delete;

    auto operator=(Database &&) -> Database & = default;

    ~Database() = default;

    auto connect(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
                 unsigned int port, std::string_view unixSocket, unsigned long clientFlag,
                 std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto inquire(std::string_view statement) const -> std::vector<std::vector<std::string>>;

private:
    auto query(std::string_view statement, std::source_location sourceLocation = std::source_location::current()) const
        -> void;

    static constinit std::mutex lock;

    std::unique_ptr<MYSQL, Deleter> handle;
};
