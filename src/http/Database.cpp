#include "Database.hpp"

#include "../log/Exception.hpp"

#include <utility>

Database::Database(Database &&other) noexcept : handle{std::exchange(other.handle, nullptr)} {}

auto Database::operator=(Database &&other) noexcept -> Database & {
    if (this == &other) return *this;

    this->close();
    this->handle = std::exchange(other.handle, nullptr);

    return *this;
}

Database::~Database() { this->close(); }

auto Database::connect(std::string_view host, std::string_view user, std::string_view password,
                       std::string_view database, unsigned int port, std::string_view unixSocket,
                       unsigned long clientFlag, std::source_location sourceLocation) const -> void {
    if (mysql_real_connect(this->handle, host.data(), user.data(), password.data(), database.data(), port,
                           unixSocket.data(), clientFlag) == nullptr) {
        throw Exception{
            Log{Log::Level::error, mysql_error(this->handle), sourceLocation}
        };
    }
}

auto Database::inquire(std::string_view statement) const -> std::vector<std::vector<std::string>> {
    std::vector<std::vector<std::string>> outcome;

    this->query(statement);

    MYSQL_RES *const result{this->getResult()};
    if (result == nullptr) return outcome;

    const unsigned int columnCount{Database::getColumnCount(result)};
    for (std::vector<std::string> row{Database::getRow(result, columnCount)}; !row.empty();
         row = Database::getRow(result, columnCount))
        outcome.emplace_back(std::move(row));

    Database::freeResult(result);

    return outcome;
}

auto Database::initialize(std::source_location sourceLocation) -> MYSQL * {
    const std::lock_guard lockGuard{Database::lock};

    MYSQL *handle{mysql_init(nullptr)};
    if (handle == nullptr) {
        throw Exception{
            Log{Log::Level::fatal, "initialization of Database handle failed", sourceLocation}
        };
    }

    return handle;
}

auto Database::close() const noexcept -> void {
    if (this->handle != nullptr) mysql_close(this->handle);
}

auto Database::query(std::string_view statement, std::source_location sourceLocation) const -> void {
    if (mysql_real_query(this->handle, statement.data(), statement.size()) != 0) {
        throw Exception{
            Log{Log::Level::error, mysql_error(this->handle), sourceLocation}
        };
    }
}

auto Database::getResult(std::source_location sourceLocation) const -> MYSQL_RES * {
    MYSQL_RES *const result{mysql_store_result(this->handle)};

    if (result == nullptr) {
        std::string error{mysql_error(this->handle)};
        if (!error.empty()) {
            throw Exception{
                Log{Log::Level::error, std::move(error), sourceLocation}
            };
        }
    }

    return result;
}

auto Database::getColumnCount(MYSQL_RES *result) noexcept -> unsigned int { return mysql_num_fields(result); }

auto Database::getRow(MYSQL_RES *result, unsigned int columnCount) -> std::vector<std::string> {
    std::vector<std::string> row;

    const char *const *const rawRow{mysql_fetch_row(result)};
    for (unsigned int i{}; rawRow != nullptr && i < columnCount; ++i) row.emplace_back(rawRow[i]);

    return row;
}

auto Database::freeResult(MYSQL_RES *result) noexcept -> void { mysql_free_result(result); }

constinit std::mutex Database::lock;
