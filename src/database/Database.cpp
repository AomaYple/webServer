#include "Database.h"
#include "DatabaseError.h"
#include <stdexcept>

using std::runtime_error;
using std::string, std::string_view;
using std::vector;

Database::Database(string_view host, string_view user, string_view password, string_view database, unsigned int port,
                   string_view unixSocket, unsigned long clientFlag)
    : connection{Database::initialize()} {
    this->connect(host, user, password, database, port, unixSocket, clientFlag);
}

Database::~Database() {
    if (this->connection != nullptr) mysql_close(this->connection);
}

auto Database::initialize() -> MYSQL * {
    MYSQL *tempConnection{mysql_init(nullptr)};
    if (tempConnection == nullptr) throw runtime_error("database failed to initialize connection");

    return tempConnection;
}

auto Database::connect(string_view host, string_view user, string_view password, string_view database,
                       unsigned int port, string_view unixSocket, unsigned long clientFlag) -> void {
    if (mysql_real_connect(connection, host.empty() ? nullptr : host.data(), user.data(), password.data(),
                           database.data(), port, unixSocket.empty() ? nullptr : unixSocket.data(),
                           clientFlag) == nullptr)
        throw runtime_error("database connect error: " + string{mysql_error(this->connection)});
}

auto Database::query(string_view statement) -> void {
    if (mysql_query(this->connection, statement.data()) != 0)
        throw runtime_error("database query error: " + string{mysql_error(this->connection)});
}

auto Database::storeResult() -> MYSQL_RES * {
    MYSQL_RES *result{mysql_store_result(this->connection)};

    if (result == nullptr) {
        const char *error{mysql_error(this->connection)};
        if (error != nullptr)
            throw runtime_error("database store result error: " + string{mysql_error(this->connection)});
    }

    return result;
}

auto Database::getColumnCount(MYSQL_RES *result) -> unsigned int { return mysql_num_fields(result); }

auto Database::getRow(MYSQL_RES *result) -> MYSQL_ROW { return mysql_fetch_row(result); }

auto Database::freeResult(MYSQL_RES *result) -> void { mysql_free_result(result); }
