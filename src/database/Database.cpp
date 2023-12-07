#include "Database.hpp"

#include "../log/Log.hpp"
#include "../log/logger.hpp"

Database::Database(Database &&other) noexcept : handle{other.handle} { other.handle.host = nullptr; }

auto Database::operator=(Database &&other) noexcept -> Database & {
    if (this != &other) {
        this->destroy();

        this->handle = other.handle;
        other.handle.host = nullptr;
    }

    return *this;
}

Database::~Database() { this->destroy(); }

auto Database::connect(std::string_view host, std::string_view user, std::string_view password,
                       std::string_view database, unsigned int port, std::string_view unixSocket,
                       unsigned long clientFlag, std::source_location sourceLocation) noexcept -> void {
    if (mysql_real_connect(&this->handle, host.data(), user.data(), password.data(), database.data(), port,
                           unixSocket.data(), clientFlag) == nullptr) {
        logger::push(Log{Log::Level::fatal, mysql_error(&this->handle), sourceLocation});
        logger::flush();

        std::terminate();
    }
}

auto Database::consult(std::string_view statement) noexcept -> std::vector<std::vector<std::string>> {
    std::vector<std::vector<std::string>> results;

    this->query(statement);

    MYSQL_RES *const queryResults{this->getResult()};
    if (queryResults == nullptr) return results;

    const unsigned int columnCount{Database::getColumnCount(*queryResults)};
    for (std::vector<std::string> row{Database::getRow(*queryResults, columnCount)}; !row.empty();
         row = Database::getRow(*queryResults, columnCount))
        results.emplace_back(std::move(row));

    Database::freeResult(*queryResults);

    return results;
}

auto Database::initialize(std::source_location sourceLocation) noexcept -> MYSQL {
    const std::lock_guard lockGuard{Database::lock};

    MYSQL handle;
    if (mysql_init(&handle) == nullptr) {
        logger::push(Log{Log::Level::fatal, "initialization of database failed", sourceLocation});
        logger::flush();

        std::terminate();
    }

    return handle;
}

auto Database::destroy() noexcept -> void {
    if (this->handle.host != nullptr) mysql_close(&this->handle);
}

auto Database::query(std::string_view statement, std::source_location sourceLocation) noexcept -> void {
    if (mysql_real_query(&this->handle, statement.data(), statement.size()) != 0) {
        logger::push(Log{Log::Level::error, mysql_error(&this->handle), sourceLocation});
        logger::flush();

        std::terminate();
    }
}

auto Database::getResult(std::source_location sourceLocation) noexcept -> MYSQL_RES * {
    MYSQL_RES *const result{mysql_store_result(&this->handle)};

    if (result == nullptr) {
        std::string error{mysql_error(&this->handle)};
        if (!error.empty()) {
            logger::push(Log{Log::Level::error, std::move(error), sourceLocation});
            logger::flush();

            std::terminate();
        }
    }

    return result;
}

auto Database::getColumnCount(MYSQL_RES &result) noexcept -> unsigned int { return mysql_num_fields(&result); }

auto Database::getRow(MYSQL_RES &result, unsigned int columnCount) noexcept -> std::vector<std::string> {
    std::vector<std::string> row;

    const char *const *const rowPointer{mysql_fetch_row(&result)};
    for (unsigned int i{0}; rowPointer != nullptr && i < columnCount; ++i) row.emplace_back(rowPointer[i]);

    return row;
}

auto Database::freeResult(MYSQL_RES &result) noexcept -> void { mysql_free_result(&result); }

constinit std::mutex Database::lock;
