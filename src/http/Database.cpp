#include "Database.h"

#include "../exception/Exception.h"

using std::source_location;
using std::string, std::string_view;
using std::vector;

Database::Database(string_view host, string_view user, string_view password, string_view database,
                   std::uint_fast32_t port, string_view unixSocket, std::uint_fast64_t clientFlag)
    : connection{Database::initialize()} {
    this->connect(host, user, password, database, port, unixSocket, clientFlag);
}

auto Database::consult(string_view statement) -> vector<vector<string>> {
    this->query(statement);

    MYSQL_RES *consultResult{this->storeResult()};

    vector<vector<string>> results;

    std::uint_fast32_t columnCount{Database::getColumnCount(consultResult)};

    for (MYSQL_ROW row{Database::getRow(consultResult)}; row != nullptr; row = Database::getRow(consultResult)) {
        vector<string> result;

        for (std::uint_fast32_t i{0}; i < columnCount; ++i) result.emplace_back(row[i]);

        results.emplace_back(std::move(result));
    }

    Database::freeResult(consultResult);

    return results;
}

Database::~Database() {
    if (this->connection != nullptr) mysql_close(this->connection);
}

auto Database::initialize(source_location sourceLocation) -> MYSQL * {
    MYSQL *connection{mysql_init(nullptr)};
    if (connection == nullptr) throw Exception{sourceLocation, Level::FATAL, "initialize error"};

    return connection;
}

auto Database::connect(string_view host, string_view user, string_view password, string_view database,
                       std::uint_fast32_t port, string_view unixSocket, std::uint_fast64_t clientFlag,
                       source_location sourceLocation) -> void {
    if (mysql_real_connect(this->connection, host.empty() ? nullptr : host.data(), user.data(), password.data(),
                           database.data(), port, unixSocket.empty() ? nullptr : unixSocket.data(),
                           clientFlag) == nullptr)
        throw Exception{sourceLocation, Level::FATAL, mysql_error(this->connection)};
}

auto Database::query(string_view statement, source_location sourceLocation) -> void {
    if (mysql_query(this->connection, statement.data()) != 0)
        throw Exception{sourceLocation, Level::FATAL, mysql_error(this->connection)};
}

auto Database::storeResult(source_location sourceLocation) -> MYSQL_RES * {
    MYSQL_RES *result{mysql_store_result(this->connection)};

    if (result == nullptr) {
        const char *error{mysql_error(this->connection)};
        if (error != nullptr) throw Exception{sourceLocation, Level::FATAL, error};
    }

    return result;
}

auto Database::getColumnCount(MYSQL_RES *result) noexcept -> std::uint_fast32_t { return mysql_num_fields(result); }

auto Database::getRow(MYSQL_RES *result) noexcept -> MYSQL_ROW { return mysql_fetch_row(result); }

auto Database::freeResult(MYSQL_RES *result) noexcept -> void { mysql_free_result(result); }
